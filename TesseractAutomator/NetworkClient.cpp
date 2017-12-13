#include "NetworkClient.h"
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/io/coded_stream.h>
#include "Version.h"
#include <sys/capability.h>
#include <sys/prctl.h>
#include <boost/uuid/uuid_io.hpp>
#include "Error.h"

NetworkClient::NetworkClient(int port, std::string ip) :
	mSynchroWaiting(0),
	mService(),
	mUdpSocket(mService, ip::udp::endpoint(ip::udp::v4(), port + 20)),
	mTcpSocket(mService, ip::tcp::endpoint(ip::tcp::v4(), port + 20)),
	mPort(port),
	mIp(ip),
	mStrand(mService),
	mTimer(mService)
{
	mUdpSocket.set_option(boost::asio::socket_base::broadcast(true));
}

google::protobuf::uint32 NetworkClient::readHeader(char *buf)
{
	google::protobuf::uint32 size;
	google::protobuf::io::ArrayInputStream ais(buf, 4);
	google::protobuf::io::CodedInputStream coded_input(&ais);
	coded_input.ReadLittleEndian32(&size);//Decode the HDR and get the size
	return size;
}

void NetworkClient::Start()
{
	try
	{
		auto self(shared_from_this());
		if (mIp.empty())
			BroadcastNetworkInfo(mPort, VERSION);
		else
			SendNetworkInfoTo(mPort, mIp, VERSION);
		mService.run();
	}
	catch(std::exception e)
	{
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Error ASIO Service: " << e.what();
	}
}


void NetworkClient::ReceiveDataHeader()
{
	auto self(shared_from_this());
	boost::asio::async_read(mTcpSocket, boost::asio::buffer(mData, max_length), boost::asio::transfer_exactly(4), [this, self](boost::system::error_code ec, std::size_t length)
	{
		if (ec)
		{
			mTcpSocket.close();
			onMasterDisconnect();
			return;
		}

		ReceiveData(readHeader(mData));
	});
}

void NetworkClient::ReceiveData(int length)
{
	auto self(shared_from_this());
	std::shared_ptr<std::vector<char>> buffer = std::make_shared<std::vector<char>>(length);
	boost::asio::async_read(mTcpSocket, boost::asio::buffer(*buffer, length), boost::asio::transfer_exactly(length), [this, self, buffer](boost::system::error_code ec, std::size_t length)
	{
		if (!ec)
		{
			Docapost::IA::Tesseract::Proto::Message_Master m;
			m.ParseFromArray(buffer->data(), length);
			if (m.has_status())
			{
				onMasterStatusChanged(0, m.status().done(), m.status().skip(), m.status().total(), m.status().psm(), m.status().oem(), m.status().lang());
			}
			else if (m.has_synchro())
			{
				boost::unordered_map<std::string, std::vector<unsigned char>*> dict;
				for (auto& d : m.synchro().data())
				{
					if (d.has_file())
					{
						dict[d.uuid()] = new std::vector<unsigned char>(d.file().begin(), d.file().end());
					}
				}
				onMasterSynchro(m.synchro().totalthread(), m.synchro().done(), m.synchro().skip(), m.synchro().total(), m.synchro().isend(), m.synchro().pending(),dict);
				mSynchroWaiting -= dict.size();
			}
			ReceiveDataHeader();
		}
		else
		{
			BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Error Read: " << ec.message();
			mTcpSocket.close();
			onMasterDisconnect();

		}
	});
}

// DO NOT USE WITHOUT STRAND
void NetworkClient::WriteToStream(std::shared_ptr<std::vector<char>> data)
{
	mWriteQueue.push(data);
	if (mWriteQueue.size() > 1) {
		// outstanding async_write
		return;
	}

	this->WriteNextItemToStream();
}

// DO NOT USE WITHOUT DIRECTLY, USE WriteToStream(data) INSTEAD
void NetworkClient::WriteNextItemToStream()
{
	const auto buffer = mWriteQueue.front();
	boost::asio::async_write(
		mTcpSocket,
		boost::asio::buffer(*buffer, buffer->size()),
		mStrand.wrap(
			boost::bind(
				&NetworkClient::WriteHandler,
				this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred
			)
		)
	);
}

void NetworkClient::WriteHandler(const boost::system::error_code& error, const size_t bytesTransferred)
{
	if (error)
	{
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Error Write: " << error.message();
		mTcpSocket.close();
		onMasterConnected();
	}
	mWriteQueue.pop();

	if (!mWriteQueue.empty()) {
		// more messages to send
		this->WriteNextItemToStream();
	}
}

void NetworkClient::BroadcastNetworkInfo(int port, std::string version)
{
	Connect(port, boost::asio::ip::address_v4::broadcast(), version);
}
void NetworkClient::SendNetworkInfoTo(int port, std::string ip, std::string version)
{

	Connect(port, ip::address_v4::from_string(ip), version);
}

void NetworkClient::Connect(int port, ip::address_v4 ip, std::string version)
{
	auto self(shared_from_this());
	Docapost::IA::Tesseract::Proto::NetworkInfo ni;
	ni.set_port(port);
	ni.set_version(version);

	auto siz = ni.ByteSize();
	auto buffer = std::make_shared<std::vector<char>>(std::vector<char>(siz + 4));
	google::protobuf::io::ArrayOutputStream aos(buffer->data(), siz + 4);
	google::protobuf::io::CodedOutputStream coded_output(&aos);
	ni.SerializeToCodedStream(&coded_output);

	// Broacast du message en UDP pour trouver les pairs

	boost::asio::ip::udp::endpoint senderEndpoint(ip, port);

	mTimer.expires_from_now(boost::posix_time::seconds(5));

	ip::tcp::resolver resolver(mService);
	ip::tcp::resolver::query query(boost::asio::ip::host_name(), "");
	ip::tcp::resolver::iterator it = resolver.resolve(query);

	while (it != ip::tcp::resolver::iterator())
	{
		boost::asio::ip::address addr = (it++)->endpoint().address();
		auto str = addr.to_string();
		std::cerr << str << std::endl;
	}


	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Sending broadcast, Version : " << ni.version() << " Port : " << ni.port();
	mUdpSocket.async_send_to(boost::asio::buffer(*buffer, coded_output.ByteCount()), senderEndpoint, [this, self](boost::system::error_code ec, std::size_t bytes_transferred)
	{
		if (ec)
		{
			BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Error broadcast, " << ec.message();
			return;
		}
		// Une fois un pair trouver
		//std::vector<char> rcv_buffer(300);
		mUdpSocket.async_receive_from(boost::asio::buffer(mData, 1204), mMasterEndpoint, [this, self](boost::system::error_code ec, std::size_t bytes_rcv)
		{
			if (ec)
			{
				BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Error Broadcast: " << ec.message();
				return;
			}
			BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Response Broadcast";

			mTimer.cancel();
			Docapost::IA::Tesseract::Proto::NetworkInfo ni;
			ni.ParseFromArray(mData, bytes_rcv);
			// On vérifié ca version
			if (ni.version() != VERSION)
			{
				BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Version mismatch, Expected : " << VERSION << " Received : " << ni.version() << std::endl;
			}

			auto ip = mMasterEndpoint.address().to_string();

			for (int i = 0; i < 3; i++)
				try
			{
				// Et on ce connect en TCP a celui-ci
				mTcpSocket.connect(ip::tcp::endpoint(mMasterEndpoint.address(), ni.port()));
				break;
			}
			catch (std::exception& e)
			{
				mTcpSocket.close();
				std::this_thread::sleep_for(std::chrono::milliseconds(300));
			}

			ReceiveDataHeader();

			onMasterConnected();
		});
	});

	mTimer.async_wait([this, port, version, self](boost::system::error_code ec) { if (!ec) { mUdpSocket.cancel(); BroadcastNetworkInfo(port, version); } });
}

void NetworkClient::SendDeclare(int thread, std::string version)
{
	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Send declare: " << version;
	auto self(shared_from_this());
	Docapost::IA::Tesseract::Proto::Declare* d = new Docapost::IA::Tesseract::Proto::Declare();
	d->set_thread(thread);
	char str[50];
	gethostname(str, sizeof(str) - 1);
	d->set_hostname(str);

	Docapost::IA::Tesseract::Proto::Message_Slave m;
	m.set_allocated_declare(d);

	auto siz = m.ByteSize();
	auto buffer = std::make_shared<std::vector<char>>(siz + 4);
	google::protobuf::io::ArrayOutputStream aos(buffer->data(), siz + 4);
	google::protobuf::io::CodedOutputStream coded_output(&aos);
	coded_output.WriteLittleEndian32(m.ByteSize());
	m.SerializeToCodedStream(&coded_output);

	mStrand.post(
		boost::bind(
			&NetworkClient::WriteToStream,
			this,
			buffer
		)
	);
}

void NetworkClient::SendSynchro(int thread, int threadId, int req, std::vector<SlaveFileStatus*> files)
{
	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Send synchro req: " << req;
	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::trace) << "Send synchro files:" << files.size();
	auto self(shared_from_this());

	Docapost::IA::Tesseract::Proto::Synchro_Slave* s = new Docapost::IA::Tesseract::Proto::Synchro_Slave{};
	s->set_threadrunning(thread);
	s->set_nbfilesrequired(req);

	for (auto& file : files)
	{
		auto f = s->add_data();
		f->set_uuid(boost::uuids::to_string(file->uuid));
		f->set_result(file->result.data(), files.size());
		f->set_start(boost::posix_time::to_time_t(file->start));
		f->set_end(boost::posix_time::to_time_t(file->end));
		f->set_ellapsed(file->ellapsed.ticks());
		f->set_threadid(file->thread);
	}

	Docapost::IA::Tesseract::Proto::Message_Slave m;
	m.set_allocated_synchro(s);

	auto siz = m.ByteSize();
	auto buffer = std::make_shared<std::vector<char>>(siz + 4);
	google::protobuf::io::ArrayOutputStream aos(buffer->data(), siz + 4);
	google::protobuf::io::CodedOutputStream coded_output(&aos);
	coded_output.WriteLittleEndian32(m.ByteSize());
	m.SerializeToCodedStream(&coded_output);

	mStrand.post(
		boost::bind(
			&NetworkClient::WriteToStream,
			this,
			buffer
		)
	);
}

void NetworkClient::SendSynchroIfNone(int thread, int threadId, int req, std::vector<SlaveFileStatus*> files)
{
	std::lock_guard<std::mutex> lock(mThreadMutex);
	if (mSynchroWaiting < req)
	{
		auto toAdd = req - mSynchroWaiting;
		if (toAdd > 0)
		{
			mSynchroWaiting += toAdd;
			SendSynchro(thread, threadId, toAdd, files);
		}
		else
		{
			SendSynchro(thread, threadId, 0, files);
		}
	}
}

void NetworkClient::Stop()
{
	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Stopping Network";
	mService.stop();
}

NetworkClient::~NetworkClient()
{
	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Destroying Netowrk";
	if (mUdpSocket.is_open())
	{
		mUdpSocket.close();
	}
	if (mTcpSocket.is_open())
	{
		mTcpSocket.shutdown(boost::asio::socket_base::shutdown_type::shutdown_both);
		mTcpSocket.close();
	}
	if (!mService.stopped())
	{
		mService.stop();
	}
}

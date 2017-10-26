#include "NetworkClient.h"
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/io/coded_stream.h>
#include "Version.h"

NetworkClient::NetworkClient(int port) :
	synchroWaiting(0),
	service(),
	udp_socket(service, ip::udp::endpoint(ip::udp::v4(), port + 20)),
	tcp_socket(service, ip::tcp::endpoint(ip::tcp::v4(), port + 20)),
	port(port),
	strand(service),
	timer(service)
{
	/*udp_socket.set_option(ip::udp::socket::reuse_address(true));
	udp_socket.set_option(boost::asio::socket_base::broadcast(true));*/
}

google::protobuf::uint32 NetworkClient::readHeader(char *buf)
{
	google::protobuf::uint32 size;
	google::protobuf::io::ArrayInputStream ais(buf, 4);
	google::protobuf::io::CodedInputStream coded_input(&ais);
	coded_input.ReadLittleEndian32(&size);//Decode the HDR and get the size
	return size;
}

void NetworkClient::Start(int port)
{
	auto self(shared_from_this());
	BroadcastNetworkInfo(port, VERSION);
	service.run();
}


void NetworkClient::ReceiveDataHeader()
{
	auto self(shared_from_this());
	boost::asio::async_read(tcp_socket, boost::asio::buffer(data_, max_length), boost::asio::transfer_exactly(4), [this, self](boost::system::error_code ec, std::size_t length)
	{
		if (ec)
		{
			tcp_socket.close();
			onMasterDisconnect();
			return;
		}

		ReceiveData(readHeader(data_));
	});
}

void NetworkClient::ReceiveData(int length)
{
	auto self(shared_from_this());
	std::shared_ptr<std::vector<char>> buffer = std::make_shared<std::vector<char>>(length);
	boost::asio::async_read(tcp_socket, boost::asio::buffer(*buffer, length), boost::asio::transfer_exactly(length), [this, self, buffer](boost::system::error_code ec, std::size_t length)
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
				onMasterSynchro(m.synchro().totalthread(), m.synchro().done(), dict);
			}
			//SendStatus(10, 20, 15, 1, 3, "fra");
			ReceiveDataHeader();
		}
		else
		{
			tcp_socket.close();
			onMasterDisconnect();

		}
	});
}

// DO NOT USE WITHOUT STRAND
void NetworkClient::WriteToStream(std::shared_ptr<std::vector<char>> data)
{
	writeQueue.push(data);
	if (writeQueue.size() > 1) {
		// outstanding async_write
		return;
	}

	this->WriteNextItemToStream();
}

// DO NOT USE WITHOUT DIRECTLY, USE WriteToStream(data) INSTEAD
void NetworkClient::WriteNextItemToStream()
{
	const auto buffer = writeQueue.front();
	boost::asio::async_write(
		tcp_socket,
		boost::asio::buffer(*buffer, buffer->size()),
		strand.wrap(
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
	writeQueue.pop();
	if (error)
	{
		tcp_socket.close();
		onMasterConnected();
	}
	if (!writeQueue.empty()) {
		// more messages to send
		this->WriteNextItemToStream();
	}
}

void NetworkClient::BroadcastNetworkInfo(int port, std::string version)
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
	boost::asio::ip::udp::endpoint senderEndpoint(boost::asio::ip::address_v4::broadcast(), port);

	timer.expires_from_now(boost::posix_time::seconds(1));

	udp_socket.async_send_to(boost::asio::buffer(*buffer, coded_output.ByteCount()), senderEndpoint, [this, self](boost::system::error_code ec, std::size_t bytes_transferred)
	{
		timer.cancel();
		// Une fois un pair trouver
		boost::asio::ip::udp::endpoint masterEndpoint;
		std::vector<char> rcv_buffer(300);
		auto bytes_rcv = udp_socket.receive_from(boost::asio::buffer(rcv_buffer, 300), masterEndpoint);


		Docapost::IA::Tesseract::Proto::NetworkInfo ni;
		ni.ParseFromArray(rcv_buffer.data(), bytes_rcv);
		// On vérifié ca version
		if (ni.version() != VERSION)
		{
			std::cout << "Version mismatch, Expected : " << VERSION << " Received : " << ni.version() << std::endl;
		}

		auto ip = masterEndpoint.address().to_string();

		for (int i = 0; i < 3; i++)
			try
		{
			// Et on ce connect en TCP a celui-ci
			tcp_socket.connect(ip::tcp::endpoint(masterEndpoint.address(), ni.port()));
			break;
		}
		catch (std::exception& e)
		{
			tcp_socket.close();
			std::this_thread::sleep_for(std::chrono::milliseconds(300));
		}

		ReceiveDataHeader();

		onMasterConnected();
	});

	timer.async_wait([this, port, version, self](boost::system::error_code ec) { std::cout << "Expire, retry" << ec <<  std::endl; if (!ec) { BroadcastNetworkInfo(port, version); } });
}

void NetworkClient::SendDeclare(int thread, std::string version)
{
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

	/*boost::asio::async_write(tcp_socket, boost::asio::buffer(*buffer, coded_output.ByteCount()), [this, self](boost::system::error_code ec, std::size_t bytes_transferred)
	{
		if (ec)
		{
			tcp_socket.close();
			onMasterDisconnect();
			return;
		}
	});*/
	strand.post(
		boost::bind(
			&NetworkClient::WriteToStream,
			this,
			buffer
		)
	);
}

void NetworkClient::SendSynchro(int thread, int threadId, int req, std::vector<SlaveFileStatus*> files)
{
	auto self(shared_from_this());
	++synchroWaiting;

	Docapost::IA::Tesseract::Proto::Synchro_Slave* s = new Docapost::IA::Tesseract::Proto::Synchro_Slave{};
	s->set_threadrunning(thread);
	s->set_nbfilesrequired(req);

	for (auto& file : files)
	{
		auto f = s->add_data();
		f->set_uuid(file->uuid);
		f->set_result(file->result.data(), files.size());
		f->set_start(boost::posix_time::to_time_t(file->start));
		f->set_end(boost::posix_time::to_time_t(file->end));
		f->set_ellapsed(file->ellapsed.ticks());
		f->set_threadid(threadId);
	}

	Docapost::IA::Tesseract::Proto::Message_Slave m;
	m.set_allocated_synchro(s);

	auto siz = m.ByteSize();
	auto buffer = std::make_shared<std::vector<char>>(siz + 4);
	google::protobuf::io::ArrayOutputStream aos(buffer->data(), siz + 4);
	google::protobuf::io::CodedOutputStream coded_output(&aos);
	coded_output.WriteLittleEndian32(m.ByteSize());
	m.SerializeToCodedStream(&coded_output);

	/*boost::asio::async_write(tcp_socket, boost::asio::buffer(*buffer, coded_output.ByteCount()), [this, self](boost::system::error_code ec, std::size_t length)
	{
		if (ec)
		{
			std::cout << "error : " << ec.value() << std::endl;
		}
	});*/

	strand.post(
		boost::bind(
			&NetworkClient::WriteToStream,
			this,
			buffer
		)
	);
}

void NetworkClient::SendSynchroIfNone(int thread, int threadId, int req, std::vector<SlaveFileStatus*> files)
{
	g_thread_mutex.lock();
	if (synchroWaiting == 0)
	{
		SendSynchro(thread, threadId, req, files);
		g_thread_mutex.unlock();
	}
	else
	{
		g_thread_mutex.unlock();
	}
}

void NetworkClient::Stop()
{
	service.stop();
}

NetworkClient::~NetworkClient()
{
	std::cout << "exit 1" << std::endl;
	if (udp_socket.is_open())
	{
		std::cout << "exit 1.1" << std::endl;
		udp_socket.close();
	}
	std::cout << "exit 2" << std::endl;
	if (tcp_socket.is_open())
	{
		std::cout << "exit 2.0" << std::endl;
		tcp_socket.shutdown(boost::asio::socket_base::shutdown_type::shutdown_both);
		std::cout << "exit 2.1" << std::endl;
		tcp_socket.close();
	}
	std::cout << "exit 3" << std::endl;
	if (!service.stopped())
	{
		std::cout << "exit 3.0" << std::endl;
		service.stop();
	}
}

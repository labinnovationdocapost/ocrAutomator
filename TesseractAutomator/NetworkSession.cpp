#include "NetworkSession.h"
#include "Network.h"

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>
#include <unordered_map>
#include <boost/date_time.hpp>

NetworkSession::NetworkSession(ip::tcp::socket socket, boost::uuids::uuid id) : socket(std::move(socket)), id(id), strand(socket.get_io_service())
{
}

void NetworkSession::Start()
{
	ReceiveDataHeader();
}

void NetworkSession::Stop()
{
}

google::protobuf::uint32 readHeader(char *buf)
{
	google::protobuf::uint32 size;
	google::protobuf::io::ArrayInputStream ais(buf, 4);
	google::protobuf::io::CodedInputStream coded_input(&ais);
	coded_input.ReadLittleEndian32(&size);//Decode the HDR and get the size
	return size;
}

void NetworkSession::ReceiveData(int length)
{
	auto self(shared_from_this());
	std::shared_ptr<std::vector<char>> buffer = std::make_shared<std::vector<char>>(length);
	boost::asio::async_read(socket, boost::asio::buffer(*buffer, length), boost::asio::transfer_exactly(length), [this, self, buffer](boost::system::error_code ec, std::size_t length)
	{
		if (!ec)
		{
			Docapost::IA::Tesseract::Proto::Message_Slave m;
			m.ParseFromArray(buffer->data(), length);
			if(m.has_declare())
			{
				hostname = m.declare().hostname();
				onSlaveConnect(this, m.declare().thread(), hostname);
			}
			else if(m.has_synchro())
			{
				std::vector<std::tuple<std::string, int, boost::posix_time::ptime, boost::posix_time::ptime, boost::posix_time::time_duration, std::string>> dict;
				for(auto& d : m.synchro().data())
				{
					if(d.has_result())
					{
						file_send.erase(d.uuid());
						dict.push_back(std::make_tuple(d.uuid(), d.threadid(), boost::posix_time::from_time_t(d.start()), boost::posix_time::from_time_t(d.end()), boost::posix_time::time_duration(0,0,0,d.ellapsed()), d.result()));
					}
				}
				onSlaveSynchro(this, m.synchro().threadrunning(), m.synchro().nbfilesrequired(), dict);
			}
			//SendStatus(10, 20, 15, 1, 3, "fra");
			ReceiveDataHeader();
		}
		else
		{
			socket.close();
			onSlaveDisconnect(this, file_send);

		}
	});
}
void NetworkSession::ReceiveDataHeader()
{
	auto self(shared_from_this());
	boost::asio::async_read(socket, boost::asio::buffer(data_, max_length), boost::asio::transfer_exactly(4), [this, self](boost::system::error_code ec, std::size_t length)
	{
		if (!ec)
		{
			ReceiveData(readHeader(data_));
		}
		else
		{
			socket.close();
			onSlaveDisconnect(this, file_send);

		}
	});
}


// DO NOT USE WITHOUT STRAND
void NetworkSession::WriteToStream(std::shared_ptr<std::vector<char>> data)
{
	writeQueue.push(data);
	if (writeQueue.size() > 1) {
		// outstanding async_write
		return;
	}

	this->WriteNextItemToStream();
}

// DO NOT USE WITHOUT DIRECTLY, USE WriteToStream(data) INSTEAD
void NetworkSession::WriteNextItemToStream()
{
	const auto buffer = writeQueue.front();
	boost::asio::async_write(
		socket,
		boost::asio::buffer(*buffer, buffer->size()),
		strand.wrap(
			boost::bind(
				&NetworkSession::WriteHandler,
				this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred
			)
		)
	);
}

void NetworkSession::WriteHandler(const boost::system::error_code& error, const size_t bytesTransferred)
{
	writeQueue.pop();
	if (error)
	{
		socket.close();
		onSlaveDisconnect(this, file_send);
	}
	if (!writeQueue.empty()) {
		// more messages to send
		this->WriteNextItemToStream();
	}
}

void NetworkSession::SendStatus(int done, int skip, int total, int psm, int oem, std::string lang)
{
	auto self(shared_from_this());
	Docapost::IA::Tesseract::Proto::Status* s = new Docapost::IA::Tesseract::Proto::Status{};
	s->set_done(done);
	s->set_lang(lang);
	s->set_oem(oem);
	s->set_psm(psm);
	s->set_skip(skip);
	s->set_total(total);

	Docapost::IA::Tesseract::Proto::Message_Master m;
	m.set_allocated_status(s);
	
	auto siz = m.ByteSize();
	auto buffer = std::make_shared<std::vector<char>>(siz + 4);
	google::protobuf::io::ArrayOutputStream aos(buffer->data(), siz + 4);
	google::protobuf::io::CodedOutputStream coded_output(&aos);
	coded_output.WriteLittleEndian32(m.ByteSize());
	m.SerializePartialToCodedStream(&coded_output);

	strand.post(
		boost::bind(
			&NetworkSession::WriteToStream,
			this,
			buffer
		)
	);
}

void NetworkSession::SendSynchro(int thread, int done, int skip, int total, bool isEnd, boost::unordered_map<std::string, std::vector<unsigned char>*> files)
{
	auto self(shared_from_this());

	Docapost::IA::Tesseract::Proto::Synchro_Master* s = new Docapost::IA::Tesseract::Proto::Synchro_Master{};
	s->set_totalthread(thread);
	s->set_done(done);
	s->set_skip(skip);
	s->set_total(total);
	s->set_isend(isEnd);

	for(auto& file : files)
	{
		auto f = s->add_data();
		f->set_uuid(file.first);
		f->set_file(file.second->data(), file.second->size());
		file_send[file.first] = false;
		//delete file.second;
	}

	Docapost::IA::Tesseract::Proto::Message_Master m;
	m.set_allocated_synchro(s);

	auto siz = m.ByteSize();
	auto buffer = std::make_shared<std::vector<char>>(siz + 4);
	google::protobuf::io::ArrayOutputStream aos(buffer->data(), siz + 4);
	google::protobuf::io::CodedOutputStream coded_output(&aos);
	coded_output.WriteLittleEndian32(m.ByteSize());
	m.SerializePartialToCodedStream(&coded_output);

	strand.dispatch(
		boost::bind(
			&NetworkSession::WriteToStream,
			this,
			buffer
		)
	);
}

NetworkSession::~NetworkSession()
{
}

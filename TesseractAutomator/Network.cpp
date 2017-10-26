#include "Network.h"
#include <boost/uuid/uuid_io.hpp>
#include <boost/smart_ptr/make_shared.hpp>

namespace ip = boost::asio::ip;
namespace proto = Docapost::IA::Tesseract::Proto;

void Network::BroadcastLoop()
{
	udp_socket.async_receive_from(boost::asio::buffer(data_, max_length), udp_sender_endpoint, [this](boost::system::error_code ec, std::size_t bytes_recvd)
	{
		if (!ec && bytes_recvd > 0)
		{
			RespondBroadcast();
			BroadcastLoop();
		}
		else
		{
			BroadcastLoop();
		}
	});
} 
void Network::RespondBroadcast()
{
	proto::NetworkInfo s;
	s.set_port(port);
	s.set_version(VERSION);
	
	
	s.SerializeToArray(data_, 1024);
	udp_socket.async_send_to(
		boost::asio::buffer(data_, s.ByteSize()), udp_sender_endpoint,
		[this](boost::system::error_code /*ec*/, std::size_t /*bytes_sent*/)
	{

	});
}

void Network::InitBroadcastReceiver()
{
	BroadcastLoop();
}

void Network::InitComm()
{
	tcp_acceptor.async_accept(tcp_socket,
		[this](boost::system::error_code ec)
	{
		if (!ec)
		{
			auto obj = std::make_shared<NetworkSession>(std::move(tcp_socket), gen());

			obj->onSlaveConnect.connect([this](NetworkSession* ns, int thread, std::string hostname)
			{
				auto id = boost::uuids::to_string(ns->GetId());
				this->connections[id] = ns;
				this->onSlaveConnect(ns, thread, hostname);
			});

			obj->onSlaveSynchro.connect([this](NetworkSession* ns, int thread, int required, std::vector<std::tuple<std::string, int, boost::posix_time::ptime, boost::posix_time::ptime, boost::posix_time::time_duration, std::string>>& results)
			{
				this->onSlaveSynchro(ns, thread, required, results);
			});


			obj->onSlaveDisconnect.connect([this](NetworkSession* ns, boost::unordered_map<std::string, bool>& noUsed)
			{
				auto id = boost::uuids::to_string(ns->GetId());
				this->connections.erase(id);
				this->onSlaveDisconnect(ns, noUsed);
			});
			obj->Start();
		}

		InitComm();
	});
}

void Network::Start()
{
	std::lock_guard<std::mutex> lock(g_state);
	service.run();
}
void Network::Stop()
{
	const auto conn = connections;
	for (auto& c : conn)
	{
		c.second->onSlaveDisconnect.disconnect_all_slots();
		//delete c.second;
	}
	udp_socket.close();
	tcp_acceptor.close();
	service.stop();
	std::lock_guard<std::mutex> lock(g_state);
}

Network::Network(short port) : 
service(),
udp_socket(service, ip::udp::endpoint(ip::udp::v4(), port)),
tcp_socket(service),
tcp_acceptor(service, ip::tcp::endpoint(ip::tcp::v4(), port)),
port(port)
{
}

Network::~Network()
{
}

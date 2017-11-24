#include "Network.h"
#include <boost/uuid/uuid_io.hpp>
#include <boost/smart_ptr/make_shared.hpp>

namespace ip = boost::asio::ip;
namespace proto = Docapost::IA::Tesseract::Proto;

void Network::BroadcastLoop()
{
	mUdpSocket.async_receive_from(boost::asio::buffer(mDefaultBuffer, max_length), mUdpSenderEndpoint, [this](boost::system::error_code ec, std::size_t bytes_recvd)
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
	s.set_port(mPort);
	s.set_version(VERSION);
	
	
	s.SerializeToArray(mDefaultBuffer, 1024);
	mUdpSocket.async_send_to(
		boost::asio::buffer(mDefaultBuffer, s.ByteSize()), mUdpSenderEndpoint,
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
	mTcpAcceptor.async_accept(mTcpSocket,
		[this](boost::system::error_code ec)
	{
		if (!ec)
		{
			auto obj = std::make_shared<NetworkSession>(std::move(mTcpSocket), mGen());

			obj->onSlaveConnect.connect([this](NetworkSession* ns, int thread, std::string hostname)
			{
				auto id = boost::uuids::to_string(ns->Id());
				this->mConnections[id] = ns;
				this->onSlaveConnect(ns, thread, hostname);
			});

			obj->onSlaveSynchro.connect([this](NetworkSession* ns, int thread, int required, std::vector<std::tuple<std::string, int, boost::posix_time::ptime, boost::posix_time::ptime, boost::posix_time::time_duration, std::string>>& results)
			{
				this->onSlaveSynchro(ns, thread, required, results);
			});


			obj->onSlaveDisconnect.connect([this](NetworkSession* ns, boost::unordered_map<std::string, bool>& noUsed)
			{
				auto id = boost::uuids::to_string(ns->Id());
				this->mConnections.erase(id);
				this->onSlaveDisconnect(ns, noUsed);
			});

			// On a besoin de faire tourner la communication dans un nouveau thread pour pouvoir accepter d'autre client
			std::thread([obj]()
			{
				obj->Start();
			}).detach();
		}

		InitComm();
	});
}

void Network::Start()
{
	std::lock_guard<std::mutex> lock(mStateMutex);
	mService.run();
}
void Network::Stop()
{
	const auto conn = mConnections;
	for (auto& c : conn)
	{
		c.second->onSlaveDisconnect.disconnect_all_slots();
		//delete c.second;
	}
	mUdpSocket.close();
	mTcpAcceptor.close();
	mService.stop();
	std::lock_guard<std::mutex> lock(mStateMutex);
}

Network::Network(short port) : 
mService(),
mUdpSocket(mService, ip::udp::endpoint(ip::udp::v4(), port)),
mTcpSocket(mService),
mTcpAcceptor(mService, ip::tcp::endpoint(ip::tcp::v4(), port)),
mPort(port)
{
}

Network::~Network()
{
}

#pragma once
#include <boost/asio.hpp>
#include <thread>
#include <fstream>
#include <mutex>
#include "NetworkSession.h"
#include <boost/uuid/random_generator.hpp>
#include <boost/unordered_map.hpp>
#include <boost/signals2.hpp>

namespace ip = boost::asio::ip;

class Network
{
private:
	enum { max_length = 1024 };
	char mDefaultBuffer[max_length];

	boost::asio::io_service mService;

	ip::udp::socket mUdpSocket;
	ip::tcp::socket mTcpSocket;
	ip::tcp::acceptor mTcpAcceptor;
	ip::udp::endpoint mUdpSenderEndpoint;
	ip::tcp::endpoint mTcpSenderEndpoint;

	std::thread mThreadBroadcast;

	std::mutex mStateMutex;

	void BroadcastLoop();

	void RespondBroadcast();


	int mPort;

	boost::unordered_map<std::string, NetworkSession*> mConnections;
	boost::uuids::basic_random_generator<boost::mt19937> mGen = boost::uuids::basic_random_generator<boost::mt19937>();

public:
	boost::signals2::signal<void(NetworkSession*, int, int, std::vector<std::tuple<std::string, int, boost::posix_time::ptime, boost::posix_time::ptime, boost::posix_time::time_duration, std::string>>&)> onSlaveSynchro;
	boost::signals2::signal<void(NetworkSession*, int, std::string)> onSlaveConnect;
	boost::signals2::signal<void(NetworkSession*, boost::unordered_map<std::string, bool>&)> onSlaveDisconnect;

	int Port() { return mPort; }

	void InitBroadcastReceiver();
	void InitComm();
	void Start();
	void Stop();
	explicit Network(short port);
	~Network();
};


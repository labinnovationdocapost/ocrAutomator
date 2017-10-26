#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <boost/asio.hpp>
#include <thread>
#include <iostream>
#include <fstream>
#include <mutex>
#include "NetworkSession.h"
#include "Version.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/unordered_map.hpp>
#include <boost/signals2.hpp>
#include "Protobuf_all.h"

namespace ip = boost::asio::ip;

class Network
{
private:
	enum { max_length = 1024 };
	char data_[max_length];

	boost::asio::io_service service;

	ip::udp::socket udp_socket;
	ip::tcp::socket tcp_socket;
	ip::tcp::acceptor tcp_acceptor;
	ip::udp::endpoint udp_sender_endpoint;
	ip::tcp::endpoint tcp_sender_endpoint;

	std::thread broadcast;

	std::mutex g_state;

	void BroadcastLoop();

	void RespondBroadcast();


	int port;

	boost::unordered_map<std::string, NetworkSession*> connections;
	boost::uuids::basic_random_generator<boost::mt19937> gen = boost::uuids::basic_random_generator<boost::mt19937>();

public:
	boost::signals2::signal<void(NetworkSession*, int, int, std::vector<std::tuple<std::string, int, boost::posix_time::ptime, boost::posix_time::ptime, boost::posix_time::time_duration, std::string>>&)> onSlaveSynchro;
	boost::signals2::signal<void(NetworkSession*, int, std::string)> onSlaveConnect;
	boost::signals2::signal<void(NetworkSession*, boost::unordered_map<std::string, bool>&)> onSlaveDisconnect;

	void InitBroadcastReceiver();
	void InitComm();
	void Start();
	void Stop();
	Network(short port);
	~Network();
};


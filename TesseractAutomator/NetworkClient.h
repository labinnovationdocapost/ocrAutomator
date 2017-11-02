#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <boost/asio.hpp>
#include <atomic>
#include <thread>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/uuid/uuid.hpp>
#include <boost/unordered_map.hpp>
#include <boost/signals2.hpp>
#include <google/protobuf/stubs/atomicops.h>
#include "Protobuf_all.h"
#include "FileStatus.h"
#include <mutex>
#include <queue>

namespace ip = boost::asio::ip;

class NetworkClient : public std::enable_shared_from_this<NetworkClient>
{
private:
	std::atomic<int> synchroWaiting;
	std::mutex g_thread_mutex;

	boost::asio::io_service service;

	ip::udp::socket udp_socket;
	ip::tcp::socket tcp_socket;
	ip::udp::endpoint udp_sender_endpoint;
	ip::tcp::endpoint tcp_sender_endpoint;
	int port;

	enum { max_length = 1024 };
	char data_[max_length];

	void ReceiveData(int length);
	void WriteToStream(std::shared_ptr<std::vector<char>> data);
	void WriteNextItemToStream();
	void ReceiveDataHeader();
	void WriteHandler(const boost::system::error_code& error, const size_t bytesTransferred);

	boost::uuids::uuid id;

	std::queue<std::shared_ptr<std::vector<char>>> writeQueue;
	boost::asio::io_service::strand strand;
	std::thread* mainThread;
	boost::asio::deadline_timer timer;
	boost::asio::ip::udp::endpoint masterEndpoint;
public:
	boost::signals2::signal<void()> onMasterConnected;
	boost::signals2::signal<void()> onMasterDisconnect;
	// threadToRun, done, skip, total, psm, oem, lang
	boost::signals2::signal<void(int, int, int, int, int, int, std::string)> onMasterStatusChanged;
	// thread, files
	boost::signals2::signal<void(int, int, int, int ,bool, boost::unordered_map<std::string, std::vector<unsigned char>*>&)> onMasterSynchro;

	explicit NetworkClient(int port);
	google::protobuf::uint32 readHeader(char* buf);
	void Start(int thread);
	void BroadcastNetworkInfo(int port, std::string version);
	void SendDeclare(int thread, std::string version);
	void SendSynchro(int thread, int threadId, int req, std::vector<SlaveFileStatus*> files);
	void SendSynchroIfNone(int thread, int threadId, int req, std::vector<SlaveFileStatus*> files);
	boost::uuids::uuid& GetId() { return id; }
	bool IsOpen() { return tcp_socket.is_open(); }
	std::string GetRemoteAddress() { return tcp_socket.remote_endpoint().address().to_string(); }
	int GetSynchroWaiting() { return synchroWaiting; }
	void Stop();
	~NetworkClient();
};


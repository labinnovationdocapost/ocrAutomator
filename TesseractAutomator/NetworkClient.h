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
#include "SlaveFileStatus.h"
#include <mutex>
#include <queue>

namespace ip = boost::asio::ip;

class NetworkClient : public std::enable_shared_from_this<NetworkClient>
{
private:
	std::atomic<int> mSynchroWaiting;
	std::mutex mThreadMutex;

	boost::asio::io_service mService;

	ip::udp::socket mUdpSocket;
	ip::tcp::socket mTcpSocket;
	ip::udp::endpoint mUdpSenderEndpoint;
	ip::tcp::endpoint mTcpSenderEndpoint;
	int mPort;
	std::string mIp;

	enum { max_length = 1024 };
	char mData[max_length];

	void ReceiveData(int length);
	void WriteToStream(std::shared_ptr<std::vector<char>> data);
	void WriteNextItemToStream();
	void ReceiveDataHeader();
	void WriteHandler(const boost::system::error_code& error, const size_t bytesTransferred);

	boost::uuids::uuid id;

	std::queue<std::shared_ptr<std::vector<char>>> mWriteQueue;
	boost::asio::io_service::strand mStrand;
	std::thread* mMainThread;
	boost::asio::deadline_timer mTimer;
	boost::asio::ip::udp::endpoint mMasterEndpoint;
public:
	boost::signals2::signal<void()> onMasterConnected;
	boost::signals2::signal<void()> onMasterDisconnect;
	// threadToRun, done, skip, total, psm, oem, lang
	boost::signals2::signal<void(int, int, int, int, int, int, std::string)> onMasterStatusChanged;
	// thread, files
	boost::signals2::signal<void(int, int, int, int ,bool, int, boost::unordered_map<std::string, std::vector<unsigned char>*>&)> onMasterSynchro;

	explicit NetworkClient(int port, std::string ip);
	google::protobuf::uint32 readHeader(char* buf);
	void Start();
	void BroadcastNetworkInfo(int port, std::string version);
	void SendNetworkInfoTo(int port, std::string ip, std::string version);
	void Connect(int port, ip::address_v4 endpoint, std::string ip);
	void SendDeclare(int thread, std::string version);
	void SendSynchro(int thread, int threadId, int req, std::vector<SlaveFileStatus*> files);
	void SendSynchroIfNone(int thread, int threadId, int req, std::vector<SlaveFileStatus*> files);
	boost::uuids::uuid& GetId() { return id; }
	bool IsOpen() const { return !mService.stopped() && mTcpSocket.is_open(); }
	std::string GetRemoteAddress() const { return mTcpSocket.remote_endpoint().address().to_string(); }
	int GetSynchroWaiting() const { return mSynchroWaiting; }
	int Port() const { return mPort; }
	void Stop();
	~NetworkClient();
};


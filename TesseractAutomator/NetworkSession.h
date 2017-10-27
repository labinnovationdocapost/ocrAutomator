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
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/uuid/uuid.hpp>
#include <boost/unordered_map.hpp>
#include <boost/signals2.hpp>
#include "Protobuf_all.h"
#include <queue>

namespace ip = boost::asio::ip;

class NetworkSession : public std::enable_shared_from_this<NetworkSession>
{
private:
	ip::tcp::socket socket;

	enum { max_length = 1024 };
	char data_[max_length];

	void ReceiveData(int length);
	void ReceiveDataHeader();

	std::string hostname;

	boost::unordered_map<std::string, bool> file_send;
	boost::uuids::uuid id;

	std::queue<std::shared_ptr<std::vector<char>>> writeQueue; 
	boost::asio::io_service::strand strand;
	void WriteToStream(std::shared_ptr<std::vector<char>>);
	void WriteNextItemToStream();
	void WriteHandler(const boost::system::error_code& error,const size_t bytesTransferred);
public:
	boost::signals2::signal<void(NetworkSession*, int, int, std::vector<std::tuple<std::string, int, boost::posix_time::ptime, boost::posix_time::ptime, boost::posix_time::time_duration, std::string>>&)> onSlaveSynchro;
	boost::signals2::signal<void(NetworkSession*, int, std::string)> onSlaveConnect;
	boost::signals2::signal<void(NetworkSession*, boost::unordered_map<std::string, bool>&)> onSlaveDisconnect;

	explicit NetworkSession(ip::tcp::socket socket, boost::uuids::uuid id);
	void Start();
	void Stop();
	void SendStatus(int done, int skip, int total, int psm, int oem, std::string lang);
	void SendSynchro(int thread, int done, int skip, int total, bool isEnd, boost::unordered_map<std::string, std::vector<unsigned char>*> files);
	boost::uuids::uuid& GetId() { return id; }
	boost::unordered_map<std::string, bool> GetFileState() const { return file_send; }
	std::string GetHostname() const { return hostname; }
	~NetworkSession();
};

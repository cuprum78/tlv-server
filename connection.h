#pragma once

#include <iostream>
#include <array>

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

#include "packet.h"

namespace Server
{

class ClientConn : public std::enable_shared_from_this<ClientConn>
{
public:
	using ResetCtr = std::function<void()>;

	ClientConn(boost::asio::io_service& ios);
	void start(ResetCtr&& cb);
	boost::asio::ip::tcp::socket& socket();
	~ClientConn();

private:
	void handleRead(const boost::system::error_code& err,
			size_t bytesTransferred);
	void handleWrite(const boost::system::error_code& err,
			 size_t bytesTransferred);

	std::weak_ptr<ClientConn> getWeak();
	const std::string hexDump(uint16_t type, uint8_t* buffer, uint32_t length);
	const std::string getTypeString(uint16_t type);
	void shutdownConn();
	static void handleTimer(std::weak_ptr<ClientConn> weakthis,
				const boost::system::error_code& ec);

	//Data Members
	boost::asio::ip::tcp::socket socket_;

	//All the requests are wrapped on the strand
	boost::asio::io_service::strand strand_;

	static const uint32_t ACCEPTABLE_LENGTH = 1024;
	std::array<uint8_t, ACCEPTABLE_LENGTH> data_;

	//The purpose of this timer is to keep track of client connection that
	//hogs the listener resources by keeping an open connection indefinately.
	//The idea is to kill the connection if there are no reads or writes
	//on this connection for WAIT_SECONDS.
	//{POTENTIAL OPTION}There could also be another timer which could keep track of absolute
	//connection timeout
	//{POTENTIAL OPTION}This could also be done differently where ConnectionManager keeps track of a
	//timer (like a garbage collector) and check for all connections that are idle and
	//exceeded a time theshold and cleanup all connections
	boost::asio::steady_timer connection_timer_;

	//time to wait if no read write on this connection
	static uint32_t WAIT_SECONDS;

	ResetCtr resetCtr_;
};

} // Server

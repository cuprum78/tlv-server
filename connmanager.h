#pragma once

#include <iostream>
#include <memory>

#include <boost/asio.hpp>

#include "connection.h"

namespace Server
{
/*
  ConnectionManager is a mediator between listener and Connection object.
  It is tasked with setup of acceptor and it does the task of accepting the
  incoming connection, spawning a connection object and associating the
  incoming connection with the connection object.
*/

class ConnectionManager : public std::enable_shared_from_this<ConnectionManager>
{
public:
	using AcceptCB = std::function<void(const boost::system::error_code&)>;
	using WeakThis = std::weak_ptr<ConnectionManager>;

	ConnectionManager(boost::asio::io_service& ios, short port);
	void asyncAccept(AcceptCB cb);
	void start();
	~ConnectionManager();

private:
	static void setupConnection(WeakThis,
				    const boost::system::error_code& err,
				    AcceptCB cb);

	static void resetCounter(WeakThis weakThis);
	WeakThis getWeak();

	boost::asio::io_service& ios_;
	boost::asio::ip::tcp::acceptor tcpAcceptor_;
	std::shared_ptr<Server::ClientConn> conn;
	std::atomic<int> currentReqCount_;
};

} // namespace

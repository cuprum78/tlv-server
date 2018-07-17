#include <functional>
#include <thread>

#include <boost/bind.hpp>

#include "connmanager.h"


namespace Server
{

	//Setup acceptor for INADDR_ANY and the port specified from the command line.
	ConnectionManager::ConnectionManager(boost::asio::io_service& ios, short port)
		: ios_(ios),
		  tcpAcceptor_(ios)
	{
		boost::asio::ip::tcp::endpoint ep(boost::asio::ip::tcp::v4(), port);
		tcpAcceptor_.open(ep.protocol());
		tcpAcceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		tcpAcceptor_.bind(ep);
		tcpAcceptor_.listen();
		currentReqCount_.store(0);
	}

	void ConnectionManager::start()
	{
		//Nothing specific at this point
		return;
	}

	ConnectionManager::~ConnectionManager()
	{
	}

	//This method accepts the incomming connection and delegates the task of connection
	//creation to setupConnection(...) method.
	void ConnectionManager::asyncAccept(AcceptCB cb)
	{
		if (!cb)
		{
			std::cout << "Invalid callback." << std::endl;
			return;
		}

		conn.reset(new Server::ClientConn(ios_));

		ClientConn::ResetCtr resetcb = std::bind(&ConnectionManager::resetCounter,
							 getWeak());

		tcpAcceptor_.async_accept(conn->socket(),
					  std::bind(&ConnectionManager::setupConnection,
						    getWeak(),
						    std::placeholders::_1,
						    cb));
	}

	//This method is responsible to associate incomming request with
	//connection object. Once this is done the connection object is on it own.
	void ConnectionManager::setupConnection(WeakThis weakThis,
						const boost::system::error_code& err,
						AcceptCB cb)
	{
		std::shared_ptr<ConnectionManager> SharedThis = weakThis.lock();
		if (!SharedThis)
		{
			std::cout << "setupConnection failure" << std::endl;
			return;
		}

		if (!err)
		{
			SharedThis->currentReqCount_.fetch_add(1, std::memory_order_relaxed);

			int value = SharedThis->currentReqCount_;
			std::cout << "Connection count: " << value << std::endl;

			SharedThis->conn->start(std::bind(&ConnectionManager::resetCounter,
							  weakThis));
		}

		cb(err);
	}

	//Invoked from connections destructor.
	void ConnectionManager::resetCounter(WeakThis weakThis)
	{
		std::shared_ptr<ConnectionManager> SharedThis = weakThis.lock();
		if (SharedThis)
		{
			SharedThis->currentReqCount_.fetch_sub(1, std::memory_order_relaxed);
		}
	}

	ConnectionManager::WeakThis ConnectionManager::getWeak()
	{
		return shared_from_this();
	}
} // namespace

#pragma once

#include <iostream>
#include <atomic>
#include <memory>

#include <boost/asio.hpp>

#include "connmanager.h"

namespace Server
{

/*
  The task of listener object is basic house keeping work w.r.t io_service
  and it's thread pool. Also to deligate all incomming requests to connectionManager.
 */
class Listener : public std::enable_shared_from_this<Listener>
{

public:
	Listener(size_t noofthreads, short port);
	void run();
	void stop();

private:
	void acceptNext();
	static void handleNext(std::weak_ptr<Listener> weakThis,
			       const boost::system::error_code& err);
	void exiting();
	std::weak_ptr<Listener> getWeak();

	//Member data
	boost::asio::io_service ios_;
	std::atomic<bool> running_;
	size_t no_of_threads_;
	std::shared_ptr<ConnectionManager> acceptor_;
	boost::asio::signal_set signals_;
	std::shared_ptr<boost::asio::io_service::work> fillerWork_;
};

} // namespace

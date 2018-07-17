#include <functional>
#include <thread>

#include <boost/bind.hpp>

#include "listener.h"

namespace Server
{

	Listener::Listener(size_t noofthread, short port)
		: ios_(),
		  running_(false),
		  no_of_threads_(noofthread),
		  acceptor_(new Server::ConnectionManager(ios_, port)),
		  signals_(ios_),
		  fillerWork_(new boost::asio::io_service::work(ios_))
	{
	}

	std::weak_ptr<Listener> Listener::getWeak()
	{
		return shared_from_this();
	}

	void Listener::acceptNext()
	{
		if (!acceptor_ || !running_.load())
		{
			return;
		}

		acceptor_->asyncAccept(std::bind(&Listener::handleNext,
						 getWeak(),
						 std::placeholders::_1));
	}

	void Listener::handleNext(std::weak_ptr<Listener> weakThis,
				  const boost::system::error_code& err)
	{
		std::shared_ptr<Listener> sharedThis = weakThis.lock();
		if (!sharedThis)
		{
			return;
		}

		if (err)
		{
			std::cout << "Error accepting connection. error code: "
				  << err
				  << " .Move on, accept other requests" << std::endl;
		}

		sharedThis->acceptNext();
	}

	void Listener::run()
	{
		//register signals for the listener.CTRL+C will stop the server
		signals_.add(SIGINT);
		signals_.add(SIGTERM);
		signals_.async_wait(std::bind(&Listener::exiting, shared_from_this()));

		//start up the listener.
		acceptor_->start();
		running_.store(true);
		std::cout << "listener running..." << std::endl;
		std::cout << "-------------------" << std::endl;
		//accept any incomming connections.
		acceptNext();

		std::vector<std::shared_ptr<std::thread>> threads;

		//this placement is needed for std::bind to resolve ambiguity between run(err) & run()
		size_t (boost::asio::io_service::*run)() = &boost::asio::io_service::run;

		//add 'no_of_threads' to the thread pool.
		for(size_t i = 0; i < no_of_threads_; ++i)
		{
			std::shared_ptr<std::thread> t(new std::thread(std::bind(run,
										 &ios_)));
			if (t)
			{
				threads.push_back(t);
			}
		}

		//Wait for all threads to complete the work.
		for(size_t i = 0; i < threads.size(); ++i)
		{
			threads[i]->join();
		}
	}

	void Listener::stop()
	{
		running_.store(false);
	}

	void Listener::exiting()
	{
		std::cout << "-------------------" << std::endl;
		std::cout << "server quitting..." <<std::endl;
		running_ = false;
		//clear up work object, io_service will stop working when no work.
		fillerWork_.reset();
		ios_.stop();
	}

} // namespace Server

#include <algorithm>
#include <thread>
#include <iomanip>

#include <boost/bind.hpp>

#include "connection.h"

uint32_t Server::ClientConn::WAIT_SECONDS = 120;

namespace Server
{
	//All requests on this connections are stranded. Thread safe.
	ClientConn::ClientConn(boost::asio::io_service& ios)
		:  socket_(ios),
		   strand_(ios),
		   connection_timer_(ios)
	{
	}

	ClientConn::~ClientConn()
	{
		connection_timer_.cancel();
		if (resetCtr_)
		{
			resetCtr_();
		}
	}

	//
	void ClientConn::start(ResetCtr&& cb)
	{
		resetCtr_ = std::move(cb);

		//start the time.
		connection_timer_.expires_from_now(std::chrono::seconds(WAIT_SECONDS));
		connection_timer_.async_wait(strand_.wrap(std::bind(&ClientConn::handleTimer,
								   getWeak(),
								   std::placeholders::_1)));

		socket_.async_read_some(boost::asio::buffer(data_),
					strand_.wrap(std::bind(&ClientConn::handleRead,
							       shared_from_this(),
							       std::placeholders::_1 ,
							       std::placeholders::_2 )));
	}

	std::weak_ptr<ClientConn> ClientConn::getWeak()
	{
		return shared_from_this();
	}

	void ClientConn::shutdownConn()
	{
		boost::system::error_code nothing;
		socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both,
				 nothing);
	}

	void ClientConn::handleTimer(std::weak_ptr<ClientConn> weakthis,
				     const boost::system::error_code& ec)
	{
		std::shared_ptr<ClientConn> SharedThis = weakthis.lock();
		if (SharedThis)
		{
			if (!ec)
			{
				//timeout expired shutdown the connection
				SharedThis->shutdownConn();
			}
		}
	}

	const std::string ClientConn::getTypeString(uint16_t type)
	{
		std::string strval;
		switch(type)
		{
		case TYPE_HELLO:
			strval = "HELLO";
			break;
		case TYPE_DATA:
			strval = "DATA";
			break;
		case TYPE_GOODBYE:
			strval = "GOODBYE";
			break;
		}
		return strval;
	}

	const std::string ClientConn::hexDump(uint16_t type, uint8_t* buffer, uint32_t length)
	{
		std::stringstream ss;
		ss << "["
		   << socket_.remote_endpoint().address().to_string()
		   << ":"
		   << socket_.remote_endpoint().port()
		   << "]"
		   << " "
		   << "["
		   << std::hex
		   << std::setw(4)
		   << std::setfill('0')
		   << getTypeString(type)
		   << "]"
		   << " "
		   << "["
		   << length
		   << "]"
		   << "  ";

		if ((type == TYPE_DATA) && buffer && length > 0)
		{
			ss << "[ ";
			for(int i = 0; i < length ; i++)
			{
				ss << "0x"
				   << std::uppercase
				   << std::setfill('0')
				   << std::setw(2)
				   << std::hex
				   << unsigned(buffer[i]) << " ";
			}
			ss << "]";
		}

		return ss.str();
	}

	void ClientConn::handleRead(const boost::system::error_code& err, size_t bytesTransferred)
	{
		if (err)
		{
			//dont queue up any thing, will go out of scope and connection
			//closes.
			return;
		}
		
		uint32_t pos = 0;
		bool success_response = false;
		bool quit_loop = true;

		while(1)
		{
			tlv* tlv_data = reinterpret_cast<tlv*>(data_.data()+pos);
			uint16_t transfer_type = ntohs(tlv_data->type);
			uint32_t length = ntohl(tlv_data->length);
			quit_loop = true;

			switch(transfer_type)
			{
			        case TYPE_HELLO:
			        case TYPE_GOODBYE:
					std::cout << hexDump(transfer_type, nullptr, 0) 
						  << std::endl;

					success_response = true;
					quit_loop = false;
					pos += sizeof(tlv::type) + sizeof(tlv::length);
					break;
				
			        case TYPE_DATA:
				{
					//length range checks. Kill the connection if client is misbehaving, gone rogue.					
					if (length > ACCEPTABLE_LENGTH)
					{
						std::cout << "invalid data closing connection, socket: "
							  << socket_.native_handle()
							  << std::endl;
						success_response = false;
						break;
					}

					if (length > 0)
					{																			       										
						std::cout << hexDump(transfer_type, 
								     data_.data()+(pos+sizeof(tlv::type)+sizeof(tlv::length)), 
								     length) 
							  << std::endl;
						success_response = true;
						quit_loop = false;
						pos += sizeof(tlv::type) + sizeof(tlv::length) + length;
					}
					break;
				}
			};

			if (quit_loop)
			{
				break;
			}
		
		} //while
		
		/*
		  Send a simple text response back to the client in case of acceptable request.
		*/
		if (success_response)
		{
			//valid requests, reset the timer and start again.
			connection_timer_.cancel();
			connection_timer_.expires_from_now(std::chrono::seconds(WAIT_SECONDS));
			connection_timer_.async_wait(strand_.wrap(std::bind(&ClientConn::handleTimer,
									    getWeak(),
									    std::placeholders::_1)));
			
			std::string strresult = "success";
			std::fill(std::begin(data_), std::end(data_), 0);
			std::copy(strresult.begin(), strresult.end(), data_.data());
			
			//post the result back to the client.
			boost::asio::async_write(socket_,
						 boost::asio::buffer(data_),
						 strand_.wrap(std::bind(&ClientConn::handleWrite,
									shared_from_this(),
									std::placeholders::_1 /*error*/,
									std::placeholders::_2 /*bytes trans*/)));
		}
		else
		{
			//if a request failed, kill the connection.
			shutdownConn();
		}
	}

	void ClientConn::handleWrite(const boost::system::error_code& err, size_t bytesTransferred)
	{
		if (!err)
		{
			//when write completes, post another read, to see if we get more.
			socket_.async_read_some(boost::asio::buffer(data_),
					strand_.wrap(std::bind(&ClientConn::handleRead,
							       shared_from_this(),
							       std::placeholders::_1 ,
							       std::placeholders::_2 )));
		}
		else
		{
			std::cout << "write err:" << err << std::endl;
			shutdownConn();
		}		
	}

	boost::asio::ip::tcp::socket& ClientConn::socket()
	{
		return socket_;
	}

} // namespace

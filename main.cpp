#include <iostream>
#include <thread>

#include <boost/program_options.hpp>

#include "listener.h"

#define DEFAULT_PORT 9080

int main(int argc, char** argv)
{
       try
       {
	       uint16_t port = 0;

	       //Parse the command line to retrieve port number for the listener to run on
	       //two options are supported, help & port.
	       //if port number is not passed, the listener assumes 9080 as default port.
	       boost::program_options::options_description desc("Allowed options");
	       desc.add_options()
		       ("help,h", "print usage message")
		       ("port,p", boost::program_options::value(&port), "port number server runs on")
		       ;

	       boost::program_options::variables_map vm;
	       store(boost::program_options::parse_command_line(argc, argv, desc), vm);

	       if (vm.count("help"))
	       {
		       std::cout << desc << std::endl;
		       return 0;
	       }

	       //if port is not specified server binds to 9080
	       if (!vm["port"].empty())
	       {
		       port = vm["port"].as<unsigned short>();
	       }
	       else
	       {
		       port = DEFAULT_PORT;
	       }

	       // Calculate how many thread we can afford to spawn.
	       unsigned int cores = std::thread::hardware_concurrency();

	       if(cores == 0)
	       {
		       cores = 5;
	       }

	       std::cout << "starting listener on port : " << port << std::endl;

	       std::shared_ptr<Server::Listener> listener(new Server::Listener(cores + 2, port));
	       listener->run();

       }
       catch(std::exception& e)
       {
	       std::cerr << e.what() << std::endl;
       }
       return 0;
}

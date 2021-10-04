#include <iostream>
#include <thread>

#include "rpc/server.h"
#include "rpc/client.h"

namespace calculator {
	float add(float first, float second) {
		return first + second;
	}
	float sub(float first, float second) {
		return first - second;
	}
	float mul(float first, float second) {
		return first * second;
	}
	float div(float first, float second) {
		return first / second;
	}
}

class Tree
{
public:
	float height;
	float radius;
	std::size_t age;
	std::size_t leaves;

	//stub function for determining the list of types of class
	int creator(float, float, std::size_t, std::size_t) {
		return 0;
	}

	int print()
	{
		std::cout << "Height is " << height << std::endl;
		std::cout << "Radius is " << radius << std::endl;
		std::cout << "Age is " << age << std::endl;
		std::cout << "Number of leaves is " << leaves << std::endl;
		return 0;
	}
};

int main()
{
	net::initializeSockets();

	//test example
	auto server_part = []()
	{
		rpc::server server({ "127.0.0.1", rpc::default_port }, // port can be set to an arbitrary one
			std::pair{ "+", calculator::add },
			std::pair{ "-", calculator::sub },
			std::pair{ "*", calculator::mul },
			std::pair{ "/", calculator::div }
		);

		server.register_type("Tree", &Tree::creator);
		server.register_method("Tree::print", &Tree::print);

		server.run();
	};

	std::thread server_thread(server_part);

	std::string ip;
	std::cout << "Enter ip address of the remote server, please: ";
	std::cin >> ip;
	std::fflush(stdin);

	rpc::client client({ ip.c_str(), rpc::default_port });

	client.create_object("Tree", "tree", 10.0f, 2.0f, 50, 30000);
	misc::buffer result_buffer = client.call_method("Tree::print", "tree");
	if (result_buffer.is_empty()) {
		std::cout << "Something happened with buffer\n";
		return -1;
	}
	int result = result_buffer.cast<int>();

	server_thread.join();

}
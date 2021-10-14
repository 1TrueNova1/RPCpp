#include <iostream>
#include <thread>

#include "rpc/server.h"
#include "rpc/client.h"


void first_lab();
void second_lab();

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

	std::tuple<char, char, char> do_tuple(std::tuple<char, char, char> tuple)
	{
		return std::tuple<char, char, char>{std::get<0>(tuple) + 1, std::get<1>(tuple) + 1, std::get<2>(tuple) + 1 };
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

struct password_cracker
{
	std::string begin;
	std::string end;

	int creator(std::string, std::string)
	{
		return 0;
	}

	std::string guessSHA()
	{
		return begin + end;
	}
};

std::string vector_sum(std::string s1, std::string s2)
{
	//std::vector<int> sum = v;
	//for (std::size_t i = 0; i < v.size(); ++i) {
		//sum[i] += 5;
	//}
	return s1 + s2;
}

int main()
{
	net::initializeSockets();
	//test example
	auto server_part = []()
	{
		rpc::server server({ "127.0.0.1", rpc::default_port }, // port can be set to an arbitrary one
			std::pair{ "add", calculator::add },
			std::pair{ "sum", vector_sum},
			std::pair{ "t", calculator::do_tuple }
		);

		server.register_type("PC", &password_cracker::creator);
		server.register_method("PC::guessSHA", &password_cracker::guessSHA);

		server.run();
	};


	auto client_part = []() {
		std::string ip;
		std::cout << "Enter ip address of the remote server, please: ";
		std::cin >> ip;
		std::fflush(stdin);

		rpc::client client;
		auto server = client.connect({ ip.c_str(), rpc::default_port });
		auto pc_h = client.create_object(server, "PC", "pc", std::string("Hello"), std::string(", world"));

		misc::buffer result_buffer = client.call_method(server, "PC::guessSHA", pc_h);
		//std::vector<int> vector{1, 3, 5, 7};
		//std::string s1{"Hello, "};
		//std::string s2;
		//std::cin >> s2;

		//misc::buffer result_buffer = client.call_function(server, "sum", s1, s2);
		if (result_buffer.is_empty()) {
			std::cout << "Something happened with buffer\n";
			return;
		}
		std::string result = result_buffer.cast<std::string>();
		//std::cout << "Result: " << std::get<0>(result) << std::get<1>(result);
		std::cout << "Result: ";
		//for (int n : result) std::cout << n << " ";
		std::cout << result;
		std::cout << std::endl;
	};

	std::thread th(server_part);
	client_part();
	th.join();

	//misc::buffer<> buffer(net::kilobyte);
	//std::vector<int> v{1, 5, 10};
	//std::string s("123");
	//char* arr = new char[10];
	//buffer.add(3.5, 5, arr, 10, 10);
	//buffer.add(5);
	//buffer.add(3, v, 3.5, s);
}


void first_lab()
{
	//test example
	auto server_part = []()
	{
		rpc::server server({ "127.0.0.1", rpc::default_port }, // port can be set to an arbitrary one
			std::pair{ "+", calculator::add },
			std::pair{ "-", calculator::sub },
			std::pair{ "*", calculator::mul },
			std::pair{ "/", calculator::div }
		);
		server.run();
	};
	std::thread server_thread(server_part);
	server_thread.join();

	auto client_part = []() {
		std::string ip;
		std::cout << "Enter ip address of the remote server, please: ";
		std::cin >> ip;
		std::fflush(stdin);
		//rpc::client client(net::address<net::IPv::IPv4>{ ip.c_str(), rpc::default_port });

		std::string function;
		std::cout << "Please enter the operation to execute: ";
		std::cin >> function;
		float n1;
		float n2;
		std::cout << "Please enter the first number: ";
		std::cin >> n1;
		std::cout << "Please enter the second number: ";
		std::cin >> n2;

		//misc::buffer<> result_buffer = client.call_function(function, n1, n2);
		//if (result_buffer.is_empty()) {
		//	std::cout << "Something happened with buffer\n";
		//	return;
		//}
		//float result = result_buffer.cast<float>();
		//std::cout << "Result: " << result;
	};
	//client_part();
}


void second_lab()
{

}
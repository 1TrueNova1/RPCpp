#pragma once

#include "../networking/socket.h"
#include "../networking/miscellaneous.h"

#include "rpc/miscellaneous.h"

namespace rpc {

	class client
	{
	private:
		net::socket<net::protocols::TCP> socket = {};
	public:
		client(const net::address<net::IPv::IPv4>& server_address)
		{
			connect(server_address);
		}

		bool connect(const net::address<net::IPv::IPv4>& server_address)
		{
			if (!socket.create(server_address))
				return false;
			return socket.connect();
		}

		template<typename ...Args>
		misc::buffer<> call_function(const std::string_view func_name, const Args&... args)
		{
			return call_function(std::hash<std::string_view>{}(func_name), args...);
		}

		template<typename ...Args>
		misc::buffer<> call_function(const id_t func_id, const Args&... args)
		{
			misc::buffer<> buffer = form_packet(opcodes::call_function, func_id, args...);

			if (!socket.send(buffer.data(), buffer.size())) {
				std::cout << "Something happened while sending the call to the server\n";
				int error_code = WSAGetLastError();
				std::cout << "Error code: " << error_code;
				return misc::buffer<>();
			}
			buffer.clear();
			return receive_and_return(buffer);
		}

		template<typename ...Args>
		misc::buffer<> create_object(std::string_view type_name, std::string_view object_name, const Args&... args)
		{
			std::hash<std::string_view> hash{};
			return create_object(hash(type_name), hash(object_name), args...);
		}
		template<typename ...Args>
		misc::buffer<> create_object(id_t type_id, id_t object_id, const Args&... args)
		{
			misc::buffer<> buffer = form_packet(opcodes::create_object, type_id, object_id, args...);
			
			if (!socket.send(buffer.data(), buffer.size())) {
				std::cout << "Something happened while sending the call to the server\n";
				int error_code = WSAGetLastError();
				std::cout << "Error code: " << error_code;
				return misc::buffer<>();
			}
			buffer.clear();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			return buffer;
		}
		template<typename ...Args>
		misc::buffer<> call_method(std::string_view method_name, std::string_view object_name, const Args&... args)
		{
			std::hash<std::string_view> hash{};
			return call_method(hash(method_name), hash(object_name), args...);
		}
		template<typename ...Args>
		misc::buffer<> call_method(id_t method_id, id_t object_id, const Args&... args)
		{
			misc::buffer<> buffer = form_packet(opcodes::call_method, method_id, object_id, args...);

			if (!socket.send(buffer.data(), buffer.size())) {
				std::cout << "Something happened while sending the call to the server\n";
				int error_code = WSAGetLastError();
				std::cout << "Error code: " << error_code;
				return misc::buffer<>();
			}
			buffer.clear();
			return receive_and_return(buffer);
		}

		misc::buffer<> receive_and_return(misc::buffer<>& buffer)
		{
			auto return_value = socket.receive(buffer.data_nc(), buffer.capacity());
			if (return_value == 0) {
				std::cout << "0 bytes received\n";
				int error_code = WSAGetLastError();
				std::cout << "Error code: " << error_code;
				return misc::buffer<>();
			}
			else if (return_value < 0) {
				std::cout << "Some error happened while recieve data from server \n";
				int error_code = WSAGetLastError();
				std::cout << "Error code: " << error_code;
				return misc::buffer<>();
			}

			buffer.set_size(return_value);
			return buffer;
		}
	};
}
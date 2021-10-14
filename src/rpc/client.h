#pragma once


#include "../networking/socket.h"
#include "../networking/miscellaneous.h"


#include <limits>

namespace rpc
{
	template class net::socket<net::protocols::TCP>;

	class client
	{
	private:
		std::map<handle_t, net::socket<net::protocols::TCP>> sockets;
		error_t error = errors::no_error;
	public:

		client() {}

		handle_t connect(const net::address<net::IPv::IPv4>& server_address)
		{
			net::socket<net::protocols::TCP> socket = {};
			if (!socket.create(server_address))
				return null_handle;
			if (!socket.connect())
				return null_handle;

			static handle_t server_id = 0;

			sockets.emplace(server_id, socket);
			return server_id++;
		}

		template<typename ...Args>
		misc::buffer<> call_function(handle_t server_id, const std::string_view func_name, const Args&... args)
		{
			return call_function(server_id, std::hash<std::string_view>{}(func_name), args...);
		}

		template<typename ...Args>
		misc::buffer<> call_function(const std::string_view func_name, const Args&... args)
		{
			return call_function(static_cast<handle_t>(0), std::hash<std::string_view>{}(func_name), args...);
		}

		template<typename ...Args>
		misc::buffer<> call_function(handle_t server_id, const id_t func_id, const Args&... args)
		{
			misc::buffer<> buffer = form_packet(opcodes::call_function, func_id, args...);
			if (!sockets[server_id].send(buffer.data(), buffer.size())) {
				std::cout << "Something happened while sending the call to the server\n";
				int error_code = WSAGetLastError();
				std::cout << "Error code: " << error_code;
				error = errors::connection_failure;
				return misc::buffer<>();
			}
			buffer.clear();
			return receive_and_return(server_id, buffer);
		}

		template<typename ...Args>
		id_t create_object(handle_t server_id, std::string_view type_name, std::string_view object_name, const Args&... args)
		{
			std::hash<std::string_view> hash{};
			return create_object(server_id, hash(type_name), hash(object_name), args...);
		}

		template<typename ...Args>
		id_t create_object(std::string_view type_name, std::string_view object_name, const Args&... args)
		{
			std::hash<std::string_view> hash{};
			return create_object(static_cast<handle_t>(0), hash(type_name), hash(object_name), args...);
		}

		template<typename ...Args>
		id_t create_object(handle_t server_id, id_t type_id, id_t object_id, const Args&... args)
		{
			misc::buffer<> buffer = form_packet(opcodes::create_object, type_id, object_id, args...);
			
			if (!sockets[server_id].send(buffer.data(), buffer.size())) {
				std::cout << "Something happened while sending the call to the server\n";
				int error_code = WSAGetLastError();
				std::cout << "Error code: " << error_code;
				error = errors::connection_failure;
				return null_id;
			}
			buffer.clear();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));

			return object_id;
		}

		template<typename ...Args>
		misc::buffer<> call_method(handle_t server_id, std::string_view method_name, std::string_view object_name, const Args&... args)
		{
			std::hash<std::string_view> hash{};
			return call_method(server_id, hash(method_name), hash(object_name), args...);
		}
		template<typename ...Args>
		misc::buffer<> call_method(handle_t server_id, std::string_view method_name, id_t object_id, const Args&... args)
		{
			return call_method(server_id, std::hash<std::string_view>{}(method_name), object_id, args...);
		}
		template<typename ...Args>
		misc::buffer<> call_method(std::string_view method_name, std::string_view object_name, const Args&... args)
		{
			std::hash<std::string_view> hash{};
			return call_method(static_cast<handle_t>(0), hash(method_name), hash(object_name), args...);
		}
		template<typename ...Args>
		misc::buffer<> call_method(std::string_view method_name, id_t object_id, const Args&... args)
		{
			return call_method(static_cast<handle_t>(0), std::hash<std::string_view>{}(method_name), object_id, args...);
		}

		template<typename ...Args>
		misc::buffer<> call_method(handle_t server_id, id_t method_id, id_t object_id, const Args&... args)
		{
			misc::buffer<> buffer = form_packet(opcodes::call_method, method_id, object_id, args...);

			if (!sockets[server_id].send(buffer.data(), buffer.size())) {
				std::cout << "Something happened while sending the call to the server\n";
				int error_code = WSAGetLastError();
				std::cout << "Error code: " << error_code;
				error = errors::connection_failure;
				return misc::buffer<>();
			}
			buffer.clear();
			return receive_and_return(server_id, buffer);
		}

		misc::buffer<> receive_and_return(handle_t server_id, misc::buffer<>& buffer)
		{
			auto return_value = sockets[server_id].receive(buffer.data_nc(), buffer.capacity());
			if (return_value == 0) {
				std::cout << "0 bytes received\n";
				int error_code = WSAGetLastError();
				std::cout << "Error code: " << error_code;
				error = errors::connection_failure;
				return misc::buffer<>();
			}
			else if (return_value < 0) {
				std::cout << "Some error happened while recieve data from server \n";
				int error_code = WSAGetLastError();
				std::cout << "Error code: " << error_code;
				error = errors::connection_failure;
				return misc::buffer<>();
			}

			buffer.set_size(return_value);
			return buffer;
		}


		error_t get_error() const { return error; }
		error_t null_error() { error = errors::no_error; }
	};
}
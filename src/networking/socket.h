#pragma once

#include <iostream>
#include <type_traits>

#include "networking.h"
#include "address.h"

namespace net {
	enum class protocols : uint8_t {
		IP,
		UDP,
		TCP
	};

	template<typename T>
	class empty_base
	{
	public:
		empty_base() = default;

		template<typename... Args>
		constexpr empty_base(Args&& ...) {}
	};

	template<typename T>
	struct wrapper
	{
		T value;
	};

	template<bool condition, typename T>
	using optional_base = std::conditional_t<condition, T, empty_base<T>>;
	template<bool condition, typename T>
	using optional_base_w = std::conditional_t<condition, wrapper<T>, empty_base<T>>;

	struct server_socket_helper
	{
		sockaddr_in addr = { };
	};

	struct receiver_address_helper
	{
		sockaddr_in receiver_address = {};
	};

	template<protocols type, bool is_server_socket = false, bool static_address = true, typename Enable = void>
	class socket {};

	template<protocols type, bool is_server_socket, bool static_address>
	class socket<type, is_server_socket, static_address, typename std::enable_if<type == protocols::UDP>::type>
		: public optional_base<is_server_socket == true, server_socket_helper>,
		public optional_base<static_address == true, receiver_address_helper>
	{
		using server_part = optional_base<is_server_socket == true, server_socket_helper>;
		using address_part = optional_base<static_address == true, receiver_address_helper>;

		socket_t fd;

	public:
		socket()
		{
		}

		socket(const address<IPv::IPv4>& address)
		{
			create(address);
		}

		bool create(const address<IPv::IPv4>& address = {})
		{
			if ((fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) <= 0) {
				std::cout << "Failed to create a socket\n";
				return false;
			}
			if constexpr (static_address == true) {
				address_part::receiver_address = (sockaddr_in)address;
			}

			if constexpr (is_server_socket == true) {
				server_part::addr = (sockaddr_in)address;
				return bind();
			}
			else {
				return true;
			}
		}
	private:
		bool bind()
		{
			if constexpr (is_server_socket == true) {
				if (::bind(fd, (const sockaddr*)&(server_part::addr), sizeof sockaddr_in) < 0) {
					std::cout << "Failed to bind a socket\n";
					int error = WSAGetLastError();
					std::cout << "Error code: " << error << '\n';
					return false;
				}
			}
			return true;
		}
	public:

		bool send(const class address<IPv::IPv4>& receiver, const std::string& message, int flags = 0) const
		{
			return send(receiver, message.c_str(), message.size(), flags);
		}

		bool send(const class address<IPv::IPv4>& receiver, const void* message, size_t size, int flags = 0) const
		{
			sockaddr_in receiver_address = (sockaddr_in)receiver;

			int send_bytes = ::sendto(fd, (const char*)message, size, flags, (const sockaddr*)&receiver_address, sizeof(sockaddr_in));
			return send_bytes == size;
		}

		std::enable_if_t<static_address == true> set_address(const std::string& ip, uint16_t port)
		{
			address_part::receiver_address.sin_family = AF_INET;
			::inet_pton(AF_INET, ip.c_str(), &address_part::receiver_address.sin_addr.s_addr);
			address_part::receiver_address.sin_port = ntohs(port);
		}

		template<typename = std::enable_if_t<static_address == true>>
		bool send(const std::string& message, int flags = 0) const
		{
			return (bool)send(message.c_str(), message.size(), flags);
		}
		template<typename = std::enable_if_t<static_address == true>>
		bool send(const void* message, size_t size, int flags = 0) const
		{
			int send_bytes = ::sendto(fd, (const char*)message, size, flags, (const sockaddr*)&(address_part::receiver_address), sizeof(sockaddr_in));
			return send_bytes == size;
		}

		std::pair<address<IPv::IPv4>, int> receive(void* buffer, size_t buffer_size, int flags = 0) const
		{
			sockaddr_in sender_addr;
			socklen_t sender_length = sizeof(sockaddr_in);

			int received_bytes = ::recvfrom(fd, (char*)buffer, buffer_size, flags, (sockaddr*)&sender_addr, &sender_length);
			// sender's address + error_code if any
			return { sender_addr, received_bytes };
		}

		//specifies size of OS's internal buffer for TCP and UDP
		//that holds data arrived but not yet recv'ed
		bool set_recv_buffer_size(size_t buffer_size)
		{
			if (::setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char*)&buffer_size, buffer_size) == -1) {
				std::cout << "Setting custom receive buffer failed\n";
				return false;
			}
			return true;
		}
		bool set_non_blocking()
		{
#if PLATFORM == PLATFORM_UNIX || PLATFORM == PLATFORM_MAC
			int non_blocking = 1;
			if (::fcntl(fd, F_SETFL, O_NONBLOCK, non_blocking) == -1) {
				std::cout << "Failed to set non-blocking to socket\n";
				return false;
			}
		}
#elif PLATFORM == PLATFORM_WINDOWS
			DWORD non_blocking = 1;
			if (::ioctlsocket(fd, FIONBIO, &non_blocking) != 0) {
				std::cout << "Failed to set non-blocking to socket\n";
				return false;
			}
#endif
			return true;
		}

	public:

		void close()
		{
#if PLATFORM == PLATFORM_UNIX || PLATFORM == PLATFORM_MAC
			close(fd);
#elif PLATFORM == PLATFORM_WINDOWS
			closesocket(fd);
#endif
		}
	}; // class socket

	// TODO: finish TCP socket
	template<protocols type, bool is_server_socket, bool static_address>
	class socket<type, is_server_socket, static_address, typename std::enable_if<type == protocols::TCP>::type>
		: public optional_base<is_server_socket == true, server_socket_helper>
	{
		using server_part = optional_base<is_server_socket == true, server_socket_helper>;

		socket_t fd;
		socket_t client_socket;

		sockaddr_in receiver_address = {};

	public:
		socket()
		{
		}

		socket(const address<IPv::IPv4>& address)
		{
			create(address);
		}

		bool create(const address<IPv::IPv4>& address = {})
		{
			if ((fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) <= 0) {
				std::cout << "Failed to create a socket\n";
				return false;
			}

			receiver_address = (sockaddr_in) address;

			if constexpr (is_server_socket == true) {
				server_part::addr = (sockaddr_in) address;
				server_part::addr.sin_addr.s_addr = INADDR_ANY;
				if (!bind())
					return false;
				if (!listen())
					return false;
			}
			return true;
		}

	private:
		bool bind()
		{
			if constexpr (is_server_socket == true) {
				if (::bind(fd, (const sockaddr*)&(server_part::addr), sizeof sockaddr_in) == -1) {
					std::cout << "Failed to bind a socket\n";
					int error = WSAGetLastError();
					std::cout << "Error code: " << error << '\n';
					return false;
				}
			}
			return true;
		}
		bool listen()
		{
			if constexpr (is_server_socket == true) {
				if (::listen(fd, SOMAXCONN) != 0) {
					std::cout << "Failed to set listening to a socket\n";
					int error = WSAGetLastError();
					std::cout << "Error code: " << error << '\n';
					return false;
				}
			}
			return true;
		}
	public:
		bool accept()
		{
			sockaddr_in client_info;
			int client_info_length = sizeof client_info;

			client_socket = ::accept(fd, (struct sockaddr*)&client_info, &client_info_length);
			if (client_socket < 0) {
				std::cout << "Something happened while accepting a new client\n";
				int error = WSAGetLastError();
				std::cout << "Error code: " << error << '\n';
				return false;
			}
			std::cout << "New client accepted\n";
			return true;
		}

		bool connect()
		{
			if (::connect(fd, (const sockaddr*)&receiver_address, sizeof receiver_address) < 0) {
				std::cout << "Something happened while connecting to the server\n";
				int error = WSAGetLastError();
				std::cout << "Error code: " << error << '\n';
				return false;
			}
			//std::cout << "Connected to the server\n";
			return true;
		}

		bool send(const std::string& message, int flags = 0) const
		{
			return send(message.c_str(), message.size(), flags);
		}
		bool send(const void* message, size_t size, int flags = 0) const
		{
			int send_bytes;
			if constexpr (is_server_socket) {
				send_bytes = ::send(client_socket, (const char*)message, size, flags);
			}
			else {
				send_bytes = ::send(fd, (const char*)message, size, flags);
			}
			return send_bytes == size;
		}
		int receive(void* buffer, size_t buffer_size, int flags = 0) const
		{
			int received_bytes;
			if constexpr (is_server_socket) {
				received_bytes = ::recv(client_socket, (char*)buffer, buffer_size, flags);
			}
			else {
				received_bytes = ::recv(fd, (char*)buffer, buffer_size, flags);
			}
			return received_bytes;
		}

		//specifies size of OS's internal buffer for TCP and UDP
		//that holds data arrived but not yet recv'ed
		bool set_recv_buffer_size(size_t buffer_size)
		{
			if (::setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char*)&buffer_size, buffer_size) == -1) {
				std::cout << "Setting custom receive buffer failed\n";
				return false;
			}
			return true;
		}
		bool set_non_blocking()
		{
#if PLATFORM == PLATFORM_UNIX || PLATFORM == PLATFORM_MAC
			int non_blocking = 1;
			if (::fcntl(fd, F_SETFL, O_NONBLOCK, non_blocking) == -1) {
				std::cout << "Failed to set non-blocking to socket\n";
				return false;
			}
		}
#elif PLATFORM == PLATFORM_WINDOWS
			DWORD non_blocking = 1;
			if (::ioctlsocket(fd, FIONBIO, &non_blocking) != 0) {
				std::cout << "Failed to set non-blocking to socket\n";
				return false;
			}
#endif
			return true;
		}

	public:

		void close()
		{
#if PLATFORM == PLATFORM_UNIX || PLATFORM == PLATFORM_MAC
			close(fd);
#elif PLATFORM == PLATFORM_WINDOWS
			closesocket(fd);
#endif
		}
	}; // class socket

} // namespace Networking


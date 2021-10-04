#pragma once

#include <ws2tcpip.h>
#include <string>
#include <cassert>

#include "networking.h"

namespace net {
	namespace IPv {
		using IPv4 = IN_ADDR;
		using IPv6 = IN6_ADDR;
	}

	template<typename version = IPv::IPv4>
	class address {};

	template<>
	class address<IPv::IPv4>
	{
	private:
		//sockaddr_in m_address;
		in_addr m_ip;
		uint16_t m_port;

	public:
		address() : m_ip({ 0 }), m_port(0) {}
		address(const std::string& ip, uint16_t port, bool host_order = true) : m_port((host_order) ? ::htons(port) : port)
		{
			::inet_pton(AF_INET, ip.c_str(), &m_ip.s_addr);
			//if (!host_order)
				//m_ip.s_addr = ::htonl(m_ip.s_addr);
		}
		address(const char* ip, uint16_t port, bool host_order = true) : m_port((host_order) ? ::htons(port) : port)
		{
			::inet_pton(AF_INET, ip, &m_ip.s_addr);
		}
		address(const sockaddr_in& sockaddr)
			: m_ip(sockaddr.sin_addr), m_port(sockaddr.sin_port) {}

		int set_ip(const std::string& ip) { return ::inet_pton(AF_INET, ip.c_str(), &m_ip.s_addr); }
		int set_ip(const char* ip) { return ::inet_pton(AF_INET, ip, &m_ip.s_addr); }

		template<bool host_order = false>
		void set_ip(uint32_t ip)
		{
			if constexpr (host_order)
				m_ip.s_addr = ::htonl(ip);
			if constexpr (!host_order)
				m_ip.s_addr = ip;
		}
		template<bool host_order = false>
		void set_port(uint16_t port)
		{
			if constexpr (host_order)
				m_port = ::htonl(port);
			if constexpr (!host_order)
				m_port = port;
		}

		uint32_t ip() const { return m_ip.s_addr; }
		uint16_t port() const { return m_port; }

		uint8_t& operator[](uint8_t byte)
		{
			assert(byte < 3 || "IPv4 address has only 4 octets");
			return ((uint8_t*)(&m_ip.S_un.S_un_b))[byte];
		}
		const uint8_t& operator[](uint8_t byte) const
		{
			assert(byte < 3 || "IPv4 address has only 4 octets");
			return ((uint8_t*)(&m_ip.S_un.S_un_b))[byte];
		}

		bool operator==(const address& address) const {
			return this->m_ip.s_addr == address.m_ip.s_addr && this->m_port == address.m_port;
		}

		explicit operator sockaddr_in() const
		{
			return { AF_INET, m_port, m_ip };
		}
		explicit operator sockaddr* () const
		{
			sockaddr_in addr = { AF_INET, m_port, m_ip };
			return (sockaddr*)&addr;
		}
		explicit operator const sockaddr* () const
		{
			sockaddr_in addr = { AF_INET, m_port, m_ip };
			return (const sockaddr*)&addr;
		}
	};

	//add full support of IPv6 addresses
	template<>
	class address<IPv::IPv6>
	{
	private:
		in6_addr m_ip;
		uint16_t m_port;

	public:
		address() : m_ip({ 0 }), m_port(0) { }
		address(const std::string& ip, uint16_t port) : m_port(port)
		{
			::inet_pton(AF_INET6, ip.c_str(), &m_ip);
		}
		address(const char* ip, uint16_t port) : m_port(port)
		{
			::inet_pton(AF_INET6, ip, &m_ip);
		}

		inline int ip(const std::string& ip) { return ::inet_pton(AF_INET6, ip.c_str(), &m_ip); }
		inline int ip(const char* ip) { return ::inet_pton(AF_INET6, ip, &m_ip); }
		inline void ip(in6_addr ip) { m_ip = ip; }
		//add inline void ip(uint64_t ip1, ip2) { m_ip = ip; }
		inline void port(uint16_t port) { m_port = port; }

		inline in6_addr ip() const { return m_ip; }
		inline uint16_t port() const { return m_port; }

		uint8_t& operator[](uint8_t byte)
		{
			assert(byte < 15 || "IPv6 address has only 16 octets");
			return m_ip.u.Byte[byte];
		}
		const uint8_t& operator[](uint8_t byte) const
		{
			assert(byte < 15 || "IPv6 address has only 16 octets");
			return m_ip.u.Byte[byte];
		}

		explicit operator sockaddr_in6() const
		{
			return { AF_INET6, m_port, 0, m_ip, 0 };
		}
	};
}
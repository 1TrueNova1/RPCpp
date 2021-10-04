#pragma once

#include <cstdint>

#define PLATFORM_WINDOWS 1
#define PLATFORM_UNIX 2
#define PLATFORM_MAC 3

#if defined(_WIN32)
#define PLATFORM PLATFORM_WINDOWS
#elif defined(__APPLE__)
#define PLATFORM PLATFORM_MAC
#else
#define PLATFORM PLATFORM_UNIX
#endif

#if PLATFORM == PLATFORM_WINDOWS
#include <winsock2.h>
using socket_t = SOCKET;

#elif PLATFORM == PLATFORM_MAC || PLATFORM_UNIX
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
using socket_t = int;
#endif

namespace net
{
	constexpr uint64_t byte = 1;
	constexpr uint64_t kilobyte = 1024 * byte;
	constexpr uint64_t megabyte = 1024 * kilobyte;
	constexpr uint64_t gigabyte = 1024 * megabyte;
	constexpr uint64_t terabyte = 1024 * gigabyte;
	bool initializeSockets();
	int shutdownSockets();
}
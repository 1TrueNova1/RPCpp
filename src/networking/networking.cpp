#include "networking.h"


namespace net
{
	bool initializeSockets() {
#if PLATFORM == PLATFORM_WINDOWS
		WSADATA WsaData;
		return WSAStartup(MAKEWORD(2, 2), &WsaData) == NO_ERROR;
#else
		return true;
#endif
	}
	int shutdownSockets()
	{
#if PLATFORM == PLATFORM_WINDOWS
		return WSACleanup();
#else
		return 0;
#endif
	}
}
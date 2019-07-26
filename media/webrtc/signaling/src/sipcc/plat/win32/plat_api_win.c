






































#include "cpr_ipc.h"
#include "cpr_types.h"

#include "windows.h"

#include "dns_utils.h"
#include "phone_debug.h"
#include "util_string.h"

static char ip_address[16];

char* platGetIPAddr (void) {
	char szHostName[128] = "";
	struct sockaddr_in SocketAddress;
	struct hostent     *pHost        = 0;
	

	WSADATA WSAData;

	
	if(WSAStartup(MAKEWORD(1, 0), &WSAData))
	{
		return "";
	}

	
	if(gethostname(szHostName, sizeof(szHostName)))
	{
	  
	}

	pHost = gethostbyname(szHostName);
	if(!pHost)
	{
		return "";
	}

	



	memcpy(&SocketAddress.sin_addr, pHost->h_addr_list[0], pHost->h_length);
	strcpy(ip_address, inet_ntoa(SocketAddress.sin_addr));

	WSACleanup();
	return ip_address;
}

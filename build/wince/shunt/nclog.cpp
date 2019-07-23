











































#include "mozce_internal.h"

#define USE_NC_LOGGING

#ifdef USE_NC_LOGGING


#include "winsock.h"
#include <stdarg.h>
#include <stdio.h>

static SOCKET wsa_socket=INVALID_SOCKET;
#pragma comment(lib , "winsock")

static unsigned short theLogPort;


static bool wsa_bind(unsigned short port)
{
  SOCKADDR_IN addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  int r=bind(wsa_socket,(sockaddr*)&addr,sizeof(addr));
  if (r==0) theLogPort=port;
  return (r==0);
  
}


static bool wsa_init()
{
  if (wsa_socket != INVALID_SOCKET) return true;
  int r;
  WSADATA wd;
  BOOL bc=true;
  
  if (0 != WSAStartup(0x101, &wd)) 
  {
	  MessageBox(0, L"WSAStartup failed", L"ERROR", 0);
  	  goto error;
  }
  wsa_socket=socket(PF_INET, SOCK_DGRAM, 0);
  if (wsa_socket == INVALID_SOCKET)
  {
	  MessageBox(0, L"socket failed", L"ERROR", 0);
	  goto error;
  }
  r=setsockopt(wsa_socket, SOL_SOCKET, SO_BROADCAST, (char*)&bc,
               sizeof(bc));
  if (r!=0)
  {
	MessageBox(0, L"setsockopt failed", L"ERROR", 0);
	goto error;
  }

  if (wsa_bind(9998)) return true; 

  MessageBox(0, L"Can Not Bind To Port", L"ERROR", 0);

error:
  if (wsa_socket != INVALID_SOCKET) closesocket(wsa_socket);
  return false;

}


bool set_nclog_port(unsigned short x) { return wsa_bind(x); }

static void wsa_send(const char *x)
{
  SOCKADDR_IN sa;
  sa.sin_family = AF_INET;
  sa.sin_port = htons(theLogPort);
  sa.sin_addr.s_addr = htonl(INADDR_BROADCAST);
  
  sendto(wsa_socket, x, strlen(x), 0, (sockaddr*)&sa, sizeof(sa));
}


int nclog (const char *fmt, ...)
{
  va_list vl;
  va_start(vl,fmt);
  char buf[1024]; 
  sprintf(buf,fmt,vl);
  wsa_init();
  wsa_send(buf);
  return 0;
}

void nclograw(const char* data, long length)
{
  wsa_init();
  
  SOCKADDR_IN sa;
  sa.sin_family = AF_INET;
  sa.sin_port = htons(theLogPort);
  sa.sin_addr.s_addr = htonl(INADDR_BROADCAST);
  
  sendto(wsa_socket, data, length, 0, (sockaddr*)&sa, sizeof(sa));
}


struct _nclog_module
{
  ~_nclog_module()
  {
    if (wsa_socket!=INVALID_SOCKET)
    {
      nclog("nclog goes down\n");
      shutdown(wsa_socket,2);
      closesocket(wsa_socket);
    }
  }
  
};

static _nclog_module module; 


#endif

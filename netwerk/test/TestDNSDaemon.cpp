




































#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>

#if defined(AIX) || defined(__linux)
#include <sys/select.h>         
#endif

#if defined(__linux)

#define getdtablehi() FD_SETSIZE
#elif !defined(__irix)

#define getdtablehi() getdtablesize()




#endif


#include "nspr.h"
#include "nsCRT.h"
#include "unix_dns.h"

struct sockaddr_un  unix_addr;

int async_dns_lookup(char* hostName)
{
  fprintf(stderr, "start async_dns_lookup\n");
  int socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
  if (socket_fd == -1) {
    fprintf(stderr, "socket returned error.\n");
    return -1;
  }

  unix_addr.sun_family = AF_UNIX;
  strcpy(unix_addr.sun_path, DNS_SOCK_NAME);

  int err = connect(socket_fd,(struct sockaddr*)&unix_addr, sizeof(unix_addr));
  if (err == -1) {
    fprintf(stderr, "connect failed (errno = %d).\n",errno);
    close(socket_fd);
    return -1;
  }

  char buf[256];
  strcpy(buf, "lookup: ");
  strcpy(&buf[8], hostName);

  err = send(socket_fd, buf, strlen(buf)+1, 0);
  if (err < 0)
    fprintf(stderr, "send(%s) returned error (errno=%d).\n",buf, errno);

  
  err = recv(socket_fd, buf, 256, 0);
  if (err < 0)
    fprintf(stderr, "recv() returned error (errno=%d).\n", errno);
  else
    {
      
      int id = *(int *)buf;
      fprintf(stderr, "id: %d\n", id);
    }

  return socket_fd;
}

static char *
string_trim(char *s)
{
  char *s2;
  if (!s) return 0;
  s2 = s + strlen(s) - 1;
  while (s2 > s && (*s2 == '\n' || *s2 == '\r' || *s2 == ' ' || *s2 == '\t'))
    *s2-- = 0;
  while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r')
    s++;
  return s;
}

hostent *
bytesToHostent(char *buf)
{
  int i;
  
  int len, aliasCount, addressCount;
  int addrtype, addrlength;
  char* p = buf;
  char s[1024];

  len = *(int *)p;            
  p += sizeof(int);           

  memcpy(s, p, len); s[len] = 0;
  fprintf(stderr, "hostname: %s\n", s);

  p += len;                   
  aliasCount  = *(int *)p;    
  p += sizeof(int);           

  for (i=0; i<aliasCount; i++) {
    len = *(int *)p;          
    p += sizeof(int);         

    memcpy(s, p, len); s[len] = 0;
    fprintf(stderr, "alias: %s\n", s);

    p += len;                 
  }

  addrtype = *(int *)p;

  fprintf(stderr, "addrtype: %d\n", addrtype);

  p += sizeof(int);
  addrlength = *(int *)p;

  fprintf(stderr, "addrlength: %d\n", addrlength);

  p += sizeof(int);
  addressCount = *(int *)p;
  p += sizeof(int);

  for (i=0; i<addressCount; i++) {
    len = *(int *)p;    
    p += sizeof(int);

    fprintf(stderr, "addr len: %d\n", len);
    fprintf(stderr, "addr    : %x\n", *(int *)p);

    p += len;
  }

  
  
  return 0;
}

int
main(int argc, char* argv[])
{
  PRStatus status;

  
  printf("### launch daemon...\n");

  PRProcessAttr *attributes = PR_NewProcessAttr();
  if (attributes == nsnull) {
    printf("PR_NewProcessAttr() failed.\n");
    return -1;
  }

  PRProcess *daemon = PR_CreateProcess("nsDnsAsyncLookup", nsnull, nsnull, attributes);
  if (daemon == nsnull) {
    printf("PR_CreateProcess failed.\n");
  } else {
    
    
    
    
  }

  PR_DestroyProcessAttr(attributes);

  
  int socket_fd = 0;


  PRBool notDone = PR_TRUE;
  char buf[1024];

  while(notDone) {
    int status = 0;
    fd_set fdset;

    FD_ZERO(&fdset);
    FD_SET(fileno(stdin), &fdset);
    if (socket_fd > 0)
      FD_SET(socket_fd, &fdset);
	   
    status = select(getdtablehi(), &fdset, 0, 0, 0);
    if (status <= 0)
      {
	fprintf(stderr, "%s: select() returned %d\n", argv[0], status);
	exit(-1);
      }

    

    if (FD_ISSET(fileno(stdin), &fdset))
      {
	char *line = fgets(buf, sizeof(buf)-1, stdin);
	line = string_trim(line);
	
	if(!strcmp(line, "quit") || !strcmp(line, "exit"))
	  {
	    fprintf(stderr, "bye now.\n");
	    notDone = PR_FALSE;
	  }
	else if (!strncmp(line, "abort ", 6))
	  {
	    
	  }
	else if (strchr(line, ' ') || strchr(line, '\t'))
	  {
	    fprintf(stderr, "%s: unrecognized command %s.\n", argv[0], line);
	  }
	else
	  {
	    fprintf(stderr, "%s: looking up %s...\n", argv[0], line);
	    
	    socket_fd = async_dns_lookup(line);
	  }
      }

    if (socket_fd && FD_ISSET(socket_fd, &fdset))
      {
	
	int size = read(socket_fd, buf, 1024);
	if (size > 0)
	  {
	    
	    char *p = buf;
	    fprintf(stderr, "bytes read: %d\n", size);
	    fprintf(stderr, "response code: %d\n", *(int *)p);
	    p += sizeof(int);

	    for (int i=0; i < size; i++) {
	      if (!(i%8))
		fprintf(stderr, "\n");
	      fprintf(stderr, "%2.2x ",(unsigned char)buf[i]);
	    }
	    fprintf(stderr, "\n");
	    hostent *h;
	    h = bytesToHostent(p);
	  }
	close(socket_fd);
	socket_fd = 0;
      }
  }

  return 0;
}
























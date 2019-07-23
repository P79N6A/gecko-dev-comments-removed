















































#include <assert.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <algorithm>
#include "prinit.h"
#include "prerror.h"
#include "prio.h"
#include "prnetdb.h"
#include "prtpool.h"
#include "prtypes.h"
#include "nss.h"
#include "pk11func.h"
#include "key.h"
#include "keyt.h"
#include "ssl.h"
#include "plhash.h"

using std::string;
using std::vector;

enum client_auth_option {
  caNone = 0,
  caRequire = 1,
  caRequest = 2
};


typedef struct {
  PRInt32 listen_port;
  string cert_nickname;
  PLHashTable* host_cert_table;
  PLHashTable* host_clientauth_table;
} server_info_t;

typedef struct {
  PRFileDesc* client_sock;
  PRNetAddr client_addr;
  server_info_t* server_info;
} connection_info_t;

const PRInt32 BUF_SIZE = 16384;
const PRInt32 BUF_MARGIN = 1024;
const PRInt32 BUF_TOTAL = BUF_SIZE + BUF_MARGIN;

struct relayBuffer
{
  char *buffer, *bufferhead, *buffertail, *bufferend;

  relayBuffer()
  {
    
    bufferhead = buffertail = buffer = new char[BUF_TOTAL];
    bufferend = buffer + BUF_SIZE;
  }

  ~relayBuffer()
  {
    delete [] buffer;
  }

  void compact() {
    if (buffertail == bufferhead)
      buffertail = bufferhead = buffer;
  }

  bool empty() { return bufferhead == buffertail; }
  size_t free() { return bufferend - buffertail; }
  size_t margin() { return free() + BUF_MARGIN; }
  size_t present() { return buffertail - bufferhead; }
};


class AutoCert {
public:
  AutoCert(CERTCertificate* cert) { cert_ = cert; }
  ~AutoCert() { if (cert_) CERT_DestroyCertificate(cert_); }
  operator CERTCertificate*() { return cert_; }
private:
  CERTCertificate* cert_;
};

class AutoKey {
public:
  AutoKey(SECKEYPrivateKey* key) { key_ = key; }
  ~AutoKey() { if (key_)   SECKEY_DestroyPrivateKey(key_); }
  operator SECKEYPrivateKey*() { return key_; }
private:
  SECKEYPrivateKey* key_;
};

class AutoFD {
public:
  AutoFD(PRFileDesc* fd) { fd_ = fd; }
  ~AutoFD() {
    if (fd_) {
      PR_Shutdown(fd_, PR_SHUTDOWN_BOTH);
      PR_Close(fd_);
    }
  }
  operator PRFileDesc*() { return fd_; }
  PRFileDesc* reset(PRFileDesc* newfd) {
    PRFileDesc* oldfd = fd_;
    fd_ = newfd;
    return oldfd;
  }
private:
  PRFileDesc* fd_;
};



const PRUint32 INITIAL_THREADS = 1;
const PRUint32 MAX_THREADS = 5;
const PRUint32 DEFAULT_STACKSIZE = (512 * 1024);


string nssconfigdir;
vector<server_info_t> servers;
PRNetAddr remote_addr;
PRThreadPool* threads = NULL;
PRLock* shutdown_lock = NULL;
PRCondVar* shutdown_condvar = NULL;

bool shutdown_server = false;
bool do_http_proxy = false;
bool any_host_spec_config = false;

PR_CALLBACK PRIntn ClientAuthValueComparator(const void *v1, const void *v2)
{
  int a = *static_cast<const client_auth_option*>(v1) -
          *static_cast<const client_auth_option*>(v2);
  if (a == 0)
    return 0;
  if (a > 0)
    return 1;
  else 
    return -1;
}




void SignalShutdown()
{
  PR_Lock(shutdown_lock);
  PR_NotifyCondVar(shutdown_condvar);
  PR_Unlock(shutdown_lock);
}

bool ReadConnectRequest(server_info_t* server_info, 
    relayBuffer& buffer, PRInt32* result, string& certificate,
    client_auth_option* clientauth, string& host)
{
  if (buffer.present() < 4)
    return false;
  if (strncmp(buffer.buffertail-4, "\r\n\r\n", 4))
    return false;

  *result = 400;

  char* token;
  token = strtok(buffer.bufferhead, " ");
  if (!token) 
    return true;
  if (strcmp(token, "CONNECT")) 
    return true;

  token = strtok(NULL, " ");
  void* c = PL_HashTableLookup(server_info->host_cert_table, token);
  if (c)
    certificate = static_cast<char*>(c);

  host = "https://";
  host += token;

  c = PL_HashTableLookup(server_info->host_clientauth_table, token);
  if (c)
    *clientauth = *static_cast<client_auth_option*>(c);
  else
    *clientauth = caNone;

  token = strtok(NULL, "/");
  if (strcmp(token, "HTTP"))
    return true;

  *result = 200;
  return true;
}

bool ConfigureSSLServerSocket(PRFileDesc* socket, server_info_t* si, string &certificate, client_auth_option clientAuth)
{
  const char* certnick = certificate.empty() ?
      si->cert_nickname.c_str() : certificate.c_str();

  AutoCert cert(PK11_FindCertFromNickname(
      certnick, NULL));
  if (!cert) {
    fprintf(stderr, "Failed to find cert %s\n", certnick);
    return false;
  }

  AutoKey privKey(PK11_FindKeyByAnyCert(cert, NULL));
  if (!privKey) {
    fprintf(stderr, "Failed to find private key\n");
    return false;
  }

  PRFileDesc* ssl_socket = SSL_ImportFD(NULL, socket);
  if (!ssl_socket) {
    fprintf(stderr, "Error importing SSL socket\n");
    return false;
  }

  SSLKEAType certKEA = NSS_FindCertKEAType(cert);
  if (SSL_ConfigSecureServer(ssl_socket, cert, privKey, certKEA)
      != SECSuccess) {
    fprintf(stderr, "Error configuring SSL server socket\n");
    return false;
  }

  SSL_OptionSet(ssl_socket, SSL_SECURITY, PR_TRUE);
  SSL_OptionSet(ssl_socket, SSL_HANDSHAKE_AS_CLIENT, PR_FALSE);
  SSL_OptionSet(ssl_socket, SSL_HANDSHAKE_AS_SERVER, PR_TRUE);

  if (clientAuth != caNone)
  {
    SSL_OptionSet(ssl_socket, SSL_REQUEST_CERTIFICATE, PR_TRUE);
    SSL_OptionSet(ssl_socket, SSL_REQUIRE_CERTIFICATE, clientAuth == caRequire);
  }

  SSL_ResetHandshake(ssl_socket, PR_TRUE);

  return true;
}





bool AdjustRequestURI(relayBuffer& buffer, string *host)
{
  assert(buffer.margin());

  
  
  buffer.buffertail[1] = '\0';

  char *token, *path;
  path = strchr(buffer.bufferhead, ' ') + 1;
  if (!path)
    return false;

  
  
  if (*path != '/')
    return true;

  token = strchr(path, ' ') + 1;
  if (!token)
    return false;

  if (strncmp(token, "HTTP/", 5))
    return false;

  size_t hostlength = host->length();
  assert(hostlength <= buffer.margin());

  memmove(path + hostlength, path, buffer.buffertail - path);
  memcpy(path, host->c_str(), hostlength);
  buffer.buffertail += hostlength;

  return true;
}

bool ConnectSocket(PRFileDesc *fd, const PRNetAddr *addr, PRIntervalTime timeout)
{
  PRStatus stat = PR_Connect(fd, addr, timeout);
  if (stat != PR_SUCCESS)
    return false;

  PRSocketOptionData option;
  option.option = PR_SockOpt_Nonblocking;
  option.value.non_blocking = PR_TRUE;
  PR_SetSocketOption(fd, &option);

  return true;
}








void HandleConnection(void* data)
{
  connection_info_t* ci = static_cast<connection_info_t*>(data);
  PRIntervalTime connect_timeout = PR_SecondsToInterval(2);

  AutoFD other_sock(PR_NewTCPSocket());
  bool client_done = false;
  bool client_error = false;
  bool connect_accepted = !do_http_proxy;
  bool ssl_updated = !do_http_proxy;
  bool expect_request_start = do_http_proxy;
  string certificateToUse;
  client_auth_option clientAuth;
  string fullHost;

  if (other_sock) 
  {
    PRInt32 numberOfSockets = 1;

    relayBuffer buffers[2];

    if (!do_http_proxy)
    {
      if (!ConfigureSSLServerSocket(ci->client_sock, ci->server_info, certificateToUse, caNone))
        client_error = true;
      else if (!ConnectSocket(other_sock, &remote_addr, connect_timeout))
        client_error = true;
      else
        numberOfSockets = 2;
    }

    PRPollDesc sockets[2] = 
    { 
      {ci->client_sock, PR_POLL_READ, 0},
      {other_sock, PR_POLL_READ, 0}
    };
    PRBool socketErrorState[2] = {PR_FALSE, PR_FALSE};

    while (!((client_error||client_done) && buffers[0].empty() && buffers[1].empty()))
    {
      sockets[0].in_flags |= PR_POLL_EXCEPT;
      sockets[1].in_flags |= PR_POLL_EXCEPT;
      PRInt32 pollStatus = PR_Poll(sockets, numberOfSockets, PR_MillisecondsToInterval(1000));
      if (pollStatus < 0)
      {
        client_error = true;
        break;
      }

      if (pollStatus == 0)
        
        continue;

      for (PRInt32 s = 0; s < numberOfSockets; ++s)
      {
        PRInt32 s2 = s == 1 ? 0 : 1;
        PRInt16 out_flags = sockets[s].out_flags;
        PRInt16 &in_flags = sockets[s].in_flags;
        PRInt16 &in_flags2 = sockets[s2].in_flags;
        sockets[s].out_flags = 0;

        if (out_flags & (PR_POLL_EXCEPT | PR_POLL_ERR | PR_POLL_HUP))
        {
          client_error = true;
          socketErrorState[s] = PR_TRUE;
          
          
          
          buffers[s2].bufferhead = buffers[s2].buffertail = buffers[s2].buffer;
          continue;
        } 

        if (out_flags & PR_POLL_READ && buffers[s].free())
        {
          PRInt32 bytesRead = PR_Recv(sockets[s].fd, buffers[s].buffertail, 
              buffers[s].free(), 0, PR_INTERVAL_NO_TIMEOUT);

          if (bytesRead == 0)
          {
            client_done = true;
            in_flags &= ~PR_POLL_READ;
          }
          else if (bytesRead < 0)
          {
            if (PR_GetError() != PR_WOULD_BLOCK_ERROR)
            {
              
              
              client_error = true;
              socketErrorState[s] = PR_TRUE;
              
              buffers[s2].bufferhead = buffers[s2].buffertail = buffers[s2].buffer;
            }
          }
          else
          {
            
            
            if (socketErrorState[s2])
              continue;

            buffers[s].buffertail += bytesRead;

            
            PRInt32 response;
            if (!connect_accepted && ReadConnectRequest(ci->server_info, buffers[s],
                &response, certificateToUse, &clientAuth, fullHost))
            {
              
              buffers[s].bufferhead = buffers[s].buffertail = buffers[s].buffer;

              
              if (response != 200)
              {
                client_done = true;
                sprintf(buffers[s2].buffer, "HTTP/1.1 %d ERROR\r\nConnection: close\r\n\r\n", response);
                buffers[s2].buffertail = buffers[s2].buffer + strlen(buffers[s2].buffer);
                break;
              }

              strcpy(buffers[s2].buffer, "HTTP/1.1 200 Connected\r\nConnection: keep-alive\r\n\r\n");
              buffers[s2].buffertail = buffers[s2].buffer + strlen(buffers[s2].buffer);

              if (!ConnectSocket(other_sock, &remote_addr, connect_timeout))
              {
                client_error = true;
                break;
              }

              
              in_flags |= PR_POLL_WRITE;
              connect_accepted = true;
              break;
            } 

            if (!buffers[s].free()) 
              in_flags &= ~PR_POLL_READ;

            if (ssl_updated)
            {
              if (s == 0 && expect_request_start)
                expect_request_start = !AdjustRequestURI(buffers[s], &fullHost);

              in_flags2 |= PR_POLL_WRITE;
            }
          }
        } 

        if (out_flags & PR_POLL_WRITE)
        {
          PRInt32 bytesWrite = PR_Send(sockets[s].fd, buffers[s2].bufferhead, 
              buffers[s2].present(), 0, PR_INTERVAL_NO_TIMEOUT);

          if (bytesWrite < 0)
          {
            if (PR_GetError() != PR_WOULD_BLOCK_ERROR) {
              client_error = true;
              socketErrorState[s] = PR_TRUE;
              
              
              buffers[s2].bufferhead = buffers[s2].buffertail = buffers[s2].buffer;
            }
          }
          else
          {
            buffers[s2].bufferhead += bytesWrite;
            if (buffers[s2].present())
              in_flags |= PR_POLL_WRITE;              
            else
            {
              if (!ssl_updated)
              {
                
                ssl_updated = true;
                if (!ConfigureSSLServerSocket(ci->client_sock, ci->server_info, certificateToUse, clientAuth))
                {
                  client_error = true;
                  break;
                }

                numberOfSockets = 2;
              } 

              in_flags &= ~PR_POLL_WRITE;              
              in_flags2 |= PR_POLL_READ;
              buffers[s2].compact();
            }
          }
        } 
      } 
    } 
  }
  else
    client_error = true;

  if (!client_error)
    PR_Shutdown(ci->client_sock, PR_SHUTDOWN_SEND);
  PR_Close(ci->client_sock);

  delete ci;
}







void StartServer(void* data)
{
  server_info_t* si = static_cast<server_info_t*>(data);

  
  AutoFD listen_socket(PR_NewTCPSocket());
  if (!listen_socket) {
    fprintf(stderr, "failed to create socket\n");
    SignalShutdown();
    return;
  }

  PRNetAddr server_addr;
  PR_InitializeNetAddr(PR_IpAddrAny, si->listen_port, &server_addr);
  if (PR_Bind(listen_socket, &server_addr) != PR_SUCCESS) {
    fprintf(stderr, "failed to bind socket\n");
    SignalShutdown();
    return;
  }

  if (PR_Listen(listen_socket, 1) != PR_SUCCESS) {
    fprintf(stderr, "failed to listen on socket\n");
    SignalShutdown();
    return;
  }

  printf("Server listening on port %d with cert %s\n", si->listen_port,
         si->cert_nickname.c_str());

  while (!shutdown_server) {
    connection_info_t* ci = new connection_info_t();
    ci->server_info = si;
    
    ci->client_sock = PR_Accept(listen_socket, &ci->client_addr,
                                PR_INTERVAL_NO_TIMEOUT);
    
    PRSocketOptionData option;
    option.option = PR_SockOpt_Nonblocking;
    option.value.non_blocking = PR_TRUE;
    PR_SetSocketOption(ci->client_sock, &option);

    if (ci->client_sock)
      
      
      PR_QueueJob(threads, HandleConnection, ci, PR_TRUE);
    else
      delete ci;
  }
}


char* password_func(PK11SlotInfo* slot, PRBool retry, void* arg)
{
  if (retry)
    return NULL;

  return PL_strdup("");
}

server_info_t* findServerInfo(int portnumber)
{
  for (vector<server_info_t>::iterator it = servers.begin();
       it != servers.end(); it++) 
  {
    if (it->listen_port == portnumber)
      return &(*it);
  }

  return NULL;
}

int processConfigLine(char* configLine)
{
  if (*configLine == 0 || *configLine == '#')
    return 0;

  char* keyword = strtok(configLine, ":");

  
  if (!strcmp(keyword, "httpproxy"))
  {
    char* value = strtok(NULL, ":");
    if (!strcmp(value, "1"))
      do_http_proxy = true;

    return 0;
  }

  
  if (!strcmp(keyword, "forward"))
  {
    char* ipstring = strtok(NULL, ":");
    if (PR_StringToNetAddr(ipstring, &remote_addr) != PR_SUCCESS) {
      fprintf(stderr, "Invalid remote IP address: %s\n", ipstring);
      return 1;
    }
    char* serverportstring = strtok(NULL, ":");
    int port = atoi(serverportstring);
    if (port <= 0) {
      fprintf(stderr, "Invalid remote port: %s\n", serverportstring);
      return 1;
    }
    remote_addr.inet.port = PR_htons(port);

    return 0;
  }

  
  if (!strcmp(keyword, "listen"))
  {
    char* hostname = strtok(NULL, ":");
    char* hostportstring = NULL;
    if (strcmp(hostname, "*"))
    {
      any_host_spec_config = true;
      hostportstring = strtok(NULL, ":");
    }

    char* serverportstring = strtok(NULL, ":");
    char* certnick = strtok(NULL, ":");

    int port = atoi(serverportstring);
    if (port <= 0) {
      fprintf(stderr, "Invalid port specified: %s\n", serverportstring);
      return 1;
    }

    if (server_info_t* existingServer = findServerInfo(port))
    {
      char *certnick_copy = new char[strlen(certnick)+1];
      char *hostname_copy = new char[strlen(hostname)+strlen(hostportstring)+2];

      strcpy(hostname_copy, hostname);
      strcat(hostname_copy, ":");
      strcat(hostname_copy, hostportstring);
      strcpy(certnick_copy, certnick);

      PLHashEntry* entry = PL_HashTableAdd(existingServer->host_cert_table, hostname_copy, certnick_copy);
      if (!entry) {
        fprintf(stderr, "Out of memory");
        return 1;
      }
    }
    else
    {
      server_info_t server;
      server.cert_nickname = certnick;
      server.listen_port = port;
      server.host_cert_table = PL_NewHashTable(0, PL_HashString, PL_CompareStrings, PL_CompareStrings, NULL, NULL);
      if (!server.host_cert_table)
      {
        fprintf(stderr, "Internal, could not create hash table\n");
        return 1;
      }
      server.host_clientauth_table = PL_NewHashTable(0, PL_HashString, PL_CompareStrings, ClientAuthValueComparator, NULL, NULL);
      if (!server.host_clientauth_table)
      {
        fprintf(stderr, "Internal, could not create hash table\n");
        return 1;
      }
      servers.push_back(server);
    }

    return 0;
  }
  
  if (!strcmp(keyword, "clientauth"))
  {
    char* hostname = strtok(NULL, ":");
    char* hostportstring = strtok(NULL, ":");
    char* serverportstring = strtok(NULL, ":");

    int port = atoi(serverportstring);
    if (port <= 0) {
      fprintf(stderr, "Invalid port specified: %s\n", serverportstring);
      return 1;
    }

    if (server_info_t* existingServer = findServerInfo(port))
    {
      char* authoptionstring = strtok(NULL, ":");
      client_auth_option* authoption = new client_auth_option;
      if (!authoption) {
        fprintf(stderr, "Out of memory");
        return 1;
      }

      if (!strcmp(authoptionstring, "require"))
        *authoption = caRequire;
      else if (!strcmp(authoptionstring, "request"))
        *authoption = caRequest;
      else if (!strcmp(authoptionstring, "none"))
        *authoption = caNone;
      else
      {
        fprintf(stderr, "Incorrect client auth option modifier for host '%s'", hostname);
        return 1;
      }

      any_host_spec_config = true;

      char *hostname_copy = new char[strlen(hostname)+strlen(hostportstring)+2];
      if (!hostname_copy) {
        fprintf(stderr, "Out of memory");
        return 1;
      }

      strcpy(hostname_copy, hostname);
      strcat(hostname_copy, ":");
      strcat(hostname_copy, hostportstring);

      PLHashEntry* entry = PL_HashTableAdd(existingServer->host_clientauth_table, hostname_copy, authoption);
      if (!entry) {
        fprintf(stderr, "Out of memory");
        return 1;
      }
    }
    else
    {
      fprintf(stderr, "Server on port %d for client authentication option is not defined, use 'listen' option first", port);
      return 1;
    }

    return 0;
  }

  
  if (!strcmp(keyword, "certdbdir"))
  {
    nssconfigdir = strtok(NULL, "\n");
    return 0;
  }

  printf("Error: keyword \"%s\" unexpected\n", keyword);
  return 1;
}

int parseConfigFile(const char* filePath)
{
  FILE* f = fopen(filePath, "r");
  if (!f)
    return 1;

  char buffer[1024], *b = buffer;
  while (!feof(f))
  {
    char c;
    fscanf(f, "%c", &c);
    switch (c)
    {
    case '\n':
      *b++ = 0;
      if (processConfigLine(buffer))
        return 1;
      b = buffer;
    case '\r':
      continue;
    default:
      *b++ = c;
    }
  }

  fclose(f);

  
  if (nssconfigdir.empty())
  {
    printf("Error: missing path to NSS certification database\n,use certdbdir:<path> in the config file\n");
    return 1;
  }

  if (any_host_spec_config && !do_http_proxy)
  {
    printf("Warning: any host-specific configurations are ignored, add httpproxy:1 to allow them\n");
  }

  return 0;
}

PRIntn freeHostCertHashItems(PLHashEntry *he, PRIntn i, void *arg)
{
  delete [] (char*)he->key;
  delete [] (char*)he->value;
  return HT_ENUMERATE_REMOVE;
}

PRIntn freeClientAuthHashItems(PLHashEntry *he, PRIntn i, void *arg)
{
  delete [] (char*)he->key;
  delete (client_auth_option*)he->value;
  return HT_ENUMERATE_REMOVE;
}

int main(int argc, char** argv)
{
  const char* configFilePath;
  if (argc == 1)
    configFilePath = "ssltunnel.cfg";
  else
    configFilePath = argv[1];

  if (parseConfigFile(configFilePath)) {
    fprintf(stderr, "Error: config file \"%s\" missing or formating incorrect\n"
      "Specify path to the config file as parameter to ssltunnel or \n"
      "create ssltunnel.cfg in the working directory.\n\n"
      "Example format of the config file:\n\n"
      "       # Enable http/ssl tunneling proxy-like behavior.\n"
      "       # If not specified ssltunnel simply does direct forward.\n"
      "       httpproxy:1\n\n"
      "       # Specify path to the certification database used.\n"
      "       certdbdir:/path/to/certdb\n\n"
      "       # Forward/proxy all requests in raw to 127.0.0.1:8888.\n"
      "       forward:127.0.0.1:8888\n\n"
      "       # Accept connections on port 4443 or 5678 resp. and authenticate\n"
      "       # to any host ('*') using the 'server cert' or 'server cert 2' resp.\n"
      "       listen:*:4443:server cert\n"
      "       listen:*:5678:server cert 2\n\n"
      "       # Accept connections on port 4443 and authenticate using\n"
      "       # 'a different cert' when target host is 'my.host.name:443'.\n"
      "       # This only works in httpproxy mode and has higher priority\n"
      "       # than the previous option.\n"
      "       listen:my.host.name:443:4443:a different cert\n\n"
      "       # To make a specific host require or just request a client certificate\n"
      "       # to authenticate use the following options. This can only be used\n"
      "       # in httpproxy mode and only after the 'listen' option has been\n"
      "       # specified. You also have to specify the tunnel listen port.\n"
      "       clientauth:requesting-client-cert.host.com:443:4443:request\n"
      "       clientauth:requiring-client-cert.host.com:443:4443:require\n",
      configFilePath);
    return 1;
  }

  
  threads = PR_CreateThreadPool(PR_MAX(INITIAL_THREADS, servers.size()*2),
                                PR_MAX(MAX_THREADS, servers.size()*2),
                                DEFAULT_STACKSIZE);
  if (!threads) {
    fprintf(stderr, "Failed to create thread pool\n");
    return 1;
  }

  shutdown_lock = PR_NewLock();
  if (!shutdown_lock) {
    fprintf(stderr, "Failed to create lock\n");
    PR_ShutdownThreadPool(threads);
    return 1;
  }
  shutdown_condvar = PR_NewCondVar(shutdown_lock);
  if (!shutdown_condvar) {
    fprintf(stderr, "Failed to create condvar\n");
    PR_ShutdownThreadPool(threads);
    PR_DestroyLock(shutdown_lock);
    return 1;
  }

  PK11_SetPasswordFunc(password_func);

  
  if (NSS_Init(nssconfigdir.c_str()) != SECSuccess) {
    PRInt32 errorlen = PR_GetErrorTextLength();
    char* err = new char[errorlen+1];
    PR_GetErrorText(err);
    fprintf(stderr, "Failed to init NSS: %s", err);
    delete[] err;
    PR_ShutdownThreadPool(threads);
    PR_DestroyCondVar(shutdown_condvar);
    PR_DestroyLock(shutdown_lock);
    return 1;
  }

  if (NSS_SetDomesticPolicy() != SECSuccess) {
    fprintf(stderr, "NSS_SetDomesticPolicy failed\n");
    PR_ShutdownThreadPool(threads);
    PR_DestroyCondVar(shutdown_condvar);
    PR_DestroyLock(shutdown_lock);
    NSS_Shutdown();
    return 1;
  }

  
  if (SSL_ConfigServerSessionIDCache(0, 0, 0, NULL) != SECSuccess) {
    fprintf(stderr, "SSL_ConfigServerSessionIDCache failed\n");
    PR_ShutdownThreadPool(threads);
    PR_DestroyCondVar(shutdown_condvar);
    PR_DestroyLock(shutdown_lock);
    NSS_Shutdown();
    return 1;
  }

  for (vector<server_info_t>::iterator it = servers.begin();
       it != servers.end(); it++) {
    
    
    PR_QueueJob(threads, StartServer, &(*it), PR_TRUE);
  }
  
  PR_Lock(shutdown_lock);
  PR_WaitCondVar(shutdown_condvar, PR_INTERVAL_NO_TIMEOUT);
  PR_Unlock(shutdown_lock);
  shutdown_server = true;
  printf("Shutting down...\n");
  
  PR_ShutdownThreadPool(threads);
  PR_JoinThreadPool(threads);
  PR_DestroyCondVar(shutdown_condvar);
  PR_DestroyLock(shutdown_lock);
  if (NSS_Shutdown() == SECFailure) {
    fprintf(stderr, "Leaked NSS objects!\n");
  }
  
  for (vector<server_info_t>::iterator it = servers.begin();
       it != servers.end(); it++) 
  {
    PL_HashTableEnumerateEntries(it->host_cert_table, freeHostCertHashItems, NULL);
    PL_HashTableEnumerateEntries(it->host_clientauth_table, freeClientAuthHashItems, NULL);
    PL_HashTableDestroy(it->host_cert_table);
    PL_HashTableDestroy(it->host_clientauth_table);
  }

  PR_Cleanup();
  return 0;
}

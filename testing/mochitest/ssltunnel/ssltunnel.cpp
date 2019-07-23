





































#include <stdio.h>
#include <string>
#include <vector>
#include <algorithm>
#include "prinit.h"
#include "prerror.h"
#include "prio.h"
#include "prnetdb.h"
#include "prtpool.h"
#include "nss.h"
#include "pk11func.h"
#include "key.h"
#include "keyt.h"
#include "ssl.h"

using std::string;
using std::vector;


typedef struct {
  PRInt32 listen_port;
  PRNetAddr remote_addr;
  string cert_nickname;
} server_info_t;

typedef struct {
  PRFileDesc* client_sock;
  PRNetAddr client_addr;
  server_info_t* server_info;
} connection_info_t;


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



const PRInt32 INITIAL_THREADS = 1;
const PRInt32 MAX_THREADS = 5;
const PRInt32 DEFAULT_STACKSIZE = (512 * 1024);
const PRInt32 BUF_SIZE = 4096;


PRThreadPool* threads = NULL;
PRLock* shutdown_lock = NULL;
PRCondVar* shutdown_condvar = NULL;

bool shutdown_server = false;




void SignalShutdown()
{
  PR_Lock(shutdown_lock);
  PR_NotifyCondVar(shutdown_condvar);
  PR_Unlock(shutdown_lock);
}








void PR_CALLBACK HandleConnection(void* data)
{
  connection_info_t* ci = static_cast<connection_info_t*>(data);
  PRIntervalTime connect_timeout = PR_SecondsToInterval(2);
  PRIntervalTime short_timeout = PR_MillisecondsToInterval(250);

  AutoFD other_sock(PR_NewTCPSocket());
  bool client_done = false;
  bool client_error = false;
  PRUint8 buf[BUF_SIZE];

  if (other_sock &&
      PR_Connect(other_sock, &ci->server_info->remote_addr, connect_timeout)
      == PR_SUCCESS) {
    PRInt32 bytes = PR_Recv(ci->client_sock, buf, BUF_SIZE, 0, short_timeout);
    if (bytes > 0 &&
        PR_Send(other_sock, buf, bytes, 0, short_timeout) > 0) {
      bytes = PR_Recv(other_sock, buf, BUF_SIZE, 0, short_timeout);
      while (bytes > 0) {
        if (PR_Send(ci->client_sock, buf, bytes, 0, short_timeout) == -1) {
          client_error = true;
          break;
        }
        if (!client_done) {
          bytes = PR_Recv(ci->client_sock, buf, BUF_SIZE, 0, short_timeout);
          if (bytes > 0) {
            if (PR_Send(other_sock, buf, bytes, 0, short_timeout) == -1)
              break;
          }
          else if (bytes == 0) {
            client_done = true;
          }
          else  {
            client_error = true;
            break;
          }
        }
        bytes = PR_Recv(other_sock, buf, BUF_SIZE, 0, short_timeout);
      }
    }
    else if (bytes == -1) {
      client_error = true;
    }
  }
  if (!client_error)
    PR_Shutdown(ci->client_sock, PR_SHUTDOWN_BOTH);
  PR_Close(ci->client_sock);

  delete ci;
}







void PR_CALLBACK StartServer(void* data)
{
  server_info_t* si = static_cast<server_info_t*>(data);

  
  AutoCert cert(PK11_FindCertFromNickname(si->cert_nickname.c_str(),
                                          NULL));
  if (!cert) {
    fprintf(stderr, "Failed to find cert %s\n", si->cert_nickname.c_str());
    SignalShutdown();
    return;
  }

  AutoKey privKey(PK11_FindKeyByAnyCert(cert, NULL));
  if (!privKey) {
    fprintf(stderr, "Failed to find private key\n");
    SignalShutdown();
    return;
  }

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

  PRFileDesc* ssl_socket = SSL_ImportFD(NULL, listen_socket);
  if (!ssl_socket) {
    fprintf(stderr, "Error importing SSL socket\n");
    SignalShutdown();
    return;
  }
  listen_socket.reset(ssl_socket);

  if (SSL_ConfigSecureServer(listen_socket, cert, privKey, kt_rsa)
      != SECSuccess) {
    fprintf(stderr, "Error configuring SSL listen socket\n");
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

  return "";
}

int main(int argc, char** argv)
{
  if (argc < 6) {
    fprintf(stderr, "Error: not enough arguments\n"
            "Usage: ssltunnel <NSS db path> <remote ip> <remote port> (<certname> <port>)+\n"
            "       Provide SSL encrypted tunnels to <remote ip>:<remote port>\n"
            "       from each port specified in a <certname>,<port> pair.\n"
            "       <certname> must be the nickname of a server certificate\n"
            "       installed in the NSS db pointed to by the <NSS db path>.\n");
    return 1;
  }


  PRNetAddr remote_addr;
  if (PR_StringToNetAddr(argv[2], &remote_addr) != PR_SUCCESS) {
    fprintf(stderr, "Invalid remote IP address: %s\n", argv[2]);
    return 1;
  }

  int port = atoi(argv[3]);
  if (port <= 0) {
    fprintf(stderr, "Invalid remote port: %s\n", argv[2]);
    return 1;
  }
  remote_addr.inet.port = PR_htons(port);

  
  vector<server_info_t> servers;
  for (int i=4; i<argc; i++) {
    server_info_t server;
    memcpy(&server.remote_addr, &remote_addr, sizeof(PRNetAddr));
    server.cert_nickname = argv[i++];
    port = atoi(argv[i]);
    if (port <= 0) {
      fprintf(stderr, "Invalid port specified: %s\n", argv[i]);
      return 1;
    }
    server.listen_port = port;
    servers.push_back(server);
  }

  
  threads = PR_CreateThreadPool(std::max<PRInt32>(INITIAL_THREADS,
                                                  servers.size()*2),
                                std::max<PRInt32>(MAX_THREADS,
                                                  servers.size()*2),
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

  
  char* configdir = argv[1];

  if (NSS_Init(configdir) != SECSuccess) {
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
  PR_Cleanup();
  return 0;
}

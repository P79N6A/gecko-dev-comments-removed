



#ifndef SSLSAMPLE_H
#define SSLSAMPLE_H



#include <stdio.h>
#include <string.h>



#include "nspr.h"
#include "prerror.h"
#include "prnetdb.h"



#include "pk11func.h"
#include "secitem.h"
#include "ssl.h"
#include "certt.h"
#include "nss.h"
#include "secder.h"
#include "key.h"
#include "sslproto.h"







#define BUFFER_SIZE 10240



extern int cipherSuites[];
extern int ssl2CipherSuites[];
extern int ssl3CipherSuites[];


typedef struct DataBufferStr {
	char data[BUFFER_SIZE];
	int  index;
	int  remaining;
	int  dataStart;
	int  dataEnd;
} DataBuffer;



char * myPasswd(PK11SlotInfo *info, PRBool retry, void *arg);

SECStatus myAuthCertificate(void *arg, PRFileDesc *socket,
                            PRBool checksig, PRBool isServer);

SECStatus myBadCertHandler(void *arg, PRFileDesc *socket);

void myHandshakeCallback(PRFileDesc *socket, void *arg);

SECStatus myGetClientAuthData(void *arg, PRFileDesc *socket,
                              struct CERTDistNamesStr *caNames,
                              struct CERTCertificateStr **pRetCert,
                              struct SECKEYPrivateKeyStr **pRetKey);



void disableAllSSLCiphers(void);




void errWarn(char *function);

void exitErr(char *function);

void printSecurityInfo(FILE *outfile, PRFileDesc *fd);



#define MAX_THREADS 32

typedef SECStatus startFn(void *a, int b);

typedef enum { rs_idle = 0, rs_running = 1, rs_zombie = 2 } runState;

typedef struct perThreadStr {
	PRFileDesc *a;
	int         b;
	int         rv;
	startFn    *startFunc;
	PRThread   *prThread;
	PRBool      inUse;
	runState    running;
} perThread;

typedef struct GlobalThreadMgrStr {
	PRLock	  *threadLock;
	PRCondVar *threadStartQ;
	PRCondVar *threadEndQ;
	perThread  threads[MAX_THREADS];
	int        index;
	int        numUsed;
	int        numRunning;
} GlobalThreadMgr;

void thread_wrapper(void * arg);

SECStatus launch_thread(GlobalThreadMgr *threadMGR, 
                        startFn *startFunc, void *a, int b);

SECStatus reap_threads(GlobalThreadMgr *threadMGR);

void destroy_thread_data(GlobalThreadMgr *threadMGR);



struct lockedVarsStr {
	PRLock *    lock;
	int         count;
	int         waiters;
	PRCondVar * condVar;
};

typedef struct lockedVarsStr lockedVars;

void lockedVars_Init(lockedVars *lv);

void lockedVars_Destroy(lockedVars *lv);

void lockedVars_WaitForDone(lockedVars *lv);

int lockedVars_AddToCount(lockedVars *lv, int addend);



static const char stopCmd[] = { "GET /stop " };
static const char defaultHeader[] = {
	"HTTP/1.0 200 OK\r\n"
	"Server: SSL sample server\r\n"
	"Content-type: text/plain\r\n"
	"\r\n"
};

#endif

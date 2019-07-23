






































 

#include <stdio.h>
#include <string.h>

#if defined(XP_UNIX)
#include <unistd.h>
#endif

#include "prerror.h"

#include "pk11func.h"
#include "secitem.h"


#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>

#include "nspr.h"
#include "plgetopt.h"
#include "prio.h"
#include "prnetdb.h"
#include "nss.h"

#include "sslsample.h"

#define RD_BUF_SIZE (60 * 1024)

extern int ssl2CipherSuites[];
extern int ssl3CipherSuites[];

GlobalThreadMgr threadMGR;
char *certNickname = NULL;
char *hostName = NULL;
char *password = NULL;
unsigned short port = 0;

static void
Usage(const char *progName)
{
	fprintf(stderr, 
	  "Usage: %s [-n rsa_nickname] [-p port] [-d dbdir] [-c connections]\n"
	  "          [-w dbpasswd] [-C cipher(s)] hostname\n",
	progName);
	exit(1);
}

PRFileDesc *
setupSSLSocket(PRNetAddr *addr)
{
	PRFileDesc         *tcpSocket;
	PRFileDesc         *sslSocket;
	PRSocketOptionData	socketOption;
	PRStatus            prStatus;
	SECStatus           secStatus;

#if 0
retry:
#endif

	tcpSocket = PR_NewTCPSocket();
	if (tcpSocket == NULL) {
		errWarn("PR_NewTCPSocket");
	}

	
	socketOption.option	            = PR_SockOpt_Nonblocking;
	socketOption.value.non_blocking = PR_FALSE;

	prStatus = PR_SetSocketOption(tcpSocket, &socketOption);
	if (prStatus != PR_SUCCESS) {
		errWarn("PR_SetSocketOption");
		goto loser;
	} 

#if 0
	
	prStatus = PR_Connect(tcpSocket, addr, PR_INTERVAL_NO_TIMEOUT);
	if (prStatus != PR_SUCCESS) {
		PRErrorCode err = PR_GetError();
		if (err == PR_CONNECT_REFUSED_ERROR) {
			PR_Close(tcpSocket);
			PR_Sleep(PR_MillisecondsToInterval(10));
			fprintf(stderr, "Connection to port refused, retrying.\n");
			goto retry;
		}
		errWarn("PR_Connect");
		goto loser;
	}
#endif

	
	sslSocket = SSL_ImportFD(NULL, tcpSocket);
	if (!sslSocket) {
		errWarn("SSL_ImportFD");
		goto loser;
	}

	
	secStatus = SSL_OptionSet(sslSocket, SSL_SECURITY, PR_TRUE);
	if (secStatus != SECSuccess) {
		errWarn("SSL_OptionSet:SSL_SECURITY");
		goto loser;
	}

	secStatus = SSL_OptionSet(sslSocket, SSL_HANDSHAKE_AS_CLIENT, PR_TRUE);
	if (secStatus != SECSuccess) {
		errWarn("SSL_OptionSet:SSL_HANDSHAKE_AS_CLIENT");
		goto loser;
	}

	
	secStatus = SSL_GetClientAuthDataHook(sslSocket,
	                          (SSLGetClientAuthData)myGetClientAuthData,
	                          (void *)certNickname);
	if (secStatus != SECSuccess) {
		errWarn("SSL_GetClientAuthDataHook");
		goto loser;
	}

	secStatus = SSL_AuthCertificateHook(sslSocket,
	                                   (SSLAuthCertificate)myAuthCertificate,
                                       (void *)CERT_GetDefaultCertDB());
	if (secStatus != SECSuccess) {
		errWarn("SSL_AuthCertificateHook");
		goto loser;
	}

	secStatus = SSL_BadCertHook(sslSocket, 
	                           (SSLBadCertHandler)myBadCertHandler, NULL);
	if (secStatus != SECSuccess) {
		errWarn("SSL_BadCertHook");
		goto loser;
	}

	secStatus = SSL_HandshakeCallback(sslSocket, 
	                                  myHandshakeCallback,
	                                  NULL);
	if (secStatus != SECSuccess) {
		errWarn("SSL_HandshakeCallback");
		goto loser;
	}

	return sslSocket;

loser:

	PR_Close(tcpSocket);
	return NULL;
}


const char requestString[] = {"GET /testfile HTTP/1.0\r\n\r\n" };

SECStatus
handle_connection(PRFileDesc *sslSocket, int connection)
{
	int	     countRead = 0;
	PRInt32  numBytes;
	char    *readBuffer;

	readBuffer = PORT_Alloc(RD_BUF_SIZE);
	if (!readBuffer) {
		exitErr("PORT_Alloc");
	}

	

	numBytes = PR_Write(sslSocket, requestString, strlen(requestString));
	if (numBytes <= 0) {
		errWarn("PR_Write");
		PR_Free(readBuffer);
		readBuffer = NULL;
		return SECFailure;
	}

	
	while (PR_TRUE) {
		numBytes = PR_Read(sslSocket, readBuffer, RD_BUF_SIZE);
		if (numBytes == 0) {
			break;	
		}
		if (numBytes < 0) {
			errWarn("PR_Read");
			break;
		}
		countRead += numBytes;
		fprintf(stderr, "***** Connection %d read %d bytes (%d total).\n", 
			connection, numBytes, countRead );
		readBuffer[numBytes] = '\0';
		fprintf(stderr, "************\n%s\n************\n", readBuffer);
	}

	printSecurityInfo(sslSocket);
	
	PR_Free(readBuffer);
	readBuffer = NULL;

	

	fprintf(stderr, 
	        "***** Connection %d read %d bytes total.\n", 
	        connection, countRead);

	return SECSuccess;	
}




SECStatus
do_connects(void *a, int connection)
{
	PRNetAddr  *addr = (PRNetAddr *)a;
	PRFileDesc *sslSocket;
	PRHostEnt   hostEntry;
	char        buffer[PR_NETDB_BUF_SIZE];
	PRStatus    prStatus;
	PRIntn      hostenum;
	SECStatus   secStatus;

	
	sslSocket = setupSSLSocket(addr);
	if (sslSocket == NULL) {
		errWarn("setupSSLSocket");
		return SECFailure;
	}

	secStatus = SSL_SetPKCS11PinArg(sslSocket, password);
	if (secStatus != SECSuccess) {
		errWarn("SSL_SetPKCS11PinArg");
		return secStatus;
	}

	secStatus = SSL_SetURL(sslSocket, hostName);
	if (secStatus != SECSuccess) {
		errWarn("SSL_SetURL");
		return secStatus;
	}

	
	prStatus = PR_GetHostByName(hostName, buffer, sizeof(buffer), &hostEntry);
	if (prStatus != PR_SUCCESS) {
		errWarn("PR_GetHostByName");
		return SECFailure;
	}

	hostenum = PR_EnumerateHostEnt(0, &hostEntry, port, addr);
	if (hostenum == -1) {
		errWarn("PR_EnumerateHostEnt");
		return SECFailure;
	}

	prStatus = PR_Connect(sslSocket, addr, PR_INTERVAL_NO_TIMEOUT);
	if (prStatus != PR_SUCCESS) {
		errWarn("PR_Connect");
		return SECFailure;
	}

	
#if 0
	secStatus = SSL_ForceHandshake(sslSocket);
	if (secStatus != SECSuccess) {
		errWarn("SSL_ForceHandshake");
		return secStatus;
	}
#endif

	secStatus = SSL_ResetHandshake(sslSocket,  PR_FALSE);
	if (secStatus != SECSuccess) {
		errWarn("SSL_ResetHandshake");
		prStatus = PR_Close(sslSocket);
		if (prStatus != PR_SUCCESS) {
			errWarn("PR_Close");
		}
		return secStatus;
	}

	secStatus = handle_connection(sslSocket, connection);
	if (secStatus != SECSuccess) {
		errWarn("handle_connection");
		return secStatus;
	}

	PR_Close(sslSocket);
	return SECSuccess;
}

void
client_main(unsigned short      port, 
            int	                connections, 
            const char *        hostName)
{
	int			i;
	SECStatus	secStatus;
	PRStatus    prStatus;
	PRInt32     rv;
	PRNetAddr	addr;
	PRHostEnt   hostEntry;
	char        buffer[256];

	
	prStatus = PR_GetHostByName(hostName, buffer, 256, &hostEntry);
	if (prStatus != PR_SUCCESS) {
		exitErr("PR_GetHostByName");
	}

	rv = PR_EnumerateHostEnt(0, &hostEntry, port, &addr);
	if (rv < 0) {
		exitErr("PR_EnumerateHostEnt");
	}

	secStatus = launch_thread(&threadMGR, do_connects, &addr, 1);
	if (secStatus != SECSuccess) {
		exitErr("launch_thread");
	}

	if (connections > 1) {
		
		reap_threads(&threadMGR);
		
		for (i = 2; i <= connections; ++i) {
			secStatus = launch_thread(&threadMGR, do_connects, &addr, i);
			if (secStatus != SECSuccess) {
				errWarn("launch_thread");
			}
		}
	}

	reap_threads(&threadMGR);
	destroy_thread_data(&threadMGR);
}

int
main(int argc, char **argv)
{
	char *               certDir      = ".";
	char *               progName     = NULL;
	int					 connections  = 1;
	char *               cipherString = NULL;
	SECStatus            secStatus;
	PLOptState *         optstate;
	PLOptStatus          status;

	
	PR_Init( PR_SYSTEM_THREAD, PR_PRIORITY_NORMAL, 1);

	progName = PL_strdup(argv[0]);

	hostName = NULL;
	optstate = PL_CreateOptState(argc, argv, "C:c:d:n:p:w:");
	while ((status = PL_GetNextOpt(optstate)) == PL_OPT_OK) {
		switch(optstate->option) {
		case 'C' : cipherString = PL_strdup(optstate->value); break;
		case 'c' : connections = PORT_Atoi(optstate->value);  break;
		case 'd' : certDir = PL_strdup(optstate->value);      break;
		case 'n' : certNickname = PL_strdup(optstate->value); break;
		case 'p' : port = PORT_Atoi(optstate->value);         break;
		case 'w' : password = PL_strdup(optstate->value);     break;
		case '\0': hostName = PL_strdup(optstate->value);     break;
		default  : Usage(progName);
		}
	}

	if (port == 0 || hostName == NULL)
		Usage(progName);

	if (certDir == NULL) {
		certDir = PR_smprintf("%s/.netscape", getenv("HOME"));
	}

	
	PK11_SetPasswordFunc(myPasswd);

	
	secStatus = NSS_Init(certDir);
	if (secStatus != SECSuccess) {
		exitErr("NSS_Init");
	}

	
	NSS_SetDomesticPolicy();
	SSL_CipherPrefSetDefault(SSL_RSA_WITH_NULL_MD5, PR_TRUE);

	
	if (cipherString) {
	    int ndx;

	    
	    disableAllSSLCiphers();

	    while (0 != (ndx = *cipherString++)) {
		int *cptr;
		int  cipher;

		if (! isalpha(ndx))
		    Usage(progName);
		cptr = islower(ndx) ? ssl3CipherSuites : ssl2CipherSuites;
		for (ndx &= 0x1f; (cipher = *cptr++) != 0 && --ndx > 0; )
		    ;
		if (cipher) {
		    SSL_CipherPrefSetDefault(cipher, PR_TRUE);
		}
	    }
	}

	client_main(port, connections, hostName);

	if (NSS_Shutdown() != SECSuccess) {
            exit(1);
        }
	PR_Cleanup();
	return 0;
}


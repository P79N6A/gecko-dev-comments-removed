










































#include <stdio.h>
#include <string.h>



#include "nspr.h"
#include "plgetopt.h"
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



#include "sslsample.h"

#ifndef PORT_Sprintf
#define PORT_Sprintf sprintf
#endif

#define REQUEST_CERT_ONCE 1
#define REQUIRE_CERT_ONCE 2
#define REQUEST_CERT_ALL  3
#define REQUIRE_CERT_ALL  4


GlobalThreadMgr   threadMGR;
char             *password = NULL;
CERTCertificate  *cert = NULL;
SECKEYPrivateKey *privKey = NULL;
int               stopping;

static void
Usage(const char *progName)
{
	fprintf(stderr, 

"Usage: %s -n rsa_nickname -p port [-3RFrf] [-w password]\n"
"					[-c ciphers] [-d dbdir] \n"
"-3 means disable SSL v3\n"
"-r means request certificate on first handshake.\n"
"-f means require certificate on first handshake.\n"
"-R means request certificate on all handshakes.\n"
"-F means require certificate on all handshakes.\n"
"-c ciphers   Letter(s) chosen from the following list\n"
"A	  SSL2 RC4 128 WITH MD5\n"
"B	  SSL2 RC4 128 EXPORT40 WITH MD5\n"
"C	  SSL2 RC2 128 CBC WITH MD5\n"
"D	  SSL2 RC2 128 CBC EXPORT40 WITH MD5\n"
"E	  SSL2 DES 64 CBC WITH MD5\n"
"F	  SSL2 DES 192 EDE3 CBC WITH MD5\n"
"\n"
"c	  SSL3 RSA WITH RC4 128 MD5\n"
"d	  SSL3 RSA WITH 3DES EDE CBC SHA\n"
"e	  SSL3 RSA WITH DES CBC SHA\n"
"f	  SSL3 RSA EXPORT WITH RC4 40 MD5\n"
"g	  SSL3 RSA EXPORT WITH RC2 CBC 40 MD5\n"
"i	  SSL3 RSA WITH NULL MD5\n"
"j	  SSL3 RSA FIPS WITH 3DES EDE CBC SHA\n"
"k	  SSL3 RSA FIPS WITH DES CBC SHA\n"
"l	  SSL3 RSA EXPORT WITH DES CBC SHA\t(new)\n"
"m	  SSL3 RSA EXPORT WITH RC4 56 SHA\t(new)\n",
	progName);
	exit(1);
}






SECStatus
readDataFromSocket(PRFileDesc *sslSocket, DataBuffer *buffer, char **fileName)
{
	char  *post;
	int    numBytes = 0;
	int    newln    = 0;  

	
	while (PR_TRUE) {
		buffer->index = 0;
		newln = 0;

		
		numBytes = PR_Read(sslSocket, &buffer->data[buffer->index], 
		                   buffer->remaining);
		if (numBytes <= 0) {
			errWarn("PR_Read");
			return SECFailure;
		}
		buffer->dataEnd = buffer->dataStart + numBytes;

		




		while (buffer->index < buffer->dataEnd && newln < 2) {
			int octet = buffer->data[buffer->index++];
			if (octet == '\n') {
				newln++;
			} else if (octet != '\r') {
				newln = 0;
			}
		}

		


		if (newln < 2) 
			continue;

		




		post = PORT_Strstr(buffer->data, "POST ");
		if (!post || *post != 'P') 
			break;

		
		
		while (buffer->index < buffer->dataEnd && newln < 3) {
			int octet = buffer->data[buffer->index++];
			if (octet == '\n') {
				newln++;
			}
		}

		if (newln == 3)
			break;
	}

	

	
	if (buffer->index > 0 && PORT_Strncmp(buffer->data, "GET ", 4) == 0) {
		int fnLength;

		
		fnLength = strcspn(buffer->data + 5, " \r\n");
		*fileName = (char *)PORT_Alloc(fnLength + 1);
		PORT_Strncpy(*fileName, buffer->data + 5, fnLength);
		(*fileName)[fnLength] = '\0';
	}

	return SECSuccess;
}







PRFileDesc * 
setupSSLSocket(PRFileDesc *tcpSocket, int requestCert)
{
	PRFileDesc *sslSocket;
	SSLKEAType  certKEA;
	int         certErr = 0;
	SECStatus   secStatus;

	

	sslSocket = SSL_ImportFD(NULL, tcpSocket);
	if (sslSocket == NULL) {
		errWarn("SSL_ImportFD");
		goto loser;
	}
   
	secStatus = SSL_OptionSet(sslSocket, SSL_SECURITY, PR_TRUE);
	if (secStatus != SECSuccess) {
		errWarn("SSL_OptionSet SSL_SECURITY");
		goto loser;
	}

	secStatus = SSL_OptionSet(sslSocket, SSL_HANDSHAKE_AS_SERVER, PR_TRUE);
	if (secStatus != SECSuccess) {
		errWarn("SSL_OptionSet:SSL_HANDSHAKE_AS_SERVER");
		goto loser;
	}

	secStatus = SSL_OptionSet(sslSocket, SSL_REQUEST_CERTIFICATE, 
	                       (requestCert >= REQUEST_CERT_ONCE));
	if (secStatus != SECSuccess) {
		errWarn("SSL_OptionSet:SSL_REQUEST_CERTIFICATE");
		goto loser;
	}

	secStatus = SSL_OptionSet(sslSocket, SSL_REQUIRE_CERTIFICATE, 
	                       (requestCert == REQUIRE_CERT_ONCE));
	if (secStatus != SECSuccess) {
		errWarn("SSL_OptionSet:SSL_REQUIRE_CERTIFICATE");
		goto loser;
	}

	

	secStatus = SSL_AuthCertificateHook(sslSocket, myAuthCertificate, 
	                                    CERT_GetDefaultCertDB());
	if (secStatus != SECSuccess) {
		errWarn("SSL_AuthCertificateHook");
		goto loser;
	}

	secStatus = SSL_BadCertHook(sslSocket, 
	                            (SSLBadCertHandler)myBadCertHandler, &certErr);
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

	secStatus = SSL_SetPKCS11PinArg(sslSocket, password);
	if (secStatus != SECSuccess) {
		errWarn("SSL_HandshakeCallback");
		goto loser;
	}

	certKEA = NSS_FindCertKEAType(cert);

	secStatus = SSL_ConfigSecureServer(sslSocket, cert, privKey, certKEA);
	if (secStatus != SECSuccess) {
		errWarn("SSL_ConfigSecureServer");
		goto loser;
	}

	return sslSocket;

loser:

	PR_Close(tcpSocket);
	return NULL;
}






SECStatus
authenticateSocket(PRFileDesc *sslSocket, PRBool requireCert)
{
	CERTCertificate *cert;
	SECStatus secStatus;

	

	cert = SSL_PeerCertificate(sslSocket);
	if (cert) {
		
		CERT_DestroyCertificate(cert);
		return SECSuccess;
	}

	
	secStatus = SSL_OptionSet(sslSocket, SSL_REQUEST_CERTIFICATE, PR_TRUE);
	if (secStatus != SECSuccess) {
		errWarn("SSL_OptionSet:SSL_REQUEST_CERTIFICATE");
		return SECFailure;
	}

	

	secStatus = SSL_OptionSet(sslSocket, SSL_REQUIRE_CERTIFICATE, requireCert);
	if (secStatus != SECSuccess) {
		errWarn("SSL_OptionSet:SSL_REQUIRE_CERTIFICATE");
		return SECFailure;
	}

	
	secStatus = SSL_ReHandshake(sslSocket, PR_TRUE);
	if (secStatus != SECSuccess) {
		errWarn("SSL_ReHandshake");
		return SECFailure;
	}

	
	secStatus = SSL_ForceHandshake(sslSocket);
	if (secStatus != SECSuccess) {
		errWarn("SSL_ForceHandshake");
		return SECFailure;
	}

	return SECSuccess;
}







SECStatus
writeDataToSocket(PRFileDesc *sslSocket, DataBuffer *buffer, char *fileName)
{
	int headerLength;
	int numBytes;
	char messageBuffer[120];
	PRFileDesc *local_file_fd = NULL;
	char header[] = "<html><body><h1>Sample SSL server</h1><br><br>";
	char filehd[] = "<h2>The file you requested:</h2><br>";
	char reqhd[]  = "<h2>This is your request:</h2><br>";
	char link[]   = "Try getting a <a HREF=\"../testfile\">file</a><br>";
	char footer[] = "<br><h2>End of request.</h2><br></body></html>";

	headerLength = PORT_Strlen(defaultHeader);

	
	numBytes = PR_Write(sslSocket, header, PORT_Strlen(header));
	if (numBytes < 0) {
		errWarn("PR_Write");
		goto loser;
	}

	if (fileName) {
		PRFileInfo  info;
		PRStatus    prStatus;

		


		prStatus = PR_GetFileInfo(fileName, &info);
		if (prStatus != PR_SUCCESS ||
		    info.type != PR_FILE_FILE ||
		    info.size < 0) {
			PORT_Free(fileName);
			
			goto writerequest;
		}

		local_file_fd = PR_Open(fileName, PR_RDONLY, 0);
		if (local_file_fd == NULL) {
			PORT_Free(fileName);
			goto writerequest;
		}

		
		numBytes = PR_Write(sslSocket, filehd, PORT_Strlen(filehd));
		if (numBytes < 0) {
			errWarn("PR_Write");
			goto loser;
		}

		


		numBytes = PR_TransmitFile(sslSocket, local_file_fd, 
		                           defaultHeader, headerLength,
		                           PR_TRANSMITFILE_KEEP_OPEN,
		                           PR_INTERVAL_NO_TIMEOUT);

		
		if (numBytes < 0) {
			errWarn("PR_TransmitFile");
			



		
		} else {
			numBytes -= headerLength;
			fprintf(stderr, "PR_TransmitFile wrote %d bytes from %s\n",
			        numBytes, fileName);
		}

		PORT_Free(fileName);
		PR_Close(local_file_fd);
	}

writerequest:

	
	numBytes = PR_Write(sslSocket, reqhd, PORT_Strlen(reqhd));
	if (numBytes < 0) {
		errWarn("PR_Write");
		goto loser;
	}

	
	if (buffer->index <= 0) {
		
		PORT_Sprintf(messageBuffer,
		             "GET or POST incomplete after %d bytes.\r\n",
		             buffer->dataEnd);
		numBytes = PR_Write(sslSocket, messageBuffer, 
		                    PORT_Strlen(messageBuffer));
		if (numBytes < 0) {
			errWarn("PR_Write");
			goto loser;
		}
	} else {
		
		fwrite(buffer->data, 1, buffer->index, stdout);
		
		numBytes = PR_Write(sslSocket, buffer->data, buffer->index);
		if (numBytes < 0) {
			errWarn("PR_Write");
			goto loser;
		}
		
		printSecurityInfo(sslSocket);
		
		if (buffer->index < buffer->dataEnd) {
			PORT_Sprintf(buffer->data, "Discarded %d characters.\r\n", 
			             buffer->dataEnd - buffer->index);
			numBytes = PR_Write(sslSocket, buffer->data, 
			                    PORT_Strlen(buffer->data));
			if (numBytes < 0) {
				errWarn("PR_Write");
				goto loser;
			}
		}
	}

	
	numBytes = PR_Write(sslSocket, footer, PORT_Strlen(footer));
	if (numBytes < 0) {
		errWarn("PR_Write");
		goto loser;
	}

	
	numBytes = PR_Write(sslSocket, link, PORT_Strlen(link));
	if (numBytes < 0) {
		errWarn("PR_Write");
		goto loser;
	}

	
	numBytes = PR_Write(sslSocket, "EOF\r\n\r\n\r\n", 9);
	if (numBytes < 0) {
		errWarn("PR_Write");
		goto loser;
	}

	
	if (!strncmp(buffer->data, stopCmd, strlen(stopCmd))) {
		stopping = 1;
	}
	return SECSuccess;

loser:

	
	if (!strncmp(buffer->data, stopCmd, strlen(stopCmd))) {
		stopping = 1;
	}
	return SECFailure;
}






SECStatus
handle_connection(void *tcp_sock, int requestCert)
{
	PRFileDesc *       tcpSocket = (PRFileDesc *)tcp_sock;
	PRFileDesc *       sslSocket = NULL;
	SECStatus          secStatus = SECFailure;
	PRStatus           prStatus;
	PRSocketOptionData socketOption;
	DataBuffer         buffer;
	char *             fileName = NULL;

	
	memset(buffer.data, 0, BUFFER_SIZE);
	buffer.remaining = BUFFER_SIZE;
	buffer.index = 0;
	buffer.dataStart = 0;
	buffer.dataEnd = 0;

	
	socketOption.option             = PR_SockOpt_Nonblocking;
	socketOption.value.non_blocking = PR_FALSE;
	PR_SetSocketOption(tcpSocket, &socketOption);

	sslSocket = setupSSLSocket(tcpSocket, requestCert);
	if (sslSocket == NULL) {
		errWarn("setupSSLSocket");
		goto cleanup;
	}

	secStatus = SSL_ResetHandshake(sslSocket,  PR_TRUE);
	if (secStatus != SECSuccess) {
		errWarn("SSL_ResetHandshake");
		goto cleanup;
	}

	


	fprintf(stdout, "\nReading data from socket...\n\n");
	secStatus = readDataFromSocket(sslSocket, &buffer, &fileName);
	if (secStatus != SECSuccess) {
		goto cleanup;
	}
	if (requestCert >= REQUEST_CERT_ALL) {
		fprintf(stdout, "\nAuthentication requested.\n\n");
		secStatus = authenticateSocket(sslSocket, 
		                               (requestCert == REQUIRE_CERT_ALL));
		if (secStatus != SECSuccess) {
			goto cleanup;
		}
	}

	fprintf(stdout, "\nWriting data to socket...\n\n");
	secStatus = writeDataToSocket(sslSocket, &buffer, fileName);

cleanup:

	
	prStatus = PR_Close(tcpSocket);
	if (prStatus != PR_SUCCESS) {
		errWarn("PR_Close");
	}

	return secStatus;
}






SECStatus
accept_connection(void *listener, int requestCert)
{
	PRFileDesc *listenSocket = (PRFileDesc*)listener;
	PRNetAddr   addr;
	PRStatus    prStatus;

	
	while (!stopping) {
		PRFileDesc *tcpSocket;
		SECStatus	result;

		fprintf(stderr, "\n\n\nAbout to call accept.\n");

		
		tcpSocket = PR_Accept(listenSocket, &addr, PR_INTERVAL_NO_TIMEOUT);
		if (tcpSocket == NULL) {
			errWarn("PR_Accept");
			break;
		}

		
		result = launch_thread(&threadMGR, handle_connection, 
		                       tcpSocket, requestCert);

		if (result != SECSuccess) {
			prStatus = PR_Close(tcpSocket);
			if (prStatus != PR_SUCCESS) {
				exitErr("PR_Close");
			}
			break;
		}
	}

	fprintf(stderr, "Closing listen socket.\n");

	prStatus = PR_Close(listenSocket);
	if (prStatus != PR_SUCCESS) {
		exitErr("PR_Close");
	}
	return SECSuccess;
}







void
server_main(
	unsigned short      port, 
	int                 requestCert, 
	SECKEYPrivateKey *  privKey,
	CERTCertificate *   cert, 
	PRBool              disableSSL3)
{
	SECStatus           secStatus;
	PRStatus            prStatus;
	PRFileDesc *        listenSocket;
	PRNetAddr           addr;
	PRSocketOptionData  socketOption;

	
	listenSocket = PR_NewTCPSocket();
	if (listenSocket == NULL) {
		exitErr("PR_NewTCPSocket");
	}

	


	socketOption.option = PR_SockOpt_Nonblocking;
	socketOption.value.non_blocking = PR_FALSE;

	prStatus = PR_SetSocketOption(listenSocket, &socketOption);
	if (prStatus != PR_SUCCESS) {
		exitErr("PR_SetSocketOption");
	}

	


	secStatus = SSL_CipherPrefSetDefault(SSL_RSA_WITH_NULL_MD5, PR_TRUE);
	if (secStatus != SECSuccess) {
		exitErr("SSL_CipherPrefSetDefault:SSL_RSA_WITH_NULL_MD5");
	}

	
	addr.inet.family = PR_AF_INET;
	addr.inet.ip	 = PR_INADDR_ANY;
	addr.inet.port	 = PR_htons(port);

	
	prStatus = PR_Bind(listenSocket, &addr);
	if (prStatus != PR_SUCCESS) {
		exitErr("PR_Bind");
	}

	


	prStatus = PR_Listen(listenSocket, 5);
	if (prStatus != PR_SUCCESS) {
		exitErr("PR_Listen");
	}

	
	secStatus = launch_thread(&threadMGR, accept_connection, 
                              listenSocket, requestCert);
	if (secStatus != SECSuccess) {
		PR_Close(listenSocket);
	} else {
		reap_threads(&threadMGR);
		destroy_thread_data(&threadMGR);
	}
}






int
main(int argc, char **argv)
{
	char *              progName      = NULL;
	char *              nickName      = NULL;
	char *              cipherString  = NULL;
	char *              dir           = ".";
	int                 requestCert   = 0;
	unsigned short      port          = 0;
	SECStatus           secStatus;
	PRBool              disableSSL3   = PR_FALSE;
	PLOptState *        optstate;
	PLOptStatus         status;

	
	PORT_Memset(&threadMGR, 0, sizeof(threadMGR));

	progName = PL_strdup(argv[0]);

	optstate = PL_CreateOptState(argc, argv, "3FRc:d:fp:n:rw:");
	while ((status = PL_GetNextOpt(optstate)) == PL_OPT_OK) {
		switch(optstate->option) {
		case '3': disableSSL3 = PR_TRUE;                      break;
		case 'F': requestCert = REQUIRE_CERT_ALL;             break;
		case 'R': requestCert = REQUEST_CERT_ALL;             break;
		case 'c': cipherString = PL_strdup(optstate->value);  break;
		case 'd': dir = PL_strdup(optstate->value);           break;
		case 'f': requestCert = REQUIRE_CERT_ONCE;            break;
		case 'n': nickName = PL_strdup(optstate->value);      break;
		case 'p': port = PORT_Atoi(optstate->value);          break;
		case 'r': requestCert = REQUEST_CERT_ONCE;            break;
		case 'w': password = PL_strdup(optstate->value);      break;
		default:
		case '?': Usage(progName);
		}
	}

	if (nickName == NULL || port == 0)
		Usage(progName);

	
	PR_Init( PR_SYSTEM_THREAD, PR_PRIORITY_NORMAL, 1);

	
	PK11_SetPasswordFunc(myPasswd);

	
	secStatus = NSS_Init(dir);
	if (secStatus != SECSuccess) {
		exitErr("NSS_Init");
	}

	
	secStatus = NSS_SetDomesticPolicy();
	if (secStatus != SECSuccess) {
		exitErr("NSS_SetDomesticPolicy");
	}

	
	
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
		    SECStatus status;
		    status = SSL_CipherPrefSetDefault(cipher, PR_TRUE);
		    if (status != SECSuccess) 
			errWarn("SSL_CipherPrefSetDefault()");
		}
	    }
	}

	
	cert = PK11_FindCertFromNickname(nickName, password);
	if (cert == NULL) {
		exitErr("PK11_FindCertFromNickname");
	}

	privKey = PK11_FindKeyByAnyCert(cert, password);
	if (privKey == NULL) {
		exitErr("PK11_FindKeyByAnyCert");
	}

	


	SSL_ConfigMPServerSIDCache(256, 0, 0, NULL);

	
	server_main(port, requestCert, privKey, cert, disableSSL3);

	
	if (NSS_Shutdown() != SECSuccess) {
            exit(1);
        }
	PR_Cleanup();
	return 0;
}

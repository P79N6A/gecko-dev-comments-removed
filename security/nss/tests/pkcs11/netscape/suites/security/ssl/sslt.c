



#define VERION_MAJOR 1
#define VERION_MINOR 0
#define VERSION_POINT 7

#include <prinit.h>
#include <prprf.h>
#include <prsystem.h>
#include <prmem.h>
#include <plstr.h>
#include <prnetdb.h>
#include <prinrval.h>
#include <prmon.h>
#include <prlock.h>


#include "cert.h"
#include "key.h"
#include "secmod.h"
#include "secutil.h"
#include "pk11func.h"


#include "ssl.h"
#include "sslproto.h"

#define EXIT_OOPS 14

#include "ssls.h"
#include "sslc.h"

#ifdef XP_PC

#include "excpt.h"
#endif

#ifndef DEBUG_stevep
#define dbmsg(x) if (debug) PR_fprintf x ;
#else
#define dbmsg(x) ;
#endif



PRInt32 ServerThread(PRInt32 argc,char **argv);
void ClientThread(void *arg);
void SetupNickNames(void );
int OpenDBs(void);
int ConfigServerSocket(void);
int DoIO(struct ThreadData *);
int Client(void);
int SetClientSecParams(void);
int CreateClientSocket(void);

#ifdef XP_PC
extern char getopt(int, char**, char*);
#endif
extern int Version2Enable(PRFileDesc *s);
extern int Version3Enable(PRFileDesc *s);
extern int Version23Clear(PRFileDesc *s);
extern void SetupNickNames();
extern int AuthCertificate(void *arg,PRFileDesc *fd,
			   PRBool checkSig, PRBool isServer);
extern char *MyPWFunc(void *slot, PRBool retry, void *arg);

extern char *nicknames[];
extern char *client_nick;
extern char *password, *nickname;



int rc;            
PRMonitor *rcmon;  
                   



PRInt32 debug   = 0;
PRInt32 verbose = 0;
CERTCertDBHandle *cert_db_handle = NULL;

struct ThreadData cl,svr;








#define INSERT_TABLES
#include "sslt.h"
#include "nss.h"









int OpenDBs() {
  int r;

  NSS_Init(".");
  return 0;
}











int CreateServerSocket(struct ThreadData *td) {
  

  td->fd = PR_NewTCPSocket();
  if (td->fd == NULL) return Error(20);

  td->r = SSL_ImportFD(NULL, td->fd);
  if (td->r == NULL) return Error(21);

  return 0;
} 


int ConfigServerSocket() {

  
  int r;
 
  r = PR_InitializeNetAddr(PR_IpAddrAny,0,&svr.na);
  if (PR_SUCCESS != r) return Error(2);

  
  r = PR_Bind(svr.r,&svr.na);     
  if (PR_SUCCESS != r) return Error(3);


  r = PR_Listen(svr.r,5);
  if (PR_SUCCESS != r) return Error(4);


  r = PR_GetSockName(svr.r,&svr.na);
  if (PR_SUCCESS != r) return Error(5);
  return r;
}








PRIntn main(PRIntn ac, char **av, char **ev) {
  int r;
  extern char *optarg;	
  extern int optind;
  int c;
  

  if( ac == 1 ) {
     PR_fprintf(PR_STDERR,
"\nSSL Test Suite Version %d.%d.%d\n\
All Rights Reserved\n\
Usage: sslt [-c client_nickname] [-n server_nickname] [-p passwd] [-d] testid\n",
VERION_MAJOR, VERION_MINOR, VERSION_POINT);

    exit(0);
  }

  for (c = 1; c<ac; c++) {
	if (!PL_strcmp(av[c],"-c")) {
	
		  c++;
		  if (c < ac) {
			client_nick = av[c];
		  }
		  else {
			  PR_fprintf(PR_STDOUT,"must supply argument for -c\n");
			  exit(0);
		  }
	}

	else if (!PL_strcmp(av[c],"-n")) {
	
		  c++;
		  if (c < ac) {
			nickname = av[c];
		  }
		  else {
			  PR_fprintf(PR_STDOUT,"must supply argument for -n\n");
			  exit(0);
		  }
      }
	  else if (!PL_strcmp(av[c],"-p")) {

		  c++;
		  if (c < ac) {
			password = av[c];
		  }
		  else {
			  PR_fprintf(PR_STDOUT,"must supply argument for -p\n");
			  exit(0);
		  }
      }
	else if (!PL_strcmp(av[c],"-d")) {
		  c++;
		  debug++;
      }
	else 
		testId = atoi(av[c]);
  }



#ifdef XP_PC
    __try {
#endif
 
  r = PR_Initialize(ServerThread,ac,av,400);         /* is 400 enough? */

  
  if (99 == r)  r = 0;

#ifdef XP_PC
    } __except( PR_fprintf(PR_STDERR, "\nCERT-TEST crashed\n"), EXCEPTION_EXECUTE_HANDLER ) {
        r = 255;
    }
#endif

  return r;

}









PRInt32 ServerThread(PRInt32 argc,char **argv) {

  PRNetAddr na;

  PRStatus r;
  SECStatus rv;
  
  CERTCertDBHandle *cert_db_handle;
  PRInt32 i,j;
  struct ThreadData * td;

  
  




  rcmon = PR_NewMonitor();
  if (NULL == rcmon) return Error(140);

  PR_EnterMonitor(rcmon);
  rc = 0;
  PR_ExitMonitor(rcmon);

  InitCiphers();
  SetPolicy();
  SetupNickNames();
  
  cl.peer = &svr;
  svr.peer = &cl;


  r = OpenDBs();            
  if (PR_SUCCESS != r) return r;


  r = CreateServerSocket(&svr);
  if (PR_SUCCESS != r) return r;

  r = ConfigServerSocket();
  if (PR_SUCCESS != r) return r;

  cl.peerport = svr.na.inet.port;


  r = SetServerSecParms(&svr);  

  if (r) return r;

  r = SSL_HandshakeCallback(svr.r, HandshakeCallback, &svr);
  if (PR_SUCCESS != r) return Error(150);

  r = SSL_AuthCertificateHook(svr.r,AuthCertificate,&svr);
  if (PR_SUCCESS !=r ) return Error(151);

  


  svr.subthread =
    PR_CreateThread(PR_SYSTEM_THREAD,   
		    ClientThread,       
		    NULL,               
		    PR_PRIORITY_NORMAL, 
		    PR_GLOBAL_THREAD,   
		    PR_JOINABLE_THREAD, 
		    0          
		    );
  if (svr.subthread == NULL) return Error(6);


  
  

  svr.s = PR_Accept(svr.r, NULL, PR_SecondsToInterval(100)); 
  if (NULL == svr.s) {
    r = PR_GetError();
    if (r) {
      return Error(7);
    }
  }

  td = &svr;
  td->client = PR_FALSE;
  td->xor_reading = CLIENTXOR;
  td->xor_writing = 0;
  
  r = DoIO(td);
  dbmsg((PR_STDERR,"Server IO complete - returned %d\n",r));
  dbmsg((PR_STDERR,"PR_GetError() = %d\n",PR_GetError()));


  
  r = 0;
  if (r) return r;
  

  

  r = PR_Close(svr.s);   
  if (r != PR_SUCCESS) return Error(8);

  dbmsg((PR_STDERR,"PR_Close(svr.s) - returned %d\n",r));

  r = PR_Close(svr.r);  
  if (r != PR_SUCCESS) return Error(8);

  dbmsg((PR_STDERR,"PR_Close(svr.r) - returned %d\n",r));

  r = PR_JoinThread(svr.subthread);
  if (r != PR_SUCCESS)  return Error(9);

  PR_EnterMonitor(rcmon);
  r = rc;
  PR_ExitMonitor(rcmon);
  
  dbmsg((PR_STDERR,"Client Thread Joined. client's returncode=%d\n",r));
  dbmsg((PR_STDERR,"Server Thread closing down.\n"));

  return r;
  
  }





 

int GetSecStatus(struct ThreadData *td) {
  int r;

  r = SSL_SecurityStatus(td->s,
			 &td->status_on,
			 &td->status_cipher,
			 &td->status_keysize,
			 &td->status_skeysize,
			 &td->status_issuer,
			 &td->status_subject
			 );

  return r;
  

}
 







int Error(int s)
{
  int r;

  PR_EnterMonitor(rcmon);
  r = rc;
  if (0 == rc) { 
    rc = s;
  }    
  PR_ExitMonitor(rcmon);

  if (r) return s;
  else return 0;
}



#define ALLOWEDBYPROTOCOL    1
#define ALLOWEDBYPOLICY      2
#define ALLOWEDBYCIPHERSUITE 4




int VerifyStatus(struct ThreadData *td)
{
  int i,j;
  int matched =0;

  
  
  

  




  for (i=0;i<cipher_array_size;i++) {

  

    if (

	
	(!( 
	  (REP_SSLVersion2 && REP_SSLVersion3) && cipher_array[i].sslversion == 2)
	 )

	&&


	(  
	 ((cipher_array[i].sslversion == 2) && REP_SSLVersion2) ||
	 ((cipher_array[i].sslversion == 3) && REP_SSLVersion3) 
	 )
	
	&&  

	((cipher_array[i].on == 1) ||
	 ((cipher_array[i].on == 2) &&
	  (REP_ServerCert == SERVER_CERT_VERISIGN_STEPUP)))

	&&  
    
	(
	 (REP_Policy == POLICY_DOMESTIC) ||
	 ((REP_Policy == POLICY_EXPORT)  && 
	  (cipher_array[i].exportable == SSL_ALLOWED)))
	)

  
      {
	
	
	matched = 1;
	break;	
      }
  }

GetSecStatus(td);


#define SSLT_STATUS_CORRECT           0 /* The status is correct. Continue with test */
#define SSLT_STATUS_WRONG_KEYSIZE     1 /* The reported keysize is incorrect. abort */
#define SSLT_STATUS_WRONG_SKEYSIZE    2 /* The reported secret keysize is incorrect. abort */
#define SSLT_STATUS_WRONG_DESCRIPTION 3 /* The reported description is incorrect. abort*/
#define SSLT_STATUS_WRONG_ERRORCODE   4 /* sec. library error - but wrong one - abort */
#define SSLT_STATUS_CORRECT_ERRORCODE 5 /* security library error - right one - abort with err 99 */

  if (matched) {
    if (td->status_keysize  != cipher_array[i].ks) {
      PR_fprintf(PR_STDERR,"wrong keysize. seclib: %d,  expected %d\n",
		 td->status_keysize,cipher_array[i].ks);
      return  SSLT_STATUS_WRONG_KEYSIZE;
    }
    if (td->status_skeysize != cipher_array[i].sks) return SSLT_STATUS_WRONG_SKEYSIZE;
    if (PL_strcmp(td->status_cipher,cipher_array[i].name)) {
      PR_fprintf(PR_STDERR,"wrong cipher description.  seclib: %s, expected: %s\n",
	     td->status_cipher,cipher_array[i].name);
      return SSLT_STATUS_WRONG_DESCRIPTION;
    }

    
  
    return SSLT_STATUS_CORRECT;
  }

  else {
    



    




#if 0
	if (PR_FALSE == REP_SSLVersion2 &&
	    PR_FALSE == REP_SSLVersion3)
{
if ( (td->secerr_flag == PR_FALSE ) ||
          ((td->secerr_flag == PR_TRUE) && 
	     !((td->secerr == SSL_ERROR_SSL_DISABLED) ||
	      (td->secerr == SSL_ERROR_NO_CYPHER_OVERLAP))
   	)) {
       return SSLT_STATUS_WRONG_ERRORCODE;
     }
     else
  return SSLT_STATUS_CORRECT_ERRORCODE;
   }

	else {

	  



	  if ((td->secerr_flag == PR_FALSE) ||
	      ((td->secerr_flag == PR_TRUE) && (td->secerr != SSL_ERROR_NO_CYPHER_OVERLAP))) {
	    return SSLT_STATUS_WRONG_ERRORCODE;
	  }
	  else return SSLT_STATUS_CORRECT_ERRORCODE;
	}
#endif
  }
	return SSLT_STATUS_CORRECT_ERRORCODE;
}









int DoRedoHandshake(struct ThreadData *td) {
  int r;


  
  if ((td->client  && (PR_TRUE== REP_ClientRedoHandshake)) ||
	(!td->client && (PR_TRUE== REP_ServerRedoHandshake))) {

    if ((!td->client && (SSLT_CLIENTAUTH_REDO==REP_ServerDoClientAuth))) {
       r = SSL_Enable(td->s, SSL_REQUEST_CERTIFICATE, 1);
    }

    r = SSL_RedoHandshake(td->s);                
    if (PR_SUCCESS == r) {                  
                                            

	




      
#if 0
      if (SSLT_CLIENTAUTH_INITIAL == REP_ServerDoClientAuth) {
	if ((CLIENT_CERT_SPARK == REP_ClientCert) ||
	    (SERVER_CERT_HARDCOREII_512       == REP_ClientCert) ||
	    (NO_CERT                           == REP_ClientCert)
	    ) 
	  return Error(90);

      }
#endif
      
    }
    
    else {  
      
      
      r = Error(91);
      if (0==r) return 0;  
      else {
      




      
	if (PR_TRUE == REP_ServerDoClientAuth) {
	  if ((CLIENT_CERT_HARDCOREII_512         == REP_ClientCert) ||
	      (CLIENT_CERT_HARDCOREII_1024        == REP_ClientCert) ||
	      (CLIENT_CERT_VERISIGN               == REP_ClientCert) ||
	      (SERVER_CERT_HARDCOREII_512         == REP_ClientCert)
	      ) 
	    return Error(91);
	}
      }
    }
  }
}





























    






int NextState(struct ThreadData *td,
	       int finishedReading,
	       int finishedWriting) {
  int r;



  



  if (STATE_BEFORE_INITIAL_HANDSHAKE == td->state ) {
    
    td->state = STATE_BEFORE_REDO_HANDSHAKE;  
    
    r = GetSecStatus(td);
    if (PR_SUCCESS != r) {
      return Error(80);
    }
    
#if 0
    r = VerifyStatus(td);   

    if (PR_SUCCESS != r) return r;
#endif

      
  }
  
  if (STATE_BEFORE_REDO_HANDSHAKE == td->state) {
    
    if (td->client) {
      if (PR_FALSE  == REP_ClientRedoHandshake) td->state = STATE_STATUS_COLLECTED;
    }
    else {
      if (PR_FALSE == REP_ServerRedoHandshake) td->state = STATE_STATUS_COLLECTED;
    }
    r = DoRedoHandshake(td);
    if (PR_SUCCESS != r) return r;
    td->state = STATE_STATUS_COLLECTED;
  }
		  

  switch (td->state) {
  case STATE_STATUS_COLLECTED:
    if (finishedWriting) td->state = STATE_DONE_WRITING;
    if (finishedReading) td->state = STATE_DONE_READING;
    break;
  case STATE_DONE_WRITING:
    if (finishedReading) td->state = STATE_DONE;
    break;
  case STATE_DONE_READING:
    if (finishedWriting) td->state = STATE_DONE;
    break;
  default:
    return PR_SUCCESS;
  }
}








int CheckSSLEnabled(int j) {
  if (PR_FALSE == REP_SSLVersion2 &&
      PR_FALSE == REP_SSLVersion3) {
    if (( -1 != j ) ||
	(( -1 == j) && (PR_GetError() != SSL_ERROR_SSL_DISABLED))) {
      return 52;
    }
    else return 99;
  }
  else return 0;
}








 
int DoIO(struct ThreadData *td) {

int i,j,r;

  td->pd.fd        = td->s;
  td->pd.in_flags  = PR_POLL_READ | PR_POLL_WRITE | PR_POLL_EXCEPT;
  td->data_read    = 0;
  td->data_sent    = 0;

  td->data_tosend = REP_ServerIOSessionLength;
  
  td->state = STATE_BEFORE_INITIAL_HANDSHAKE;


  while (PR_TRUE) {
    dbmsg((PR_STDERR,"%s: DoIO loop\n",
	       &svr==td ? "Server" : "Client"));

    
    r = PR_Poll(&td->pd,1,PR_SecondsToInterval(5));

     

    PR_EnterMonitor(rcmon);
    if (0 != rc) {
      

      PR_ExitMonitor(rcmon);
      dbmsg((PR_STDERR,"%s: Peer has aborted (error code %d). We should too\n",
	     &svr==td ? "Server" : "Client",rc));
      
      return 0;
    }
    else {
      PR_ExitMonitor(rcmon);
    }

    if (0 == r) ;   

    if (td->pd.out_flags & PR_POLL_EXCEPT) return Error(50);

    
    
    if (! (STATE_DONE == td->state || STATE_DONE_READING == td->state)) {
      if (td->pd.out_flags & PR_POLL_READ) {

	td->secerr = 0;
	i = PR_Read(td->s, td->recvbuf, BUFSIZE);

	if (i < 0) {
	  td->secerr_flag = 1;
	  td->secerr = PR_GetError();
	}
	else td->secerr_flag =0;

	r = VerifyStatus(td);

	switch (r) {
	case SSLT_STATUS_CORRECT:
	  break;
	case SSLT_STATUS_CORRECT_ERRORCODE:
	  return Error(99);
	default:
	  return Error(60+r);
	}
	
	r = VerifyBuffer(td->recvbuf, i, td->data_read, td->xor_reading);
	if (r) return r;
	td->data_read += i;
	
	

	NextState(td, 0==i, 0);  

      }
    }
    
    if (! (STATE_DONE == td->state || STATE_DONE_WRITING == td->state)) {
      if (td->pd.out_flags & PR_POLL_WRITE) {
	FillBuffer(td->sendbuf,BUFSIZE,td->data_sent,td->xor_writing);

	i = td->data_tosend - td->data_sent;
	if (i > BUFSIZE) i = BUFSIZE;  

	td->secerr = 0;
	j = PR_Write(td->s, td->sendbuf, i);


	if (j < 0) {
	  td->secerr_flag = 1;
	  td->secerr = PR_GetError();
	}
	else td->secerr_flag =0;

	r = VerifyStatus(td);

	switch (r) {
	case SSLT_STATUS_CORRECT:
	  break;
	case SSLT_STATUS_CORRECT_ERRORCODE:
	  return Error(99);
	default:
	  return Error(60+r);
	}

      }
      if (j == -1) return Error(53);        


      
      if (j != i) return Error(54);         

      
      td->data_sent += j;
      
      if (td->data_sent == td->data_tosend) {
	PR_Shutdown(td->s,PR_SHUTDOWN_SEND);
      }
      
      

      NextState(td,
		0,
		td->data_sent == td->data_tosend  
		);      
    }



    if (STATE_DONE == td->state) break;

  } 
    
    dbmsg((PR_STDERR,"%s: DoIO loop:returning 0\n",
	       &svr==td ? "Server" : "Client"));

    return 0;
    
}














int CreateClientSocket() {
  

  cl.fd = PR_NewTCPSocket();
  if (cl.fd == NULL) return Error(120);  

  cl.s = SSL_ImportFD(NULL, cl.fd);
  if (cl.s == NULL) return Error(121);

  return 0;
}  








int SetClientSecParams()  {
  int rv;
  
  
  rv = SSL_Enable(cl.s, SSL_SECURITY, 1);
  if (rv < 0)  return Error(130);

  rv = Version23Clear(cl.s);
  if (rv) return rv;

  if (REP_SSLVersion2) {
    rv = Version2Enable(cl.s);
    if (rv) return rv;
  }
  if (REP_SSLVersion3) {
    rv = Version3Enable(cl.s);
    if (rv) return rv;
  }

  SSL_SetPKCS11PinArg(cl.s,(void*)MyPWFunc);

  if (REP_ClientCert == NO_CERT) {
    return 0;
  }
  else {
    cl.cert = PK11_FindCertFromNickname(client_nick,NULL);
  }
  if (cl.cert == NULL) return Error(131);
  
  return 0;
}







int Client() {
  int r;

  r = CreateClientSocket();
  if (r) return r;

  r = SetClientSecParams();
  if (r) return r;

  

  r = PR_InitializeNetAddr(PR_IpAddrLoopback,0,&cl.na);
  cl.na.inet.port = cl.peerport;
  if (PR_FAILURE == r) return Error(101);

  r = SSL_AuthCertificateHook(cl.s,AuthCertificate,&cl);
  if (r) return Error(102);
  r = SSL_HandshakeCallback(cl.s,HandshakeCallback,&cl);
  if (r) return Error(103);

  r = PR_Connect(cl.s, &cl.na, PR_SecondsToInterval(50));
  if (PR_FAILURE == r) {
    dbmsg((PR_STDERR, "Client: Seclib error: %s\n",SECU_Strerror(PR_GetError())));
    return Error(104);
  }


  if (PR_TRUE == REP_ClientForceHandshake) {
    r = SSL_ForceHandshake(cl.s);
    if (PR_FAILURE == r) {
      dbmsg((PR_STDERR, "Client: Seclib error: %s\n",
	SECU_Strerror(PR_GetError())));
      return Error(105);
    }
  }

  cl.client = PR_TRUE;
  cl.xor_reading = 0;
  cl.xor_writing = CLIENTXOR;
  
  r = DoIO(&cl);

  dbmsg((PR_STDERR,"Client Thread done with IO. Returned %d\n",r));


  if (PR_SUCCESS != r) return r;

  r = PR_Close(cl.s);

  dbmsg((PR_STDERR,"Client Socket closing. Returned %d\n",r));

  return Error(r);
  
}



 void ClientThread(void *arg) {
   int r;

   Error(Client());

   dbmsg((PR_STDERR,"Client Thread returning %d\n",r));
   
   
 }

   




 











 
 int VerifyBuffer(char *recvbuf,int bufsize,int done, char xor) {
  int i,j,k;

  while (bufsize) {
    i = done % DATABUFSIZE;

    k = DATABUFSIZE;
    if (bufsize < k) {
      k = bufsize;
    }
    for (j = i; j < k ; j++) {
      if ((data[j] ^ xor) != (*recvbuf)) {
	return 71;
      }
      
      recvbuf++;
    }
    done += k-i;
    bufsize -= (k - i);
    if (bufsize < 0) return 73;
  }
  return (0);
}




 void FillBuffer(char *sendbuf,int bufsize, int offset, char xor) {
   int done=0,i,j;
   
   while (done < bufsize) {
    i = offset % DATABUFSIZE;
    for (j = i; j < DATABUFSIZE ; j++) {
      *sendbuf = (data[j] ^ xor);
      sendbuf++;
    }
    done += (DATABUFSIZE - i);
    offset += (DATABUFSIZE - i);
   }
 }
 
 
 
 

 







 void HandshakeCallback(PRFileDesc *s, void *td)   {
   int r;

   

   r = GetSecStatus(td);
   if (PR_SUCCESS != r) {
     
   }
   else {

   

#if 0   
  r =VerifyStatus(td); 
     if (PR_SUCCESS != r) {
       
     }
#endif
   }

 }
 








int
AuthCertificate(void *arg, PRFileDesc *fd, PRBool checkSig, PRBool isServer)
{
    SECStatus rv;
    CERTCertDBHandle *handle;
    
    SECCertUsage certUsage;
    
    


    handle = (CERTCertDBHandle *)arg;

    if ( isServer ) {
	certUsage = certUsageSSLClient;
    } else {
	certUsage = certUsageSSLServer;
    }
    
    

    return((int)PR_SUCCESS);
}












#ifndef SSLC_H
#define SSLC_H

#include "ssls.h"

struct cipherspec {
  int sslversion;  
  int exportable;  
  int ks,sks;      
  char *name;      
  int enableid;    
  int on;          
};






#define CIPHER(p_sslversion,p_policy,p_ks,p_sks,p_name,p_x) {\
 cipher_array[i].sslversion = p_sslversion; \
 cipher_array[i].exportable = p_policy;     \
 cipher_array[i].ks         = p_ks;         \
 cipher_array[i].sks        = p_sks;        \
 cipher_array[i].name       = p_name;       \
 cipher_array[i].enableid   = SSL_ ## p_x;  \
 cipher_array[i].on         = REP_Cipher_ ## p_x; \
 i++; }


#define DIPHER(sslversion,policy,ks,sks,name,x)  ;




#define NO_CERT                       -1
#define CLIENT_CERT_VERISIGN          1
#define CLIENT_CERT_HARDCOREII_1024   2
#define CLIENT_CERT_HARDCOREII_512    3
#define CLIENT_CERT_SPARK             4
#define SERVER_CERT_HARDCOREII_512    5
#define SERVER_CERT_VERISIGN_REGULAR  6
#define SERVER_CERT_VERISIGN_STEPUP   7
#define SERVER_CERT_SPARK             8
#define MAX_NICKNAME                  10

extern struct cipherspec cipher_array[];
extern int cipher_array_size;

extern void ClearCiphers();
extern void EnableCiphers();
extern void SetPolicy();
extern int  Version2Enable();
extern int  Version3Enable();
extern int  Version23Clear();
extern char *nicknames[];
extern void SetupNickNames();
extern int  SetServerSecParms(struct ThreadData *td);


#endif




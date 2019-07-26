




























































#ifndef _MD5_H
#define _MD5_H


#define CISCO_MD5_MODS

#if defined(CISCO_MD5_MODS)

#define MD5_LEN 16

#endif 


typedef struct {
    unsigned long int state[4]; 
    unsigned long int count[2]; 
    unsigned char buffer[64];   
} MD5_CTX;

void MD5Init(MD5_CTX *);
void MD5Update(MD5_CTX *, unsigned char *, unsigned int);
void MD5Final(unsigned char[16], MD5_CTX *);


#if defined(MD5TESTSUITE) 
void MDString(char *string);
void MDPrint(char *digest);
void MDTestSuite(void);
#endif

#endif 

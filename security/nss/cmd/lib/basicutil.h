


#ifndef _BASIC_UTILS_H_
#define _BASIC_UTILS_H_

#include "seccomon.h"
#include "secitem.h"
#include "secoid.h"
#include "secoidt.h"
#include "secport.h"
#include "prerror.h"
#include "base64.h"
#include "secasn1.h"
#include "secder.h"
#include <stdio.h>

#ifdef SECUTIL_NEW
typedef int (*SECU_PPFunc)(PRFileDesc *out, SECItem *item, 
                           char *msg, int level);
#else
typedef int (*SECU_PPFunc)(FILE *out, SECItem *item, char *msg, int level);
#endif


extern void SECU_PrintError(char *progName, char *msg, ...);


extern void SECU_PrintSystemError(char *progName, char *msg, ...);


extern void SECU_PrintErrMsg(FILE *out, int level, char *progName, char *msg, ...);


extern SECStatus SECU_FileToItem(SECItem *dst, PRFileDesc *src);
extern SECStatus SECU_TextFileToItem(SECItem *dst, PRFileDesc *src);


extern void SECU_Indent(FILE *out, int level);


extern void SECU_Newline(FILE *out);


extern void SECU_PrintInteger(FILE *out, SECItem *i, char *m, int level);


extern void SECU_PrintAsHex(FILE *out, SECItem *i, const char *m, int level);


extern void SECU_PrintBuf(FILE *out, const char *msg, const void *vp, int len);

#ifdef HAVE_EPV_TEMPLATE

extern int SECU_PrintPrivateKey(FILE *out, SECItem *der, char *m, int level);
#endif


extern SECStatus SECU_PKCS11Init(PRBool readOnly);


extern int SECU_PrintSignedData(FILE *out, SECItem *der, const char *m, 
                                int level, SECU_PPFunc inner);

extern void SECU_PrintString(FILE *out, SECItem *si, char *m, int level);
extern void SECU_PrintAny(FILE *out, SECItem *i, char *m, int level);

extern void SECU_PrintPRandOSError(char *progName);


void
SECU_SECItemToHex(const SECItem * item, char * dst);



SECStatus
SECU_SECItemHexStringToBinary(SECItem* srcdest);








typedef struct {
    char flag;
    PRBool needsArg;
    char *arg;
    PRBool activated;
    char *longform;
} secuCommandFlag;


typedef struct
{
    int numCommands;
    int numOptions;

    secuCommandFlag *commands;
    secuCommandFlag *options;
} secuCommand;


SECStatus 
SECU_ParseCommandLine(int argc, char **argv, char *progName,
		      const secuCommand *cmd);
char *
SECU_GetOptionArg(const secuCommand *cmd, int optionNum);







void printflags(char *trusts, unsigned int flags);

#if !defined(XP_UNIX) && !defined(XP_OS2)
extern int ffs(unsigned int i);
#endif

#include "secerr.h"

extern const char *hex;
extern const char printable[];

#endif 

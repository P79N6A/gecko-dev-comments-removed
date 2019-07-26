





#ifdef FREEBL_NO_DEPEND
#include "stubs.h"
#endif

#include "shsign.h"
#include "prlink.h"
#include "prio.h"
#include "blapi.h"
#include "seccomon.h"
#include "stdio.h"
#include "prmem.h"
#include "hasht.h"
#include "pqg.h"


















#ifdef FREEBL_USE_PRELINK
#ifndef FREELB_PRELINK_COMMAND
#define FREEBL_PRELINK_COMMAND "/usr/sbin/prelink -u -o -"
#endif
#include "private/pprio.h"

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>














PRFileDesc *
bl_OpenUnPrelink(const char *shName, int *pid)
{
    char *command= strdup(FREEBL_PRELINK_COMMAND);
    char *argString = NULL;
    char  **argv = NULL;
    char *shNameArg = NULL;
    char *cp;
    pid_t child;
    int argc = 0, argNext = 0;
    struct stat statBuf;
    int pipefd[2] = {-1,-1};
    int ret;

    *pid = 0;

    

    for (cp = command; *cp ; cp++) {
	if (*cp == ' ') {
	    *cp++ = 0;
	    argString = cp;
	    break;
        }
    }
    memset (&statBuf, 0, sizeof(statBuf));
    
    ret = stat(command, &statBuf);
    if (ret < 0) {
	free(command);
	return PR_Open(shName, PR_RDONLY, 0);
    }
    
    if (!S_ISREG(statBuf.st_mode) || 
			((statBuf.st_mode & (S_IXUSR|S_IXGRP|S_IXOTH)) == 0)) {
	free(command);
	return PR_Open(shName, PR_RDONLY, 0);
    }

    
    
    
    if (argString && *argString) {
	
	for (cp = argString; *cp && *cp == ' '; cp++);
	argString = cp;
	if (*cp) {
	    
	    argc = 1;
	}

        

	for (cp = argString; *cp ; cp++) {
	    if (*cp == ' ') {
		while (*cp && *cp == ' ') cp++;
		if (*cp) argc++;
	    }
	}
    }

    
    argc += 3;
    argv = PORT_NewArray(char *, argc);
    if (argv == NULL) {
	goto loser;
    }

    
    argv[argNext++] = command;
    if (argString && *argString) {
	argv[argNext++] = argString;
	for (cp = argString; *cp; cp++) {
	    if (*cp == ' ') {
		*cp++ = 0;
		while (*cp && *cp == ' ') cp++;
		if (*cp) argv[argNext++] = cp;
	    }
	}
    }
    

    shNameArg = strdup(shName);
    if (shNameArg == NULL) {
	goto loser;
    }
    argv[argNext++] = shNameArg;
    argv[argNext++] = 0;
    
    ret = pipe(pipefd);
    if (ret < 0) {
	goto loser;
    }

    
    child = vfork();
    if (child < 0) goto loser;
    if (child == 0) {
	
	

	close(0);
	
	if (pipefd[1] != 1) dup2(pipefd[1], 1);
	close(2);
	close(pipefd[0]);
	


	execv(command, argv);
	
	_exit(1); 
    }
    close(pipefd[1]);
    pipefd[1] = -1;

    


    free(command);
    free(shNameArg);
    PORT_Free(argv);

    *pid = child;

    return PR_ImportPipe(pipefd[0]);

loser:
    if (pipefd[0] != -1) {
	close(pipefd[0]);
    }
    if (pipefd[1] != -1) {
	close(pipefd[1]);
    }
    free(command);
    free(shNameArg);
    PORT_Free(argv);

    return NULL;
}









void
bl_CloseUnPrelink( PRFileDesc *file, int pid)
{
    
    PR_Close(file);
    
    if (pid) {
	waitpid(pid, NULL, 0);
    }
}
#endif



static char *
mkCheckFileName(const char *libName)
{
    int ln_len = PORT_Strlen(libName);
    char *output = PORT_Alloc(ln_len+sizeof(SGN_SUFFIX));
    int index = ln_len + 1 - sizeof("."SHLIB_SUFFIX);

    if ((index > 0) &&
        (PORT_Strncmp(&libName[index],
                        "."SHLIB_SUFFIX,sizeof("."SHLIB_SUFFIX)) == 0)) {
        ln_len = index;
    }
    PORT_Memcpy(output,libName,ln_len);
    PORT_Memcpy(&output[ln_len],SGN_SUFFIX,sizeof(SGN_SUFFIX));
    return output;
}

static int
decodeInt(unsigned char *buf)
{
    return (buf[3]) | (buf[2] << 8) | (buf[1] << 16) | (buf[0] << 24);
}

static SECStatus
readItem(PRFileDesc *fd, SECItem *item)
{
    unsigned char buf[4];
    int bytesRead;


    bytesRead = PR_Read(fd, buf, 4);
    if (bytesRead != 4) {
	return SECFailure;
    }
    item->len = decodeInt(buf);

    item->data = PORT_Alloc(item->len);
    if (item->data == NULL) {
	item->len = 0;
	return SECFailure;
    }
    bytesRead = PR_Read(fd, item->data, item->len);
    if (bytesRead != item->len) {
	PORT_Free(item->data);
	item->data = NULL;
	item->len = 0;
	return SECFailure;
    }
    return SECSuccess;
}

PRBool
BLAPI_SHVerify(const char *name, PRFuncPtr addr)
{
    PRBool result = PR_FALSE; 

    
    char *shName = PR_GetLibraryFilePathname(name, addr);
    if (!shName) {
	goto loser;
    }
    result = BLAPI_SHVerifyFile(shName);

loser:
    if (shName != NULL) {
	PR_Free(shName);
    }

    return result;
}

PRBool
BLAPI_SHVerifyFile(const char *shName)
{
    char *checkName = NULL;
    PRFileDesc *checkFD = NULL;
    PRFileDesc *shFD = NULL;
    void  *hashcx = NULL;
    const SECHashObject *hashObj = NULL;
    SECItem signature = { 0, NULL, 0 };
    SECItem hash;
    int bytesRead, offset;
    SECStatus rv;
    DSAPublicKey key;
    int count;
#ifdef FREEBL_USE_PRELINK
    int pid = 0;
#endif

    PRBool result = PR_FALSE; 

    unsigned char buf[4096];
    unsigned char hashBuf[HASH_LENGTH_MAX];

    PORT_Memset(&key,0,sizeof(key));
    hash.data = hashBuf;
    hash.len = sizeof(hashBuf);

    if (!shName) {
	goto loser;
    }

    
    checkName = mkCheckFileName(shName);
    if (!checkName) {
	goto loser;
    }

    
    checkFD = PR_Open(checkName, PR_RDONLY, 0);
    if (checkFD == NULL) {
#ifdef DEBUG_SHVERIFY
        fprintf(stderr, "Failed to open the check file %s: (%d, %d)\n",
                checkName, (int)PR_GetError(), (int)PR_GetOSError());
#endif 
	goto loser;
    }

    
    bytesRead = PR_Read(checkFD, buf, 12);
    if (bytesRead != 12) {
	goto loser;
    }
    if ((buf[0] != NSS_SIGN_CHK_MAGIC1) || (buf[1] != NSS_SIGN_CHK_MAGIC2)) {
	goto loser;
    }
    if ((buf[2] != NSS_SIGN_CHK_MAJOR_VERSION) || 
				(buf[3] < NSS_SIGN_CHK_MINOR_VERSION)) {
	goto loser;
    }
#ifdef notdef
    if (decodeInt(&buf[8]) != CKK_DSA) {
	goto loser;
    }
#endif

    
    offset = decodeInt(&buf[4]);
    PR_Seek(checkFD, offset, PR_SEEK_SET);

    
    rv = readItem(checkFD,&key.params.prime);
    if (rv != SECSuccess) {
	goto loser;
    }
    rv = readItem(checkFD,&key.params.subPrime);
    if (rv != SECSuccess) {
	goto loser;
    }
    rv = readItem(checkFD,&key.params.base);
    if (rv != SECSuccess) {
	goto loser;
    }
    rv = readItem(checkFD,&key.publicValue);
    if (rv != SECSuccess) {
	goto loser;
    }
    
    rv = readItem(checkFD,&signature);
    if (rv != SECSuccess) {
	goto loser;
    }

    
    PR_Close(checkFD);
    checkFD = NULL;

    hashObj = HASH_GetRawHashObject(PQG_GetHashType(&key.params));
    if (hashObj == NULL) {
	goto loser;
    }

    
#ifdef FREEBL_USE_PRELINK
    shFD = bl_OpenUnPrelink(shName,&pid);
#else
    shFD = PR_Open(shName, PR_RDONLY, 0);
#endif
    if (shFD == NULL) {
#ifdef DEBUG_SHVERIFY
        fprintf(stderr, "Failed to open the library file %s: (%d, %d)\n",
                shName, (int)PR_GetError(), (int)PR_GetOSError());
#endif 
	goto loser;
    }

    
    hashcx = hashObj->create();
    if (hashcx == NULL) {
	goto loser;
    }
    hashObj->begin(hashcx);

    count = 0;
    while ((bytesRead = PR_Read(shFD, buf, sizeof(buf))) > 0) {
	hashObj->update(hashcx, buf, bytesRead);
	count += bytesRead;
    }
#ifdef FREEBL_USE_PRELINK
    bl_CloseUnPrelink(shFD, pid);
#else
    PR_Close(shFD);
#endif
    shFD = NULL;

    hashObj->end(hashcx, hash.data, &hash.len, hash.len);


    
    if (DSA_VerifyDigest(&key, &signature, &hash) == SECSuccess) {
	result = PR_TRUE;
    }
#ifdef DEBUG_SHVERIFY
  {
        int i,j;
        fprintf(stderr,"File %s: %d bytes\n",shName, count);
        fprintf(stderr,"  hash: %d bytes\n", hash.len);
#define STEP 10
        for (i=0; i < hash.len; i += STEP) {
           fprintf(stderr,"   ");
           for (j=0; j < STEP && (i+j) < hash.len; j++) {
                fprintf(stderr," %02x", hash.data[i+j]);
           }
           fprintf(stderr,"\n");
        }
        fprintf(stderr,"  signature: %d bytes\n", signature.len);
        for (i=0; i < signature.len; i += STEP) {
           fprintf(stderr,"   ");
           for (j=0; j < STEP && (i+j) < signature.len; j++) {
                fprintf(stderr," %02x", signature.data[i+j]);
           }
           fprintf(stderr,"\n");
        }
	fprintf(stderr,"Verified : %s\n",result?"TRUE": "FALSE");
    }
#endif 


loser:
    if (checkName != NULL) {
	PORT_Free(checkName);
    }
    if (checkFD != NULL) {
	PR_Close(checkFD);
    }
    if (shFD != NULL) {
	PR_Close(shFD);
    }
    if (hashcx != NULL) {
	if (hashObj) {
	    hashObj->destroy(hashcx,PR_TRUE);
	}
    }
    if (signature.data != NULL) {
	PORT_Free(signature.data);
    }
    if (key.params.prime.data != NULL) {
	PORT_Free(key.params.prime.data);
    }
    if (key.params.subPrime.data != NULL) {
	PORT_Free(key.params.subPrime.data);
    }
    if (key.params.base.data != NULL) {
	PORT_Free(key.params.base.data);
    }
    if (key.publicValue.data != NULL) {
	PORT_Free(key.publicValue.data);
    }

    return result;
}

PRBool
BLAPI_VerifySelf(const char *name)
{
    if (name == NULL) {
	



	return PR_TRUE;
    }
    return BLAPI_SHVerify(name, (PRFuncPtr) decodeInt);
}

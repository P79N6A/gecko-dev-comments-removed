









































#ifdef XP_UNIX
#define USES_LINKS 1
#endif

#include "nspr.h"
#include <stdio.h>
#include "nss.h"
#include "secutil.h"
#include "cert.h"
#include "pk11func.h"

#include "plgetopt.h"
#include "pk11sdr.h"
#include "shsign.h"
#include "pk11pqg.h"

#ifdef USES_LINKS
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

static void
usage (char *program_name)
{
    PRFileDesc *pr_stderr;

    pr_stderr = PR_STDERR;
    PR_fprintf (pr_stderr,
      "Usage:%s [-v] [-o outfile] [-d dbdir] [-f pwfile] [-p pwd]\n"
      "      -i shared_library_name\n", program_name);
}

static char *
mkoutput(const char *input)
{
    int in_len = PORT_Strlen(input);
    char *output = PORT_Alloc(in_len+sizeof(SGN_SUFFIX));
    int index = in_len + 1 - sizeof("."SHLIB_SUFFIX);

    if ((index > 0) && 
	(PORT_Strncmp(&input[index],
			"."SHLIB_SUFFIX,sizeof("."SHLIB_SUFFIX)) == 0)) {
	in_len = index;
    }
    PORT_Memcpy(output,input,in_len);
    PORT_Memcpy(&output[in_len],SGN_SUFFIX,sizeof(SGN_SUFFIX));
    return output;
}


static void
lperror(const char *string)
{
     int errNum = PORT_GetError();
     const char *error = SECU_Strerror(errNum);
     fprintf(stderr,"%s: %s\n",string, error);
}

static void
encodeInt(unsigned char *buf, int val)
{
    buf[3] = (val >> 0) & 0xff;
    buf[2] = (val >>  8) & 0xff;
    buf[1] = (val >> 16) & 0xff;
    buf[0] = (val >> 24) & 0xff;
    return;
}

static SECStatus 
writeItem(PRFileDesc *fd, SECItem *item, char *file)
{
    unsigned char buf[4];
    int bytesWritten;

    encodeInt(buf,item->len);
    bytesWritten = PR_Write(fd,buf, 4);
    if (bytesWritten != 4) {
	lperror(file);
	return SECFailure;
    }
    bytesWritten = PR_Write(fd, item->data, item->len);
    if (bytesWritten != item->len) {
	lperror(file);
	return SECFailure;
    }
    return SECSuccess;
}


int
main (int argc, char **argv)
{
    int		 retval = 1;  
    SECStatus	 rv;
    PLOptState	*optstate;
    char	*program_name;
    const char  *input_file = NULL; 	
    char  *output_file = NULL;	
    PRBool      verbose = PR_FALSE;
    SECKEYPrivateKey *privk = NULL;
    SECKEYPublicKey *pubk = NULL;
    PK11SlotInfo *slot = NULL;
    PRFileDesc *fd;
    int bytesRead;
    int bytesWritten;
    unsigned char file_buf[512];
    unsigned char hash_buf[SHA1_LENGTH];
    unsigned char sign_buf[40]; 
    SECItem hash,sign;
    PK11Context *hashcx = NULL;
    int ks, count=0;
    int keySize = 1024;
    PQGParams *pqgParams = NULL;
    PQGVerify *pqgVerify = NULL;
    const char *nssDir = NULL;
    secuPWData  pwdata = { PW_NONE, 0 };
#ifdef USES_LINKS
    int ret;
    struct stat stat_buf;
    char link_buf[MAXPATHLEN+1];
    char *link_file = NULL;
#endif

    hash.len = sizeof(hash_buf); hash.data = hash_buf;
    sign.len = sizeof(sign_buf); sign.data = sign_buf;

    program_name = PL_strrchr(argv[0], '/');
    program_name = program_name ? (program_name + 1) : argv[0];

    optstate = PL_CreateOptState (argc, argv, "d:f:i:o:p:v");
    if (optstate == NULL) {
	SECU_PrintError (program_name, "PL_CreateOptState failed");
	return 1;
    }

    while (PL_GetNextOpt (optstate) == PL_OPT_OK) {
	switch (optstate->option) {
#ifdef notdef
	  case '?':
	    short_usage (program_name);
	    return 0;

	  case 'H':
	    long_usage (program_name);
	    return 0;
#endif

	  case 'd':
	    nssDir = optstate->value;
	    break;

          case 'i':
            input_file = optstate->value;
            break;

          case 'o':
            output_file = PORT_Strdup(optstate->value);
            break;

          case 'f':
            pwdata.source = PW_FROMFILE;
            pwdata.data = PORT_Strdup(optstate->value);
            break;

          case 'p':
            pwdata.source = PW_PLAINTEXT;
            pwdata.data = PORT_Strdup(optstate->value);
            break;

          case 'v':
            verbose = PR_TRUE;
            break;
	}
    }

    if (input_file == NULL) {
	usage(program_name);
	return 1;
    }

    


    PK11_SetPasswordFunc(SECU_GetModulePassword);

    if (nssDir) {
        rv = NSS_Init(nssDir);
        if (rv != SECSuccess) {
            rv = NSS_NoDB_Init("");
        }
    } else {
        rv = NSS_NoDB_Init("");
    }
    
    if (rv != SECSuccess) {
	lperror("NSS_Init failed");
	goto prdone;
    }
    
    
    slot = PK11_GetBestSlot(CKM_DSA,&pwdata);
    if (slot == NULL) {
	lperror("CKM_DSA");
	goto loser;
	
    }
    printf("Generating DSA Key Pair...."); fflush(stdout);
    ks = PQG_PBITS_TO_INDEX(keySize);
    rv = PK11_PQG_ParamGen(ks,&pqgParams, &pqgVerify);
    if (rv != SECSuccess) {
	lperror("Generating PQG Params");
	goto loser;
    }
    privk = PK11_GenerateKeyPair(slot, CKM_DSA_KEY_PAIR_GEN, pqgParams, &pubk, 
						PR_FALSE, PR_TRUE, &pwdata);
    if (privk == NULL) {
	lperror("Generating DSA Key");
	goto loser;
    }

    printf("done\n");

    
    fd = PR_OpenFile(input_file,PR_RDONLY,0);
    if (fd == NULL ) {
	lperror(input_file);
	goto loser;
    }
#ifdef USES_LINKS
    ret = lstat(input_file, &stat_buf);
    if (ret < 0) {
	perror(input_file);
	goto loser;
    }
    if (S_ISLNK(stat_buf.st_mode)) {
	char *dirpath,*dirend;
	ret = readlink(input_file, link_buf, sizeof(link_buf) - 1);
	if (ret < 0) {
	   perror(input_file);
	   goto loser;
	}
	link_buf[ret] = 0;
	link_file = mkoutput(input_file);
	
	dirpath = PORT_Strdup(input_file);
	dirend = PORT_Strrchr(dirpath, '/');
	if (dirend) {
	    *dirend = '\0';
	    ret = chdir(dirpath);
	    if (ret < 0) {
		perror(dirpath);
		goto loser;
	    }
	}
	PORT_Free(dirpath);
	input_file = link_buf;
	
	dirend = PORT_Strrchr(link_file, '/');
	if (dirend) {
	    link_file = dirend + 1;
	}
    }
#endif
    if (output_file == NULL) {
	output_file = mkoutput(input_file);
    }

    hashcx = PK11_CreateDigestContext(SEC_OID_SHA1);
    if (hashcx == NULL) {
	lperror("SHA1 Digest Create");
	goto loser;
    }

    
    while ((bytesRead = PR_Read(fd,file_buf,sizeof(file_buf))) > 0) {
	PK11_DigestOp(hashcx,file_buf,bytesRead);
	count += bytesRead;
    }

    PR_Close(fd);
    fd = NULL;
    if (bytesRead < 0) {
	lperror(input_file);
	goto loser;
    }


    PK11_DigestFinal(hashcx, hash.data, &hash.len, hash.len);

    if (hash.len != SHA1_LENGTH) {
	fprintf(stderr, "Digest length was not correct\n");
	goto loser;
    }

    
    rv = PK11_Sign(privk,&sign,&hash);
    if (rv != SECSuccess) {
	lperror("Signing");
	goto loser;
    }

    if (verbose) {
	int i,j;
	fprintf(stderr,"Library File: %s %d bytes\n",input_file, count);
	fprintf(stderr,"Check File: %s\n",output_file);
#ifdef USES_LINKS
	if (link_file) {
	    fprintf(stderr,"Link: %s\n",link_file);
	}
#endif
	fprintf(stderr,"  hash: %d bytes\n", hash.len);
#define STEP 10
	for (i=0; i < hash.len; i += STEP) {
	   fprintf(stderr,"   ");
	   for (j=0; j < STEP && (i+j) < hash.len; j++) {
		fprintf(stderr," %02x", hash.data[i+j]);
	   }
	   fprintf(stderr,"\n");
	}
	fprintf(stderr,"  signature: %d bytes\n", sign.len);
	for (i=0; i < sign.len; i += STEP) {
	   fprintf(stderr,"   ");
	   for (j=0; j < STEP && (i+j) < sign.len; j++) {
		fprintf(stderr," %02x", sign.data[i+j]);
	   }
	   fprintf(stderr,"\n");
	}
    }

    
    fd = PR_OpenFile(output_file,PR_WRONLY|PR_CREATE_FILE|PR_TRUNCATE,0666);
    if (fd == NULL ) {
	lperror(output_file);
	goto loser;
    }

    







    file_buf[0] = NSS_SIGN_CHK_MAGIC1;
    file_buf[1] = NSS_SIGN_CHK_MAGIC2;
    file_buf[2] = NSS_SIGN_CHK_MAJOR_VERSION;
    file_buf[3] = NSS_SIGN_CHK_MINOR_VERSION;
    encodeInt(&file_buf[4],12);			
    encodeInt(&file_buf[8],CKK_DSA);
    bytesWritten = PR_Write(fd,file_buf, 12);
    if (bytesWritten != 12) {
	lperror(output_file);
	goto loser;
    }

    rv = writeItem(fd,&pubk->u.dsa.params.prime,output_file);
    if (rv != SECSuccess) goto loser;
    rv = writeItem(fd,&pubk->u.dsa.params.subPrime,output_file);
    if (rv != SECSuccess) goto loser;
    rv = writeItem(fd,&pubk->u.dsa.params.base,output_file);
    if (rv != SECSuccess) goto loser;
    rv = writeItem(fd,&pubk->u.dsa.publicValue,output_file);
    if (rv != SECSuccess) goto loser;
    rv = writeItem(fd,&sign,output_file);
    if (rv != SECSuccess) goto loser;

    PR_Close(fd);

#ifdef USES_LINKS
    if (link_file) {
	(void)unlink(link_file);
	ret = symlink(output_file, link_file);
	if (ret < 0) {
	   perror(link_file);
	   goto loser;
	}
    }
#endif

    retval = 0;

loser:
    if (hashcx) {
        PK11_DestroyContext(hashcx, PR_TRUE);
    }
    if (privk) {
        SECKEY_DestroyPrivateKey(privk);
    }
    if (pubk) {
        SECKEY_DestroyPublicKey(pubk);
    }
    if (slot) {
        PK11_FreeSlot(slot);
    }
    if (pwdata.data) {
        PORT_Free(pwdata.data);
    }
    if (NSS_Shutdown() != SECSuccess) {
	exit(1);
    }

prdone:
    PR_Cleanup ();
    return retval;
}

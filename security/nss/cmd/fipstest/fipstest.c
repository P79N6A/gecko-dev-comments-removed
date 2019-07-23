



































#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "secitem.h"
#include "blapi.h"
#include "nss.h"
#include "secerr.h"
#include "secder.h"
#include "secdig.h"
#include "keythi.h"
#include "ec.h"
#include "hasht.h"
#include "lowkeyi.h"
#include "softoken.h"

#if 0
#include "../../lib/freebl/mpi/mpi.h"
#endif

#ifdef NSS_ENABLE_ECC
extern SECStatus
EC_DecodeParams(const SECItem *encodedParams, ECParams **ecparams);
extern SECStatus
EC_CopyParams(PRArenaPool *arena, ECParams *dstParams,
              const ECParams *srcParams);
#endif

#define ENCRYPT 1
#define DECRYPT 0
#define BYTE unsigned char
#define DEFAULT_RSA_PUBLIC_EXPONENT   0x10001
#define RSA_MAX_TEST_MODULUS_BITS     4096
#define RSA_MAX_TEST_MODULUS_BYTES    RSA_MAX_TEST_MODULUS_BITS/8
#define RSA_MAX_TEST_EXPONENT_BYTES   8
#define PQG_TEST_SEED_BYTES           20

SECStatus
hex_to_byteval(const char *c2, unsigned char *byteval)
{
    int i;
    unsigned char offset;
    *byteval = 0;
    for (i=0; i<2; i++) {
	if (c2[i] >= '0' && c2[i] <= '9') {
	    offset = c2[i] - '0';
	    *byteval |= offset << 4*(1-i);
	} else if (c2[i] >= 'a' && c2[i] <= 'f') {
	    offset = c2[i] - 'a';
	    *byteval |= (offset + 10) << 4*(1-i);
	} else if (c2[i] >= 'A' && c2[i] <= 'F') {
	    offset = c2[i] - 'A';
	    *byteval |= (offset + 10) << 4*(1-i);
	} else {
	    return SECFailure;
	}
    }
    return SECSuccess;
}

SECStatus
byteval_to_hex(unsigned char byteval, char *c2, char a)
{
    int i;
    unsigned char offset;
    for (i=0; i<2; i++) {
	offset = (byteval >> 4*(1-i)) & 0x0f;
	if (offset < 10) {
	    c2[i] = '0' + offset;
	} else {
	    c2[i] = a + offset - 10;
	}
    }
    return SECSuccess;
}

void
to_hex_str(char *str, const unsigned char *buf, unsigned int len)
{
    unsigned int i;
    for (i=0; i<len; i++) {
	byteval_to_hex(buf[i], &str[2*i], 'a');
    }
    str[2*len] = '\0';
}

void
to_hex_str_cap(char *str, const unsigned char *buf, unsigned int len)
{
    unsigned int i;
    for (i=0; i<len; i++) {
	byteval_to_hex(buf[i], &str[2*i], 'A');
    }
    str[2*len] = '\0';
}






PRBool
from_hex_str(unsigned char *buf, unsigned int len, const char *str)
{
    unsigned int nxdigit;  
    unsigned int i;  
    unsigned int j;  

    
    nxdigit = 0;
    for (nxdigit = 0; isxdigit(str[nxdigit]); nxdigit++) {
	
    }
    if (nxdigit == 0) {
	return PR_FALSE;
    }
    if (nxdigit > 2*len) {
	



	for (j = 0; j < nxdigit-2*len; j++) {
	    if (str[j] != '0') {
		return PR_FALSE;
	    }
	}
	
	str += nxdigit-2*len;
	nxdigit = 2*len;
    }
    for (i=0, j=0; i< len; i++) {
	if (2*i < 2*len-nxdigit) {
	    
	    if (2*i+1 < 2*len-nxdigit) {
		buf[i] = 0;
	    } else {
		char tmp[2];
		tmp[0] = '0';
		tmp[1] = str[j];
		hex_to_byteval(tmp, &buf[i]);
		j++;
	    }
	} else {
	    hex_to_byteval(&str[j], &buf[i]);
	    j += 2;
	}
    }
    return PR_TRUE;
}

SECStatus
tdea_encrypt_buf(
    int mode,
    const unsigned char *key, 
    const unsigned char *iv,
    unsigned char *output, unsigned int *outputlen, unsigned int maxoutputlen,
    const unsigned char *input, unsigned int inputlen)
{
    SECStatus rv = SECFailure;
    DESContext *cx;
    unsigned char doublecheck[8*20];  
    unsigned int doublechecklen = 0;

    cx = DES_CreateContext(key, iv, mode, PR_TRUE);
    if (cx == NULL) {
        goto loser;
    }
    rv = DES_Encrypt(cx, output, outputlen, maxoutputlen, input, inputlen);
    if (rv != SECSuccess) {
        goto loser;
    }
    if (*outputlen != inputlen) {
        goto loser;
    }
    DES_DestroyContext(cx, PR_TRUE);
    cx = NULL;

    



    cx = DES_CreateContext(key, iv, mode, PR_FALSE);
    if (cx == NULL) {
        goto loser;
    }
    rv = DES_Decrypt(cx, doublecheck, &doublechecklen, sizeof doublecheck,
                    output, *outputlen);
    if (rv != SECSuccess) {
        goto loser;
    }
    if (doublechecklen != *outputlen) {
        goto loser;
    }
    DES_DestroyContext(cx, PR_TRUE);
    cx = NULL;
    if (memcmp(doublecheck, input, inputlen) != 0) {
        goto loser;
    }
    rv = SECSuccess;

loser:
    if (cx != NULL) {
        DES_DestroyContext(cx, PR_TRUE);
    }
    return rv;
}

SECStatus
tdea_decrypt_buf(
    int mode,
    const unsigned char *key, 
    const unsigned char *iv,
    unsigned char *output, unsigned int *outputlen, unsigned int maxoutputlen,
    const unsigned char *input, unsigned int inputlen)
{
    SECStatus rv = SECFailure;
    DESContext *cx;
    unsigned char doublecheck[8*20];  
    unsigned int doublechecklen = 0;

    cx = DES_CreateContext(key, iv, mode, PR_FALSE);
    if (cx == NULL) {
        goto loser;
    }
    rv = DES_Decrypt(cx, output, outputlen, maxoutputlen,
                    input, inputlen);
    if (rv != SECSuccess) {
        goto loser;
    }
    if (*outputlen != inputlen) {
        goto loser;
    }
    DES_DestroyContext(cx, PR_TRUE);
    cx = NULL;

    



    cx = DES_CreateContext(key, iv, mode, PR_TRUE);
    if (cx == NULL) {
        goto loser;
    }
    rv = DES_Encrypt(cx, doublecheck, &doublechecklen, sizeof doublecheck,
        output, *outputlen);
    if (rv != SECSuccess) {
        goto loser;
    }
    if (doublechecklen != *outputlen) {
        goto loser;
    }
    DES_DestroyContext(cx, PR_TRUE);
    cx = NULL;
    if (memcmp(doublecheck, input, inputlen) != 0) {
        goto loser;
    }
    rv = SECSuccess;

loser:
    if (cx != NULL) {
        DES_DestroyContext(cx, PR_TRUE);
    }
    return rv;
}












void
tdea_kat_mmt(char *reqfn)
{
    char buf[180];      



    FILE *req;       
    FILE *resp;      
    int i, j;
    int mode;           
    int crypt = DECRYPT;    
    unsigned char key[24];              
    unsigned int numKeys = 0;
    unsigned char iv[8];		
    unsigned char plaintext[8*20];     
    unsigned int plaintextlen;
    unsigned char ciphertext[8*20];     
    unsigned int ciphertextlen;
    SECStatus rv;

    req = fopen(reqfn, "r");
    resp = stdout;
    while (fgets(buf, sizeof buf, req) != NULL) {
        
        if (buf[0] == '#' || buf[0] == '\n') {
            fputs(buf, resp);
            continue;
        }
        
        if (buf[0] == '[') {
            if (strncmp(&buf[1], "ENCRYPT", 7) == 0) {
                crypt = ENCRYPT;
            } else {
                crypt = DECRYPT;
            }
            fputs(buf, resp);
            continue;
        }
        
        if (strncmp(&buf[0], "NumKeys", 7) == 0) {
            i = 7;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            numKeys = buf[i];
            fputs(buf, resp);
            continue;
        }
        
        if (strncmp(buf, "COUNT", 5) == 0) {
            
            mode = NSS_DES_EDE3;
            
            memset(key, 0, sizeof key);
            memset(iv, 0, sizeof iv);
            memset(plaintext, 0, sizeof plaintext);
            plaintextlen = 0;
            memset(ciphertext, 0, sizeof ciphertext);
            ciphertextlen = 0;
            fputs(buf, resp);
            continue;
        }
        if (numKeys == 0) {
            if (strncmp(buf, "KEYs", 4) == 0) {
                i = 4;
                while (isspace(buf[i]) || buf[i] == '=') {
                    i++;
                }
                for (j=0; isxdigit(buf[i]); i+=2,j++) {
                    hex_to_byteval(&buf[i], &key[j]);
                    key[j+8] = key[j];
                    key[j+16] = key[j];
                }
                fputs(buf, resp);
                continue;
            }
        } else {
            
            if (strncmp(buf, "KEY1", 4) == 0) {
                i = 4;
                while (isspace(buf[i]) || buf[i] == '=') {
                    i++;
                }
                for (j=0; isxdigit(buf[i]); i+=2,j++) {
                    hex_to_byteval(&buf[i], &key[j]);
                }
                fputs(buf, resp);
                continue;
            }
            
            if (strncmp(buf, "KEY2", 4) == 0) {
                i = 4;
                while (isspace(buf[i]) || buf[i] == '=') {
                    i++;
                }
                for (j=8; isxdigit(buf[i]); i+=2,j++) {
                    hex_to_byteval(&buf[i], &key[j]);
                }
                fputs(buf, resp);
                continue;
            }
            
            if (strncmp(buf, "KEY3", 4) == 0) {
                i = 4;
                while (isspace(buf[i]) || buf[i] == '=') {
                    i++;
                }
                for (j=16; isxdigit(buf[i]); i+=2,j++) {
                    hex_to_byteval(&buf[i], &key[j]);
                }
                fputs(buf, resp);
                continue;
            }
        }

        
        if (strncmp(buf, "IV", 2) == 0) {
            mode = NSS_DES_EDE3_CBC;
            i = 2;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=0; j<sizeof iv; i+=2,j++) {
                hex_to_byteval(&buf[i], &iv[j]);
            }
            fputs(buf, resp);
            continue;
        }

        
        if (strncmp(buf, "PLAINTEXT", 9) == 0) {
            
            if (crypt != ENCRYPT) {
                goto loser;
            }
            i = 9;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=0; isxdigit(buf[i]); i+=2,j++) {
                hex_to_byteval(&buf[i], &plaintext[j]);
            }
            plaintextlen = j;
            rv = tdea_encrypt_buf(mode, key,
                            (mode == NSS_DES_EDE3) ? NULL : iv,
                            ciphertext, &ciphertextlen, sizeof ciphertext,
                            plaintext, plaintextlen);
            if (rv != SECSuccess) {
                goto loser;
            }
    
            fputs(buf, resp);
            fputs("CIPHERTEXT = ", resp);
            to_hex_str(buf, ciphertext, ciphertextlen);
            fputs(buf, resp);
            fputc('\n', resp);
            continue;
        }
        
        if (strncmp(buf, "CIPHERTEXT", 10) == 0) {
            
            if (crypt != DECRYPT) {
                goto loser;
            }
 
            i = 10;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=0; isxdigit(buf[i]); i+=2,j++) {
                hex_to_byteval(&buf[i], &ciphertext[j]);
            }
            ciphertextlen = j;
 
            rv = tdea_decrypt_buf(mode, key,
                            (mode == NSS_DES_EDE3) ? NULL : iv,
                            plaintext, &plaintextlen, sizeof plaintext,
                            ciphertext, ciphertextlen);
            if (rv != SECSuccess) {
                goto loser;
            }
 
            fputs(buf, resp);
            fputs("PLAINTEXT = ", resp);
            to_hex_str(buf, plaintext, plaintextlen);
            fputs(buf, resp);
            fputc('\n', resp);
            continue;
        }
    }

loser:
    fclose(req);
}




BYTE odd_parity( BYTE in)
{
    BYTE out = in;
    in ^= in >> 4;
    in ^= in >> 2;
    in ^= in >> 1;
    return (BYTE)(out ^ !(in & 1));
}





void
tdea_mct_next_keys(unsigned char *key,
    const unsigned char *text_2, const unsigned char *text_1, 
    const unsigned char *text, unsigned int numKeys)
{
    int k;

    
    for (k=0; k<8; k++) {
        key[k] ^= text[k];
    }
    
    if (numKeys == 2 || numKeys == 3)  {
        
        for (k=8; k<16; k++) {
            
            key[k] ^= text_1[k-8];
        }
    } else {
        
        for (k=8; k<16; k++) {
            
            key[k] = key[k-8];
        }
    }
    
    if (numKeys == 1 || numKeys == 2) {
        
        for (k=16; k<24; k++) {
            
            key[k] = key[k-16];
        }
    } else {
         
        for (k=16; k<24; k++) {
            
            key[k] ^= text_2[k-16];
        }
    }
                
    for (k=0; k<24; k++) {
        key[k] = odd_parity(key[k]);
    }
}











 void                                                       
tdea_mct_test(int mode, unsigned char* key, unsigned int numKeys, 
              unsigned int crypt, unsigned char* inputtext, 
              unsigned int inputlength, unsigned char* iv, FILE *resp) { 

    int i, j;
    unsigned char outputtext_1[8];      
    unsigned char outputtext_2[8];      
    char buf[80];       
    unsigned int outputlen;
    unsigned char outputtext[8];
    
        
    SECStatus rv;

    if (mode == NSS_DES_EDE3 && iv != NULL) {
        printf("IV must be NULL for NSS_DES_EDE3 mode");
        goto loser;
    } else if (mode == NSS_DES_EDE3_CBC && iv == NULL) {
        printf("IV must not be NULL for NSS_DES_EDE3_CBC mode");
        goto loser;
    }

    
    for (i=0; i<400; i++) {
                
        
        sprintf(buf, "COUNT = %d\n", i);
        fputs(buf, resp);
        
        fputs("KEY1 = ", resp);
        to_hex_str(buf, key, 8);
        fputs(buf, resp);
        fputc('\n', resp);
        
        fputs("KEY2 = ", resp);
        to_hex_str(buf, &key[8], 8);
        fputs(buf, resp);
        fputc('\n', resp);
        
        fputs("KEY3 = ", resp);
        to_hex_str(buf, &key[16], 8);
        fputs(buf, resp);
        fputc('\n', resp);
        if (mode == NSS_DES_EDE3_CBC) {
            
            fputs("IV = ", resp);
            to_hex_str(buf, iv, 8);
            fputs(buf, resp);
            fputc('\n', resp);
        }
        if (crypt == ENCRYPT) {
            
            fputs("PLAINTEXT = ", resp);
        } else {
            
            fputs("CIPHERTEXT = ", resp);
        }

        to_hex_str(buf, inputtext, inputlength);
        fputs(buf, resp);
        fputc('\n', resp);

        
        for (j=0; j<10000; j++) {

            outputlen = 0;
            if (crypt == ENCRYPT) {
                
                rv = tdea_encrypt_buf(mode, key,
                            (mode == NSS_DES_EDE3) ? NULL : iv,
                            outputtext, &outputlen, 8,
                            inputtext, 8);
            } else {
                
                rv = tdea_decrypt_buf(mode, key,
                            (mode == NSS_DES_EDE3) ? NULL : iv,
                            outputtext, &outputlen, 8,
                            inputtext, 8);
            }

            if (rv != SECSuccess) {
                goto loser;
            }
            if (outputlen != inputlength) {
                goto loser;
            }

            if (mode == NSS_DES_EDE3_CBC) {
                if (crypt == ENCRYPT) {
                    if (j == 0) {
                        
                        memcpy(inputtext, iv, 8);
                    } else {
                        
                        memcpy(inputtext, outputtext_1, 8);
                    }
                    
                    memcpy(iv, outputtext, 8);
                    if (j != 9999) {
                        
                        memcpy(outputtext_1, outputtext, 8);
                    }
                } else { 
                    
                    memcpy(iv, inputtext, 8);
                    
                    memcpy(inputtext, outputtext, 8);
                }
            } else {
                
                memcpy(inputtext, outputtext, 8);
            }

            
            if (j==9997) memcpy(outputtext_2, outputtext, 8);
            if (j==9998) memcpy(outputtext_1, outputtext, 8);
            
        }


        if (crypt == ENCRYPT) {
            
            fputs("CIPHERTEXT = ", resp);
        } else {
            
            fputs("PLAINTEXT = ", resp);
        }
        to_hex_str(buf, outputtext, 8);
        fputs(buf, resp);
        fputc('\n', resp);

        


        tdea_mct_next_keys(key, outputtext_2, 
                           outputtext_1, outputtext, numKeys);

        if (mode == NSS_DES_EDE3_CBC) {
            
            if (crypt == ENCRYPT) {
                
                
            } else {
                
                
                
            }
        } else {
            
            memcpy(inputtext, outputtext, 8);
        }
        
        fputc('\n', resp);
    }

loser:
    return;
}










void
tdea_mct(int mode, char *reqfn)
{
    int i, j;
    char buf[80];    
    FILE *req;       
    FILE *resp;      
    unsigned int crypt = 0;    
    unsigned char key[24];              
    unsigned int numKeys = 0;
    unsigned char plaintext[8];        
    unsigned char ciphertext[8];       
    unsigned char iv[8];

    
    memset(key, 0, sizeof key);
    memset(plaintext, 0, sizeof plaintext);
    memset(ciphertext, 0, sizeof ciphertext);
    memset(iv, 0, sizeof iv);

    req = fopen(reqfn, "r");
    resp = stdout;
    while (fgets(buf, sizeof buf, req) != NULL) {
        
        if (buf[0] == '#' || buf[0] == '\n') {
            fputs(buf, resp);
            continue;
        }
        
        if (buf[0] == '[') {
            if (strncmp(&buf[1], "ENCRYPT", 7) == 0) {
                crypt = ENCRYPT;
            } else {
                crypt = DECRYPT;
           }
           fputs(buf, resp);
           continue;
        }
        
        if (strncmp(&buf[0], "NumKeys", 7) == 0) {
            i = 7;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            numKeys = atoi(&buf[i]);
            continue;
        }
        
        if (strncmp(buf, "KEY1", 4) == 0) {
            i = 4;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=0; isxdigit(buf[i]); i+=2,j++) {
                hex_to_byteval(&buf[i], &key[j]);
            }
            continue;
        }
        
        if (strncmp(buf, "KEY2", 4) == 0) {
            i = 4;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=8; isxdigit(buf[i]); i+=2,j++) {
                hex_to_byteval(&buf[i], &key[j]);
            }
            continue;
        }
        
        if (strncmp(buf, "KEY3", 4) == 0) {
            i = 4;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=16; isxdigit(buf[i]); i+=2,j++) {
                hex_to_byteval(&buf[i], &key[j]);
            }
            continue;
        }

        
        if (strncmp(buf, "IV", 2) == 0) {
            i = 2;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=0; j<sizeof iv; i+=2,j++) {
                hex_to_byteval(&buf[i], &iv[j]);
            }
            continue;
        }

       
       if (strncmp(buf, "PLAINTEXT", 9) == 0) {

            
            if (crypt != ENCRYPT) {
                goto loser;
            }
            
            i = 9;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=0; j<sizeof plaintext; i+=2,j++) {
                hex_to_byteval(&buf[i], &plaintext[j]);
            }                                     

            
            if (mode==NSS_DES_EDE3) {
                tdea_mct_test(NSS_DES_EDE3, key, numKeys, crypt, plaintext, sizeof plaintext, NULL, resp);
            } else {
                tdea_mct_test(NSS_DES_EDE3_CBC, key, numKeys, crypt, plaintext, sizeof plaintext, iv, resp);
            }
            continue;
        }
        
        if (strncmp(buf, "CIPHERTEXT", 10) == 0) {
            
            if (crypt != DECRYPT) {
                goto loser;
            }
            
            i = 10;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=0; isxdigit(buf[i]); i+=2,j++) {
                hex_to_byteval(&buf[i], &ciphertext[j]);
            }
            
            
            if (mode==NSS_DES_EDE3) {
                tdea_mct_test(NSS_DES_EDE3, key, numKeys, crypt, ciphertext, sizeof ciphertext, NULL, resp); 
            } else {
                tdea_mct_test(NSS_DES_EDE3_CBC, key, numKeys, crypt, ciphertext, sizeof ciphertext, iv, resp); 
            }
            continue;
        }
    }

loser:
    fclose(req);
}


SECStatus
aes_encrypt_buf(
    int mode,
    const unsigned char *key, unsigned int keysize,
    const unsigned char *iv,
    unsigned char *output, unsigned int *outputlen, unsigned int maxoutputlen,
    const unsigned char *input, unsigned int inputlen)
{
    SECStatus rv = SECFailure;
    AESContext *cx;
    unsigned char doublecheck[10*16];  
    unsigned int doublechecklen = 0;

    cx = AES_CreateContext(key, iv, mode, PR_TRUE, keysize, 16);
    if (cx == NULL) {
	goto loser;
    }
    rv = AES_Encrypt(cx, output, outputlen, maxoutputlen, input, inputlen);
    if (rv != SECSuccess) {
	goto loser;
    }
    if (*outputlen != inputlen) {
	goto loser;
    }
    AES_DestroyContext(cx, PR_TRUE);
    cx = NULL;

    



    cx = AES_CreateContext(key, iv, mode, PR_FALSE, keysize, 16);
    if (cx == NULL) {
	goto loser;
    }
    rv = AES_Decrypt(cx, doublecheck, &doublechecklen, sizeof doublecheck,
	output, *outputlen);
    if (rv != SECSuccess) {
	goto loser;
    }
    if (doublechecklen != *outputlen) {
	goto loser;
    }
    AES_DestroyContext(cx, PR_TRUE);
    cx = NULL;
    if (memcmp(doublecheck, input, inputlen) != 0) {
	goto loser;
    }
    rv = SECSuccess;

loser:
    if (cx != NULL) {
	AES_DestroyContext(cx, PR_TRUE);
    }
    return rv;
}

SECStatus
aes_decrypt_buf(
    int mode,
    const unsigned char *key, unsigned int keysize,
    const unsigned char *iv,
    unsigned char *output, unsigned int *outputlen, unsigned int maxoutputlen,
    const unsigned char *input, unsigned int inputlen)
{
    SECStatus rv = SECFailure;
    AESContext *cx;
    unsigned char doublecheck[10*16];  
    unsigned int doublechecklen = 0;

    cx = AES_CreateContext(key, iv, mode, PR_FALSE, keysize, 16);
    if (cx == NULL) {
	goto loser;
    }
    rv = AES_Decrypt(cx, output, outputlen, maxoutputlen,
	input, inputlen);
    if (rv != SECSuccess) {
	goto loser;
    }
    if (*outputlen != inputlen) {
	goto loser;
    }
    AES_DestroyContext(cx, PR_TRUE);
    cx = NULL;

    



    cx = AES_CreateContext(key, iv, mode, PR_TRUE, keysize, 16);
    if (cx == NULL) {
	goto loser;
    }
    rv = AES_Encrypt(cx, doublecheck, &doublechecklen, sizeof doublecheck,
	output, *outputlen);
    if (rv != SECSuccess) {
	goto loser;
    }
    if (doublechecklen != *outputlen) {
	goto loser;
    }
    AES_DestroyContext(cx, PR_TRUE);
    cx = NULL;
    if (memcmp(doublecheck, input, inputlen) != 0) {
	goto loser;
    }
    rv = SECSuccess;

loser:
    if (cx != NULL) {
	AES_DestroyContext(cx, PR_TRUE);
    }
    return rv;
}












void
aes_kat_mmt(char *reqfn)
{
    char buf[512];      



    FILE *aesreq;       
    FILE *aesresp;      
    int i, j;
    int mode;           
    int encrypt = 0;    
    unsigned char key[32];              
    unsigned int keysize;
    unsigned char iv[16];		
    unsigned char plaintext[10*16];     
    unsigned int plaintextlen;
    unsigned char ciphertext[10*16];    
    unsigned int ciphertextlen;
    SECStatus rv;

    aesreq = fopen(reqfn, "r");
    aesresp = stdout;
    while (fgets(buf, sizeof buf, aesreq) != NULL) {
	
	if (buf[0] == '#' || buf[0] == '\n') {
	    fputs(buf, aesresp);
	    continue;
	}
	
	if (buf[0] == '[') {
	    if (strncmp(&buf[1], "ENCRYPT", 7) == 0) {
		encrypt = 1;
	    } else {
		encrypt = 0;
	    }
	    fputs(buf, aesresp);
	    continue;
	}
	
	if (strncmp(buf, "COUNT", 5) == 0) {
	    mode = NSS_AES;
	    
	    memset(key, 0, sizeof key);
	    keysize = 0;
	    memset(iv, 0, sizeof iv);
	    memset(plaintext, 0, sizeof plaintext);
	    plaintextlen = 0;
	    memset(ciphertext, 0, sizeof ciphertext);
	    ciphertextlen = 0;
	    fputs(buf, aesresp);
	    continue;
	}
	
	if (strncmp(buf, "KEY", 3) == 0) {
	    i = 3;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    for (j=0; isxdigit(buf[i]); i+=2,j++) {
		hex_to_byteval(&buf[i], &key[j]);
	    }
	    keysize = j;
	    fputs(buf, aesresp);
	    continue;
	}
	
	if (strncmp(buf, "IV", 2) == 0) {
	    mode = NSS_AES_CBC;
	    i = 2;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    for (j=0; j<sizeof iv; i+=2,j++) {
		hex_to_byteval(&buf[i], &iv[j]);
	    }
	    fputs(buf, aesresp);
	    continue;
	}
	
	if (strncmp(buf, "PLAINTEXT", 9) == 0) {
	    
	    if (!encrypt) {
		goto loser;
	    }

	    i = 9;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    for (j=0; isxdigit(buf[i]); i+=2,j++) {
		hex_to_byteval(&buf[i], &plaintext[j]);
	    }
	    plaintextlen = j;

	    rv = aes_encrypt_buf(mode, key, keysize,
		(mode == NSS_AES) ? NULL : iv,
		ciphertext, &ciphertextlen, sizeof ciphertext,
		plaintext, plaintextlen);
	    if (rv != SECSuccess) {
		goto loser;
	    }

	    fputs(buf, aesresp);
	    fputs("CIPHERTEXT = ", aesresp);
	    to_hex_str(buf, ciphertext, ciphertextlen);
	    fputs(buf, aesresp);
	    fputc('\n', aesresp);
	    continue;
	}
	
	if (strncmp(buf, "CIPHERTEXT", 10) == 0) {
	    
	    if (encrypt) {
		goto loser;
	    }

	    i = 10;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    for (j=0; isxdigit(buf[i]); i+=2,j++) {
		hex_to_byteval(&buf[i], &ciphertext[j]);
	    }
	    ciphertextlen = j;

	    rv = aes_decrypt_buf(mode, key, keysize,
		(mode == NSS_AES) ? NULL : iv,
		plaintext, &plaintextlen, sizeof plaintext,
		ciphertext, ciphertextlen);
	    if (rv != SECSuccess) {
		goto loser;
	    }

	    fputs(buf, aesresp);
	    fputs("PLAINTEXT = ", aesresp);
	    to_hex_str(buf, plaintext, plaintextlen);
	    fputs(buf, aesresp);
	    fputc('\n', aesresp);
	    continue;
	}
    }
loser:
    fclose(aesreq);
}





void
aes_mct_next_key(unsigned char *key, unsigned int keysize,
    const unsigned char *ciphertext_1, const unsigned char *ciphertext)
{
    int k;

    switch (keysize) {
    case 16:  
	
	for (k=0; k<16; k++) {
	    key[k] ^= ciphertext[k];
	}
	break;
    case 24:  
	



	for (k=0; k<8; k++) {
	    key[k] ^= ciphertext_1[k+8];
	}
	for (k=8; k<24; k++) {
	    key[k] ^= ciphertext[k-8];
	}
	break;
    case 32:  
	
	for (k=0; k<16; k++) {
	    key[k] ^= ciphertext_1[k];
	}
	for (k=16; k<32; k++) {
	    key[k] ^= ciphertext[k-16];
	}
	break;
    }
}












void
aes_ecb_mct(char *reqfn)
{
    char buf[80];       



    FILE *aesreq;       
    FILE *aesresp;      
    int i, j;
    int encrypt = 0;    
    unsigned char key[32];              
    unsigned int keysize;
    unsigned char plaintext[16];        
    unsigned char plaintext_1[16];      
    unsigned char ciphertext[16];       
    unsigned char ciphertext_1[16];     
    unsigned char doublecheck[16];
    unsigned int outputlen;
    AESContext *cx = NULL;	
    AESContext *cx2 = NULL;     


    SECStatus rv;

    aesreq = fopen(reqfn, "r");
    aesresp = stdout;
    while (fgets(buf, sizeof buf, aesreq) != NULL) {
	
	if (buf[0] == '#' || buf[0] == '\n') {
	    fputs(buf, aesresp);
	    continue;
	}
	
	if (buf[0] == '[') {
	    if (strncmp(&buf[1], "ENCRYPT", 7) == 0) {
		encrypt = 1;
	    } else {
		encrypt = 0;
	    }
	    fputs(buf, aesresp);
	    continue;
	}
	
	if (strncmp(buf, "COUNT", 5) == 0) {
	    
	    memset(key, 0, sizeof key);
	    keysize = 0;
	    memset(plaintext, 0, sizeof plaintext);
	    memset(ciphertext, 0, sizeof ciphertext);
	    continue;
	}
	
	if (strncmp(buf, "KEY", 3) == 0) {
	    
	    i = 3;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    for (j=0; isxdigit(buf[i]); i+=2,j++) {
		hex_to_byteval(&buf[i], &key[j]);
	    }
	    keysize = j;
	    continue;
	}
	
	if (strncmp(buf, "PLAINTEXT", 9) == 0) {
	    
	    if (!encrypt) {
		goto loser;
	    }
	    
	    i = 9;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    for (j=0; j<sizeof plaintext; i+=2,j++) {
		hex_to_byteval(&buf[i], &plaintext[j]);
	    }

	    for (i=0; i<100; i++) {
		sprintf(buf, "COUNT = %d\n", i);
	        fputs(buf, aesresp);
		
		fputs("KEY = ", aesresp);
		to_hex_str(buf, key, keysize);
		fputs(buf, aesresp);
		fputc('\n', aesresp);
		
		fputs("PLAINTEXT = ", aesresp);
		to_hex_str(buf, plaintext, sizeof plaintext);
		fputs(buf, aesresp);
		fputc('\n', aesresp);

		cx = AES_CreateContext(key, NULL, NSS_AES,
		    PR_TRUE, keysize, 16);
		if (cx == NULL) {
		    goto loser;
		}
		



		cx2 = AES_CreateContext(key, NULL, NSS_AES,
		    PR_FALSE, keysize, 16);
		if (cx2 == NULL) {
		    goto loser;
		}
		for (j=0; j<1000; j++) {
		    
		    memcpy(ciphertext_1, ciphertext, sizeof ciphertext);

		    
		    outputlen = 0;
		    rv = AES_Encrypt(cx,
			ciphertext, &outputlen, sizeof ciphertext,
			plaintext, sizeof plaintext);
		    if (rv != SECSuccess) {
			goto loser;
		    }
		    if (outputlen != sizeof plaintext) {
			goto loser;
		    }

		    
		    outputlen = 0;
		    rv = AES_Decrypt(cx2,
			doublecheck, &outputlen, sizeof doublecheck,
			ciphertext, sizeof ciphertext);
		    if (rv != SECSuccess) {
			goto loser;
		    }
		    if (outputlen != sizeof ciphertext) {
			goto loser;
		    }
		    if (memcmp(doublecheck, plaintext, sizeof plaintext)) {
			goto loser;
		    }

		    
		    memcpy(plaintext, ciphertext, sizeof plaintext);
		}
		AES_DestroyContext(cx, PR_TRUE);
		cx = NULL;
		AES_DestroyContext(cx2, PR_TRUE);
		cx2 = NULL;

		
		fputs("CIPHERTEXT = ", aesresp);
		to_hex_str(buf, ciphertext, sizeof ciphertext);
		fputs(buf, aesresp);
		fputc('\n', aesresp);

		
		aes_mct_next_key(key, keysize, ciphertext_1, ciphertext);
		
		

		fputc('\n', aesresp);
	    }

	    continue;
	}
	
	if (strncmp(buf, "CIPHERTEXT", 10) == 0) {
	    
	    if (encrypt) {
		goto loser;
	    }
	    
	    i = 10;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    for (j=0; isxdigit(buf[i]); i+=2,j++) {
		hex_to_byteval(&buf[i], &ciphertext[j]);
	    }

	    for (i=0; i<100; i++) {
		sprintf(buf, "COUNT = %d\n", i);
	        fputs(buf, aesresp);
		
		fputs("KEY = ", aesresp);
		to_hex_str(buf, key, keysize);
		fputs(buf, aesresp);
		fputc('\n', aesresp);
		
		fputs("CIPHERTEXT = ", aesresp);
		to_hex_str(buf, ciphertext, sizeof ciphertext);
		fputs(buf, aesresp);
		fputc('\n', aesresp);

		cx = AES_CreateContext(key, NULL, NSS_AES,
		    PR_FALSE, keysize, 16);
		if (cx == NULL) {
		    goto loser;
		}
		



		cx2 = AES_CreateContext(key, NULL, NSS_AES,
		    PR_TRUE, keysize, 16);
		if (cx2 == NULL) {
		    goto loser;
		}
		for (j=0; j<1000; j++) {
		    
		    memcpy(plaintext_1, plaintext, sizeof plaintext);

		    
		    outputlen = 0;
		    rv = AES_Decrypt(cx,
			plaintext, &outputlen, sizeof plaintext,
			ciphertext, sizeof ciphertext);
		    if (rv != SECSuccess) {
			goto loser;
		    }
		    if (outputlen != sizeof ciphertext) {
			goto loser;
		    }

		    
		    outputlen = 0;
		    rv = AES_Encrypt(cx2,
			doublecheck, &outputlen, sizeof doublecheck,
			plaintext, sizeof plaintext);
		    if (rv != SECSuccess) {
			goto loser;
		    }
		    if (outputlen != sizeof plaintext) {
			goto loser;
		    }
		    if (memcmp(doublecheck, ciphertext, sizeof ciphertext)) {
			goto loser;
		    }

		    
		    memcpy(ciphertext, plaintext, sizeof ciphertext);
		}
		AES_DestroyContext(cx, PR_TRUE);
		cx = NULL;
		AES_DestroyContext(cx2, PR_TRUE);
		cx2 = NULL;

		
		fputs("PLAINTEXT = ", aesresp);
		to_hex_str(buf, plaintext, sizeof plaintext);
		fputs(buf, aesresp);
		fputc('\n', aesresp);

		
		aes_mct_next_key(key, keysize, plaintext_1, plaintext);
		
		

		fputc('\n', aesresp);
	    }

	    continue;
	}
    }
loser:
    if (cx != NULL) {
	AES_DestroyContext(cx, PR_TRUE);
    }
    if (cx2 != NULL) {
	AES_DestroyContext(cx2, PR_TRUE);
    }
    fclose(aesreq);
}












void
aes_cbc_mct(char *reqfn)
{
    char buf[80];       



    FILE *aesreq;       
    FILE *aesresp;      
    int i, j;
    int encrypt = 0;    
    unsigned char key[32];              
    unsigned int keysize;
    unsigned char iv[16];
    unsigned char plaintext[16];        
    unsigned char plaintext_1[16];      
    unsigned char ciphertext[16];       
    unsigned char ciphertext_1[16];     
    unsigned char doublecheck[16];
    unsigned int outputlen;
    AESContext *cx = NULL;	
    AESContext *cx2 = NULL;     


    SECStatus rv;

    aesreq = fopen(reqfn, "r");
    aesresp = stdout;
    while (fgets(buf, sizeof buf, aesreq) != NULL) {
	
	if (buf[0] == '#' || buf[0] == '\n') {
	    fputs(buf, aesresp);
	    continue;
	}
	
	if (buf[0] == '[') {
	    if (strncmp(&buf[1], "ENCRYPT", 7) == 0) {
		encrypt = 1;
	    } else {
		encrypt = 0;
	    }
	    fputs(buf, aesresp);
	    continue;
	}
	
	if (strncmp(buf, "COUNT", 5) == 0) {
	    
	    memset(key, 0, sizeof key);
	    keysize = 0;
	    memset(iv, 0, sizeof iv);
	    memset(plaintext, 0, sizeof plaintext);
	    memset(ciphertext, 0, sizeof ciphertext);
	    continue;
	}
	
	if (strncmp(buf, "KEY", 3) == 0) {
	    
	    i = 3;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    for (j=0; isxdigit(buf[i]); i+=2,j++) {
		hex_to_byteval(&buf[i], &key[j]);
	    }
	    keysize = j;
	    continue;
	}
	
	if (strncmp(buf, "IV", 2) == 0) {
	    
	    i = 2;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    for (j=0; j<sizeof iv; i+=2,j++) {
		hex_to_byteval(&buf[i], &iv[j]);
	    }
	    continue;
	}
	
	if (strncmp(buf, "PLAINTEXT", 9) == 0) {
	    
	    if (!encrypt) {
		goto loser;
	    }
	    
	    i = 9;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    for (j=0; j<sizeof plaintext; i+=2,j++) {
		hex_to_byteval(&buf[i], &plaintext[j]);
	    }

	    for (i=0; i<100; i++) {
		sprintf(buf, "COUNT = %d\n", i);
	        fputs(buf, aesresp);
		
		fputs("KEY = ", aesresp);
		to_hex_str(buf, key, keysize);
		fputs(buf, aesresp);
		fputc('\n', aesresp);
		
		fputs("IV = ", aesresp);
		to_hex_str(buf, iv, sizeof iv);
		fputs(buf, aesresp);
		fputc('\n', aesresp);
		
		fputs("PLAINTEXT = ", aesresp);
		to_hex_str(buf, plaintext, sizeof plaintext);
		fputs(buf, aesresp);
		fputc('\n', aesresp);

		cx = AES_CreateContext(key, iv, NSS_AES_CBC,
		    PR_TRUE, keysize, 16);
		if (cx == NULL) {
		    goto loser;
		}
		



		cx2 = AES_CreateContext(key, iv, NSS_AES_CBC,
		    PR_FALSE, keysize, 16);
		if (cx2 == NULL) {
		    goto loser;
		}
		
		memcpy(ciphertext, iv, sizeof ciphertext);
		for (j=0; j<1000; j++) {
		    
		    memcpy(ciphertext_1, ciphertext, sizeof ciphertext);
		    







		    outputlen = 0;
		    rv = AES_Encrypt(cx,
			ciphertext, &outputlen, sizeof ciphertext,
			plaintext, sizeof plaintext);
		    if (rv != SECSuccess) {
			goto loser;
		    }
		    if (outputlen != sizeof plaintext) {
			goto loser;
		    }

		    
		    outputlen = 0;
		    rv = AES_Decrypt(cx2,
			doublecheck, &outputlen, sizeof doublecheck,
			ciphertext, sizeof ciphertext);
		    if (rv != SECSuccess) {
			goto loser;
		    }
		    if (outputlen != sizeof ciphertext) {
			goto loser;
		    }
		    if (memcmp(doublecheck, plaintext, sizeof plaintext)) {
			goto loser;
		    }

		    memcpy(plaintext, ciphertext_1, sizeof plaintext);
		}
		AES_DestroyContext(cx, PR_TRUE);
		cx = NULL;
		AES_DestroyContext(cx2, PR_TRUE);
		cx2 = NULL;

		
		fputs("CIPHERTEXT = ", aesresp);
		to_hex_str(buf, ciphertext, sizeof ciphertext);
		fputs(buf, aesresp);
		fputc('\n', aesresp);

		
		aes_mct_next_key(key, keysize, ciphertext_1, ciphertext);
		
		memcpy(iv, ciphertext, sizeof iv);
		
		

		fputc('\n', aesresp);
	    }

	    continue;
	}
	
	if (strncmp(buf, "CIPHERTEXT", 10) == 0) {
	    
	    if (encrypt) {
		goto loser;
	    }
	    
	    i = 10;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    for (j=0; isxdigit(buf[i]); i+=2,j++) {
		hex_to_byteval(&buf[i], &ciphertext[j]);
	    }

	    for (i=0; i<100; i++) {
		sprintf(buf, "COUNT = %d\n", i);
	        fputs(buf, aesresp);
		
		fputs("KEY = ", aesresp);
		to_hex_str(buf, key, keysize);
		fputs(buf, aesresp);
		fputc('\n', aesresp);
		
		fputs("IV = ", aesresp);
		to_hex_str(buf, iv, sizeof iv);
		fputs(buf, aesresp);
		fputc('\n', aesresp);
		
		fputs("CIPHERTEXT = ", aesresp);
		to_hex_str(buf, ciphertext, sizeof ciphertext);
		fputs(buf, aesresp);
		fputc('\n', aesresp);

		cx = AES_CreateContext(key, iv, NSS_AES_CBC,
		    PR_FALSE, keysize, 16);
		if (cx == NULL) {
		    goto loser;
		}
		



		cx2 = AES_CreateContext(key, iv, NSS_AES_CBC,
		    PR_TRUE, keysize, 16);
		if (cx2 == NULL) {
		    goto loser;
		}
		
		memcpy(plaintext, iv, sizeof plaintext);
		for (j=0; j<1000; j++) {
		    
		    memcpy(plaintext_1, plaintext, sizeof plaintext);
		    







		    outputlen = 0;
		    rv = AES_Decrypt(cx,
			plaintext, &outputlen, sizeof plaintext,
			ciphertext, sizeof ciphertext);
		    if (rv != SECSuccess) {
			goto loser;
		    }
		    if (outputlen != sizeof ciphertext) {
			goto loser;
		    }

		    
		    outputlen = 0;
		    rv = AES_Encrypt(cx2,
			doublecheck, &outputlen, sizeof doublecheck,
			plaintext, sizeof plaintext);
		    if (rv != SECSuccess) {
			goto loser;
		    }
		    if (outputlen != sizeof plaintext) {
			goto loser;
		    }
		    if (memcmp(doublecheck, ciphertext, sizeof ciphertext)) {
			goto loser;
		    }

		    memcpy(ciphertext, plaintext_1, sizeof ciphertext);
		}
		AES_DestroyContext(cx, PR_TRUE);
		cx = NULL;
		AES_DestroyContext(cx2, PR_TRUE);
		cx2 = NULL;

		
		fputs("PLAINTEXT = ", aesresp);
		to_hex_str(buf, plaintext, sizeof plaintext);
		fputs(buf, aesresp);
		fputc('\n', aesresp);

		
		aes_mct_next_key(key, keysize, plaintext_1, plaintext);
		
		memcpy(iv, plaintext, sizeof iv);
		
		

		fputc('\n', aesresp);
	    }

	    continue;
	}
    }
loser:
    if (cx != NULL) {
	AES_DestroyContext(cx, PR_TRUE);
    }
    if (cx2 != NULL) {
	AES_DestroyContext(cx2, PR_TRUE);
    }
    fclose(aesreq);
}

void write_compact_string(FILE *out, unsigned char *hash, unsigned int len)
{
    unsigned int i;
    int j, count = 0, last = -1, z = 0;
    long start = ftell(out);
    for (i=0; i<len; i++) {
	for (j=7; j>=0; j--) {
	    if (last < 0) {
		last = (hash[i] & (1 << j)) ? 1 : 0;
		fprintf(out, "%d ", last);
		count = 1;
	    } else if (hash[i] & (1 << j)) {
		if (last) {
		    count++; 
		} else { 
		    last = 0;
		    fprintf(out, "%d ", count);
		    count = 1;
		    z++;
		}
	    } else {
		if (!last) {
		    count++; 
		} else { 
		    last = 1;
		    fprintf(out, "%d ", count);
		    count = 1;
		    z++;
		}
	    }
	}
    }
    fprintf(out, "^\n");
    fseek(out, start, SEEK_SET);
    fprintf(out, "%d ", z);
    fseek(out, 0, SEEK_END);
}

int get_next_line(FILE *req, char *key, char *val, FILE *rsp)
{
    int ignore = 0;
    char *writeto = key;
    int w = 0;
    int c;
    while ((c = fgetc(req)) != EOF) {
	if (ignore) {
	    fprintf(rsp, "%c", c);
	    if (c == '\n') return ignore;
	} else if (c == '\n') {
	    break;
	} else if (c == '#') {
	    ignore = 1;
	    fprintf(rsp, "%c", c);
	} else if (c == '=') {
	    writeto[w] = '\0';
	    w = 0;
	    writeto = val;
	} else if (c == ' ' || c == '[' || c == ']') {
	    continue;
	} else {
	    writeto[w++] = c;
	}
    }
    writeto[w] = '\0';
    return (c == EOF) ? -1 : ignore;
}

#ifdef NSS_ENABLE_ECC
typedef struct curveNameTagPairStr {
    char *curveName;
    SECOidTag curveOidTag;
} CurveNameTagPair;

#define DEFAULT_CURVE_OID_TAG  SEC_OID_SECG_EC_SECP192R1


static CurveNameTagPair nameTagPair[] =
{ 
  { "sect163k1", SEC_OID_SECG_EC_SECT163K1},
  { "nistk163", SEC_OID_SECG_EC_SECT163K1},
  { "sect163r1", SEC_OID_SECG_EC_SECT163R1},
  { "sect163r2", SEC_OID_SECG_EC_SECT163R2},
  { "nistb163", SEC_OID_SECG_EC_SECT163R2},
  { "sect193r1", SEC_OID_SECG_EC_SECT193R1},
  { "sect193r2", SEC_OID_SECG_EC_SECT193R2},
  { "sect233k1", SEC_OID_SECG_EC_SECT233K1},
  { "nistk233", SEC_OID_SECG_EC_SECT233K1},
  { "sect233r1", SEC_OID_SECG_EC_SECT233R1},
  { "nistb233", SEC_OID_SECG_EC_SECT233R1},
  { "sect239k1", SEC_OID_SECG_EC_SECT239K1},
  { "sect283k1", SEC_OID_SECG_EC_SECT283K1},
  { "nistk283", SEC_OID_SECG_EC_SECT283K1},
  { "sect283r1", SEC_OID_SECG_EC_SECT283R1},
  { "nistb283", SEC_OID_SECG_EC_SECT283R1},
  { "sect409k1", SEC_OID_SECG_EC_SECT409K1},
  { "nistk409", SEC_OID_SECG_EC_SECT409K1},
  { "sect409r1", SEC_OID_SECG_EC_SECT409R1},
  { "nistb409", SEC_OID_SECG_EC_SECT409R1},
  { "sect571k1", SEC_OID_SECG_EC_SECT571K1},
  { "nistk571", SEC_OID_SECG_EC_SECT571K1},
  { "sect571r1", SEC_OID_SECG_EC_SECT571R1},
  { "nistb571", SEC_OID_SECG_EC_SECT571R1},
  { "secp160k1", SEC_OID_SECG_EC_SECP160K1},
  { "secp160r1", SEC_OID_SECG_EC_SECP160R1},
  { "secp160r2", SEC_OID_SECG_EC_SECP160R2},
  { "secp192k1", SEC_OID_SECG_EC_SECP192K1},
  { "secp192r1", SEC_OID_SECG_EC_SECP192R1},
  { "nistp192", SEC_OID_SECG_EC_SECP192R1},
  { "secp224k1", SEC_OID_SECG_EC_SECP224K1},
  { "secp224r1", SEC_OID_SECG_EC_SECP224R1},
  { "nistp224", SEC_OID_SECG_EC_SECP224R1},
  { "secp256k1", SEC_OID_SECG_EC_SECP256K1},
  { "secp256r1", SEC_OID_SECG_EC_SECP256R1},
  { "nistp256", SEC_OID_SECG_EC_SECP256R1},
  { "secp384r1", SEC_OID_SECG_EC_SECP384R1},
  { "nistp384", SEC_OID_SECG_EC_SECP384R1},
  { "secp521r1", SEC_OID_SECG_EC_SECP521R1},
  { "nistp521", SEC_OID_SECG_EC_SECP521R1},

  { "prime192v1", SEC_OID_ANSIX962_EC_PRIME192V1 },
  { "prime192v2", SEC_OID_ANSIX962_EC_PRIME192V2 },
  { "prime192v3", SEC_OID_ANSIX962_EC_PRIME192V3 },
  { "prime239v1", SEC_OID_ANSIX962_EC_PRIME239V1 },
  { "prime239v2", SEC_OID_ANSIX962_EC_PRIME239V2 },
  { "prime239v3", SEC_OID_ANSIX962_EC_PRIME239V3 },

  { "c2pnb163v1", SEC_OID_ANSIX962_EC_C2PNB163V1 },
  { "c2pnb163v2", SEC_OID_ANSIX962_EC_C2PNB163V2 },
  { "c2pnb163v3", SEC_OID_ANSIX962_EC_C2PNB163V3 },
  { "c2pnb176v1", SEC_OID_ANSIX962_EC_C2PNB176V1 },
  { "c2tnb191v1", SEC_OID_ANSIX962_EC_C2TNB191V1 },
  { "c2tnb191v2", SEC_OID_ANSIX962_EC_C2TNB191V2 },
  { "c2tnb191v3", SEC_OID_ANSIX962_EC_C2TNB191V3 },
  { "c2onb191v4", SEC_OID_ANSIX962_EC_C2ONB191V4 },
  { "c2onb191v5", SEC_OID_ANSIX962_EC_C2ONB191V5 },
  { "c2pnb208w1", SEC_OID_ANSIX962_EC_C2PNB208W1 },
  { "c2tnb239v1", SEC_OID_ANSIX962_EC_C2TNB239V1 },
  { "c2tnb239v2", SEC_OID_ANSIX962_EC_C2TNB239V2 },
  { "c2tnb239v3", SEC_OID_ANSIX962_EC_C2TNB239V3 },
  { "c2onb239v4", SEC_OID_ANSIX962_EC_C2ONB239V4 },
  { "c2onb239v5", SEC_OID_ANSIX962_EC_C2ONB239V5 },
  { "c2pnb272w1", SEC_OID_ANSIX962_EC_C2PNB272W1 },
  { "c2pnb304w1", SEC_OID_ANSIX962_EC_C2PNB304W1 },
  { "c2tnb359v1", SEC_OID_ANSIX962_EC_C2TNB359V1 },
  { "c2pnb368w1", SEC_OID_ANSIX962_EC_C2PNB368W1 },
  { "c2tnb431r1", SEC_OID_ANSIX962_EC_C2TNB431R1 },

  { "secp112r1", SEC_OID_SECG_EC_SECP112R1},
  { "secp112r2", SEC_OID_SECG_EC_SECP112R2},
  { "secp128r1", SEC_OID_SECG_EC_SECP128R1},
  { "secp128r2", SEC_OID_SECG_EC_SECP128R2},

  { "sect113r1", SEC_OID_SECG_EC_SECT113R1},
  { "sect113r2", SEC_OID_SECG_EC_SECT113R2},
  { "sect131r1", SEC_OID_SECG_EC_SECT131R1},
  { "sect131r2", SEC_OID_SECG_EC_SECT131R2},
};

static SECKEYECParams * 
getECParams(const char *curve)
{
    SECKEYECParams *ecparams;
    SECOidData *oidData = NULL;
    SECOidTag curveOidTag = SEC_OID_UNKNOWN; 
    int i, numCurves;

    if (curve != NULL) {
        numCurves = sizeof(nameTagPair)/sizeof(CurveNameTagPair);
	for (i = 0; ((i < numCurves) && (curveOidTag == SEC_OID_UNKNOWN)); 
	     i++) {
	    if (PL_strcmp(curve, nameTagPair[i].curveName) == 0)
	        curveOidTag = nameTagPair[i].curveOidTag;
	}
    }

    
    if ((curveOidTag == SEC_OID_UNKNOWN) || 
	(oidData = SECOID_FindOIDByTag(curveOidTag)) == NULL) {
        fprintf(stderr, "Unrecognized elliptic curve %s\n", curve);
	return NULL;
    }

    ecparams = SECITEM_AllocItem(NULL, NULL, (2 + oidData->oid.len));

    




    ecparams->data[0] = SEC_ASN1_OBJECT_ID;
    ecparams->data[1] = oidData->oid.len;
    memcpy(ecparams->data + 2, oidData->oid.data, oidData->oid.len);

    return ecparams;
}








void
ecdsa_keypair_test(char *reqfn)
{
    char buf[256];      




    FILE *ecdsareq;     
    FILE *ecdsaresp;    
    char curve[16];     
    ECParams *ecparams;
    int N;
    int i;
    unsigned int len;

    ecdsareq = fopen(reqfn, "r");
    ecdsaresp = stdout;
    strcpy(curve, "nist");
    while (fgets(buf, sizeof buf, ecdsareq) != NULL) {
	
	if (buf[0] == '#' || buf[0] == '\n') {
	    fputs(buf, ecdsaresp);
	    continue;
	}
	
	if (buf[0] == '[') {
	    const char *src;
	    char *dst;
	    SECKEYECParams *encodedparams;

	    src = &buf[1];
	    dst = &curve[4];
	    *dst++ = tolower(*src);
	    src += 2;  
	    *dst++ = *src++;
	    *dst++ = *src++;
	    *dst++ = *src++;
	    *dst = '\0';
	    encodedparams = getECParams(curve);
	    if (encodedparams == NULL) {
		goto loser;
	    }
	    if (EC_DecodeParams(encodedparams, &ecparams) != SECSuccess) {
		goto loser;
	    }
	    SECITEM_FreeItem(encodedparams, PR_TRUE);
	    fputs(buf, ecdsaresp);
	    continue;
	}
	
	if (buf[0] == 'N') {
	    if (sscanf(buf, "N = %d", &N) != 1) {
		goto loser;
	    }
	    for (i = 0; i < N; i++) {
		ECPrivateKey *ecpriv;

		if (EC_NewKey(ecparams, &ecpriv) != SECSuccess) {
		    goto loser;
		}
		fputs("d = ", ecdsaresp);
		to_hex_str(buf, ecpriv->privateValue.data,
			   ecpriv->privateValue.len);
		fputs(buf, ecdsaresp);
		fputc('\n', ecdsaresp);
		if (EC_ValidatePublicKey(ecparams, &ecpriv->publicValue)
		    != SECSuccess) {
		    goto loser;
		}
		len = ecpriv->publicValue.len;
		if (len%2 == 0) {
		    goto loser;
		}
		len = (len-1)/2;
		if (ecpriv->publicValue.data[0]
		    != EC_POINT_FORM_UNCOMPRESSED) {
		    goto loser;
		}
		fputs("Qx = ", ecdsaresp);
		to_hex_str(buf, &ecpriv->publicValue.data[1], len);
		fputs(buf, ecdsaresp);
		fputc('\n', ecdsaresp);
		fputs("Qy = ", ecdsaresp);
		to_hex_str(buf, &ecpriv->publicValue.data[1+len], len);
		fputs(buf, ecdsaresp);
		fputc('\n', ecdsaresp);
		fputc('\n', ecdsaresp);
		PORT_FreeArena(ecpriv->ecParams.arena, PR_TRUE);
	    }
	    PORT_FreeArena(ecparams->arena, PR_FALSE);
	    continue;
	}
    }
loser:
    fclose(ecdsareq);
}








void
ecdsa_pkv_test(char *reqfn)
{
    char buf[256];      



    FILE *ecdsareq;     
    FILE *ecdsaresp;    
    char curve[16];     
    ECParams *ecparams = NULL;
    SECItem pubkey;
    unsigned int i;
    unsigned int len;
    PRBool keyvalid = PR_TRUE;

    ecdsareq = fopen(reqfn, "r");
    ecdsaresp = stdout;
    strcpy(curve, "nist");
    pubkey.data = NULL;
    while (fgets(buf, sizeof buf, ecdsareq) != NULL) {
	
	if (buf[0] == '#' || buf[0] == '\n') {
	    fputs(buf, ecdsaresp);
	    continue;
	}
	
	if (buf[0] == '[') {
	    const char *src;
	    char *dst;
	    SECKEYECParams *encodedparams;

	    src = &buf[1];
	    dst = &curve[4];
	    *dst++ = tolower(*src);
	    src += 2;  
	    *dst++ = *src++;
	    *dst++ = *src++;
	    *dst++ = *src++;
	    *dst = '\0';
	    if (ecparams != NULL) {
		PORT_FreeArena(ecparams->arena, PR_FALSE);
		ecparams = NULL;
	    }
	    encodedparams = getECParams(curve);
	    if (encodedparams == NULL) {
		goto loser;
	    }
	    if (EC_DecodeParams(encodedparams, &ecparams) != SECSuccess) {
		goto loser;
	    }
	    SECITEM_FreeItem(encodedparams, PR_TRUE);
	    len = (ecparams->fieldID.size + 7) >> 3;
	    if (pubkey.data != NULL) {
		PORT_Free(pubkey.data);
		pubkey.data = NULL;
	    }
	    SECITEM_AllocItem(NULL, &pubkey, 2*len+1);
	    if (pubkey.data == NULL) {
		goto loser;
	    }
	    pubkey.data[0] = EC_POINT_FORM_UNCOMPRESSED;
	    fputs(buf, ecdsaresp);
	    continue;
	}
	
	if (strncmp(buf, "Qx", 2) == 0) {
	    fputs(buf, ecdsaresp);
	    i = 2;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    keyvalid = from_hex_str(&pubkey.data[1], len, &buf[i]);
	    continue;
	}
	
	if (strncmp(buf, "Qy", 2) == 0) {
	    fputs(buf, ecdsaresp);
	    if (!keyvalid) {
		fputs("Result = F\n", ecdsaresp);
		continue;
	    }
	    i = 2;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    keyvalid = from_hex_str(&pubkey.data[1+len], len, &buf[i]);
	    if (!keyvalid) {
		fputs("Result = F\n", ecdsaresp);
		continue;
	    }
	    if (EC_ValidatePublicKey(ecparams, &pubkey) == SECSuccess) {
		fputs("Result = P\n", ecdsaresp);
	    } else if (PORT_GetError() == SEC_ERROR_BAD_KEY) {
		fputs("Result = F\n", ecdsaresp);
	    } else {
		goto loser;
	    }
	    continue;
	}
    }
loser:
    if (ecparams != NULL) {
	PORT_FreeArena(ecparams->arena, PR_FALSE);
    }
    if (pubkey.data != NULL) {
	PORT_Free(pubkey.data);
    }
    fclose(ecdsareq);
}








void
ecdsa_siggen_test(char *reqfn)
{
    char buf[1024];     




    FILE *ecdsareq;     
    FILE *ecdsaresp;    
    char curve[16];     
    ECParams *ecparams = NULL;
    int i, j;
    unsigned int len;
    unsigned char msg[512];  
    unsigned int msglen;
    unsigned char sha1[20];  
    unsigned char sig[2*MAX_ECKEY_LEN];
    SECItem signature, digest;

    ecdsareq = fopen(reqfn, "r");
    ecdsaresp = stdout;
    strcpy(curve, "nist");
    while (fgets(buf, sizeof buf, ecdsareq) != NULL) {
	
	if (buf[0] == '#' || buf[0] == '\n') {
	    fputs(buf, ecdsaresp);
	    continue;
	}
	
	if (buf[0] == '[') {
	    const char *src;
	    char *dst;
	    SECKEYECParams *encodedparams;

	    src = &buf[1];
	    dst = &curve[4];
	    *dst++ = tolower(*src);
	    src += 2;  
	    *dst++ = *src++;
	    *dst++ = *src++;
	    *dst++ = *src++;
	    *dst = '\0';
	    if (ecparams != NULL) {
		PORT_FreeArena(ecparams->arena, PR_FALSE);
		ecparams = NULL;
	    }
	    encodedparams = getECParams(curve);
	    if (encodedparams == NULL) {
		goto loser;
	    }
	    if (EC_DecodeParams(encodedparams, &ecparams) != SECSuccess) {
		goto loser;
	    }
	    SECITEM_FreeItem(encodedparams, PR_TRUE);
	    fputs(buf, ecdsaresp);
	    continue;
	}
	
	if (strncmp(buf, "Msg", 3) == 0) {
	    ECPrivateKey *ecpriv;

	    i = 3;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    for (j=0; isxdigit(buf[i]); i+=2,j++) {
		hex_to_byteval(&buf[i], &msg[j]);
	    }
	    msglen = j;
	    if (SHA1_HashBuf(sha1, msg, msglen) != SECSuccess) {
		goto loser;
	    }
	    fputs(buf, ecdsaresp);

	    if (EC_NewKey(ecparams, &ecpriv) != SECSuccess) {
		goto loser;
	    }
	    if (EC_ValidatePublicKey(ecparams, &ecpriv->publicValue)
		!= SECSuccess) {
		goto loser;
	    }
	    len = ecpriv->publicValue.len;
	    if (len%2 == 0) {
		goto loser;
	    }
	    len = (len-1)/2;
	    if (ecpriv->publicValue.data[0] != EC_POINT_FORM_UNCOMPRESSED) {
		goto loser;
	    }
	    fputs("Qx = ", ecdsaresp);
	    to_hex_str(buf, &ecpriv->publicValue.data[1], len);
	    fputs(buf, ecdsaresp);
	    fputc('\n', ecdsaresp);
	    fputs("Qy = ", ecdsaresp);
	    to_hex_str(buf, &ecpriv->publicValue.data[1+len], len);
	    fputs(buf, ecdsaresp);
	    fputc('\n', ecdsaresp);

	    digest.type = siBuffer;
	    digest.data = sha1;
	    digest.len = sizeof sha1;
	    signature.type = siBuffer;
	    signature.data = sig;
	    signature.len = sizeof sig;
	    if (ECDSA_SignDigest(ecpriv, &signature, &digest) != SECSuccess) {
		goto loser;
	    }
	    len = signature.len;
	    if (len%2 != 0) {
		goto loser;
	    }
	    len = len/2;
	    fputs("R = ", ecdsaresp);
	    to_hex_str(buf, &signature.data[0], len);
	    fputs(buf, ecdsaresp);
	    fputc('\n', ecdsaresp);
	    fputs("S = ", ecdsaresp);
	    to_hex_str(buf, &signature.data[len], len);
	    fputs(buf, ecdsaresp);
	    fputc('\n', ecdsaresp);

	    PORT_FreeArena(ecpriv->ecParams.arena, PR_TRUE);
	    continue;
	}
    }
loser:
    if (ecparams != NULL) {
	PORT_FreeArena(ecparams->arena, PR_FALSE);
    }
    fclose(ecdsareq);
}








void
ecdsa_sigver_test(char *reqfn)
{
    char buf[1024];     



    FILE *ecdsareq;     
    FILE *ecdsaresp;    
    char curve[16];     
    ECPublicKey ecpub;
    unsigned int i, j;
    unsigned int flen;  
    unsigned int olen;  
    unsigned char msg[512];  
    unsigned int msglen;
    unsigned char sha1[20];  
    unsigned char sig[2*MAX_ECKEY_LEN];
    SECItem signature, digest;
    PRBool keyvalid = PR_TRUE;
    PRBool sigvalid = PR_TRUE;

    ecdsareq = fopen(reqfn, "r");
    ecdsaresp = stdout;
    ecpub.ecParams.arena = NULL;
    strcpy(curve, "nist");
    while (fgets(buf, sizeof buf, ecdsareq) != NULL) {
	
	if (buf[0] == '#' || buf[0] == '\n') {
	    fputs(buf, ecdsaresp);
	    continue;
	}
	
	if (buf[0] == '[') {
	    const char *src;
	    char *dst;
	    SECKEYECParams *encodedparams;
	    ECParams *ecparams;

	    src = &buf[1];
	    dst = &curve[4];
	    *dst++ = tolower(*src);
	    src += 2;  
	    *dst++ = *src++;
	    *dst++ = *src++;
	    *dst++ = *src++;
	    *dst = '\0';
	    encodedparams = getECParams(curve);
	    if (encodedparams == NULL) {
		goto loser;
	    }
	    if (EC_DecodeParams(encodedparams, &ecparams) != SECSuccess) {
		goto loser;
	    }
	    SECITEM_FreeItem(encodedparams, PR_TRUE);
	    if (ecpub.ecParams.arena != NULL) {
		PORT_FreeArena(ecpub.ecParams.arena, PR_FALSE);
	    }
	    ecpub.ecParams.arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
	    if (ecpub.ecParams.arena == NULL) {
		goto loser;
	    }
	    if (EC_CopyParams(ecpub.ecParams.arena, &ecpub.ecParams, ecparams)
		!= SECSuccess) {
		goto loser;
	    }
	    PORT_FreeArena(ecparams->arena, PR_FALSE);
	    flen = (ecpub.ecParams.fieldID.size + 7) >> 3;
	    olen = ecpub.ecParams.order.len;
	    if (2*olen > sizeof sig) {
		goto loser;
	    }
	    ecpub.publicValue.type = siBuffer;
	    ecpub.publicValue.data = NULL;
	    ecpub.publicValue.len = 0;
	    SECITEM_AllocItem(ecpub.ecParams.arena,
			      &ecpub.publicValue, 2*flen+1);
	    if (ecpub.publicValue.data == NULL) {
		goto loser;
	    }
	    ecpub.publicValue.data[0] = EC_POINT_FORM_UNCOMPRESSED;
	    fputs(buf, ecdsaresp);
	    continue;
	}
	
	if (strncmp(buf, "Msg", 3) == 0) {
	    i = 3;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    for (j=0; isxdigit(buf[i]); i+=2,j++) {
		hex_to_byteval(&buf[i], &msg[j]);
	    }
	    msglen = j;
	    if (SHA1_HashBuf(sha1, msg, msglen) != SECSuccess) {
		goto loser;
	    }
	    fputs(buf, ecdsaresp);

	    digest.type = siBuffer;
	    digest.data = sha1;
	    digest.len = sizeof sha1;

	    continue;
	}
	
	if (strncmp(buf, "Qx", 2) == 0) {
	    fputs(buf, ecdsaresp);
	    i = 2;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    keyvalid = from_hex_str(&ecpub.publicValue.data[1], flen,
				    &buf[i]);
	    continue;
	}
	
	if (strncmp(buf, "Qy", 2) == 0) {
	    fputs(buf, ecdsaresp);
	    if (!keyvalid) {
		continue;
	    }
	    i = 2;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    keyvalid = from_hex_str(&ecpub.publicValue.data[1+flen], flen,
				    &buf[i]);
	    if (!keyvalid) {
		continue;
	    }
	    if (EC_ValidatePublicKey(&ecpub.ecParams, &ecpub.publicValue)
		!= SECSuccess) {
		if (PORT_GetError() == SEC_ERROR_BAD_KEY) {
		    keyvalid = PR_FALSE;
		} else {
		    goto loser;
		}
	    }
	    continue;
	}
	
	if (buf[0] == 'R') {
	    fputs(buf, ecdsaresp);
	    i = 1;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    sigvalid = from_hex_str(sig, olen, &buf[i]);
	    continue;
	}
	
	if (buf[0] == 'S') {
	    fputs(buf, ecdsaresp);
	    i = 1;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    if (sigvalid) {
		sigvalid = from_hex_str(&sig[olen], olen, &buf[i]);
	    }
	    signature.type = siBuffer;
	    signature.data = sig;
	    signature.len = 2*olen;

	    if (!keyvalid || !sigvalid) {
		fputs("Result = F\n", ecdsaresp);
	    } else if (ECDSA_VerifyDigest(&ecpub, &signature, &digest)
		== SECSuccess) {
		fputs("Result = P\n", ecdsaresp);
	    } else {
		fputs("Result = F\n", ecdsaresp);
	    }
	    continue;
	}
    }
loser:
    if (ecpub.ecParams.arena != NULL) {
	PORT_FreeArena(ecpub.ecParams.arena, PR_FALSE);
    }
    fclose(ecdsareq);
}
#endif 





static unsigned char *
alloc_value(char *buf, int *len)
{
    unsigned char * value;
    int i, count;

    if (strncmp(buf, "<None>", 6) == 0) {
	*len = 0;
	return NULL;
    }

    
    for (count = 0; isxdigit(buf[count]); count++);
    *len = count/2;

    if (*len == 0) {
	return NULL;
    }

    value = PORT_Alloc(*len);
    if (!value) {
	*len = 0;
	return NULL;
    }
	
    for (i=0; i<*len; buf+=2 , i++) {
	hex_to_byteval(buf, &value[i]);
    }
    return value;
}

PRBool
isblankline(char *b)
{
   while (isspace(*b)) b++;
   if ((*b == '\n') || (*b == 0)) {
	return PR_TRUE;
   }
   return PR_FALSE;
}








void
drbg(char *reqfn)
{
    char buf[2000];   


    char buf2[2000]; 
    FILE *rngreq;       
    FILE *rngresp;      
    unsigned int i;
    unsigned char *entropy = NULL;
    int entropy_len = 0;
    unsigned char *nonce =  NULL;
    int nonce_len = 0;
    unsigned char *personalization_string =  NULL;
    int ps_len = 0;
    unsigned char *return_bytes =  NULL;
    unsigned char *predicted_return_bytes = NULL;
    int return_bytes_len = 0;
    unsigned char *additional_input =  NULL;
    int additional_len = 0;
    enum { NONE, INSTANTIATE, GENERATE, RESEED, UNINSTANTIATE } command =
		NONE;
    SECStatus rv;

    rngreq = fopen(reqfn, "r");
    rngresp = stdout;
    while (fgets(buf, sizeof buf, rngreq) != NULL) {
	
	if (buf[0] == '#') { 
	    fputs(buf, rngresp);
	    continue;
	}

        if (isblankline(buf)) {
	    switch (command) {
	    case INSTANTIATE:
		rv = PRNGTEST_Instantiate(entropy, entropy_len,
				      nonce, nonce_len,
				      personalization_string, ps_len);
		if (rv != SECSuccess) {
		    goto loser;
		}
		
		if (entropy) {
		    PORT_ZFree(entropy, entropy_len);
		    entropy = NULL;
		    entropy_len = 0;
		}
		if (nonce) {
		    PORT_ZFree(nonce, nonce_len);
		    nonce = NULL;
		    nonce_len = 0;
		}
		if (personalization_string) {
		    PORT_ZFree(personalization_string, ps_len);
		    personalization_string = NULL;
		    ps_len = 0;
		}
		break;
	    case GENERATE:
		rv = PRNGTEST_Generate(return_bytes, return_bytes_len,
				      additional_input, additional_len);
		if (rv != SECSuccess) {
		    goto loser;
		}
		
		if (predicted_return_bytes) {
		    fputc('+', rngresp);
		}
	   	fputs("Returned bits = ", rngresp);
		to_hex_str(buf2, return_bytes, return_bytes_len);
		fputs(buf2, rngresp);
		fputc('\n', rngresp);

		if (predicted_return_bytes) {
		    if (memcmp(return_bytes, 
			   predicted_return_bytes, return_bytes_len) != 0) {
			fprintf(stderr, "Generate failed:\n");
			fputs(  "   predicted=", stderr);
			to_hex_str(buf, predicted_return_bytes, 
							return_bytes_len);
			fputs(buf, stderr);
			fputs("\n   actual  = ", stderr);
			fputs(buf2, stderr);
			fputc('\n', stderr);
		    }
		    PORT_ZFree(predicted_return_bytes, return_bytes_len);
		    predicted_return_bytes = NULL;
		}
			
		if (return_bytes) {
		    PORT_ZFree(return_bytes, return_bytes_len);
		    return_bytes = NULL;
		    return_bytes_len = 0;
		}
		if (additional_input) {
		    PORT_ZFree(additional_input, additional_len);
		    additional_input = NULL;
		    additional_len = 0;
		}
		
		break;
	    case RESEED:
		rv = PRNGTEST_Reseed(entropy, entropy_len,
				      additional_input, additional_len);
		if (rv != SECSuccess) {
		    goto loser;
		}
		
		if (entropy) {
		    PORT_ZFree(entropy, entropy_len);
		    entropy = NULL;
		    entropy_len = 0;
		}
		if (additional_input) {
		    PORT_ZFree(additional_input, additional_len);
		    additional_input = NULL;
		    additional_len = 0;
		}
		break;
	    case UNINSTANTIATE:
		rv = PRNGTEST_Uninstantiate();
		if (rv != SECSuccess) {
		    goto loser;
		}
		break;
	    } 
	    fputs(buf, rngresp);
	    command = NONE;
	    continue;
	}

	
	if (buf[0] == '[') {
	    fputs(buf, rngresp);
	    continue;
	}
	
	if (strncmp(buf, "INSTANTIATE", 11) == 0) {
	    i = 11;

	    command = INSTANTIATE;
	    fputs(buf, rngresp);
	    continue;
	}
	
	if (strncmp(buf, "GENERATE", 8) == 0) {
	    i = 8;
	    while (isspace(buf[i])) {
		i++;
	    }
	    return_bytes_len = atoi(&buf[i])/8;
	    return_bytes = PORT_Alloc(return_bytes_len);
	    command = GENERATE;
	    fputs(buf, rngresp);
	    continue;
	}
	if (strncmp(buf, "RESEED", 6) == 0) {
	    i = 6;
	    command = RESEED;
	    fputs(buf, rngresp);
	    continue;
	}
	if (strncmp(buf, "UNINSTANTIATE", 13) == 0) {
	    i = 13;
	    command = UNINSTANTIATE;
	    fputs(buf, rngresp);
	    continue;
	}
	
	if (strncmp(buf, "Entropy input", 13) == 0) {
	    i = 13;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }

	    if ((command == INSTANTIATE) || (command == RESEED)) {
		entropy = alloc_value(&buf[i], &entropy_len);
	    }
	    
	    fputs(buf, rngresp);
	    continue;
	}
	
	if (strncmp(buf, "Nonce", 5) == 0) {
	    i = 5;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }

	    if (command == INSTANTIATE) {
		nonce = alloc_value(&buf[i], &nonce_len);
	    }
	    
	    fputs(buf, rngresp);
	    continue;
	}
	
	if (strncmp(buf, "Personalization string", 22) == 0) {
	    i = 22;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }

	    if (command == INSTANTIATE) {
		personalization_string = alloc_value(&buf[i], &ps_len);
	    }
	    
	    fputs(buf, rngresp);
	    continue;
	}
	
	if (strncmp(buf, "Returned bits", 13) == 0) {
	    i = 13;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }

	    if (command == GENERATE) {
		int len;
		predicted_return_bytes = alloc_value(&buf[i], &len);
	    }
	    
	    fputs(buf, rngresp);
	    continue;
	}
	
	if (strncmp(buf, "Additional input", 16) == 0) {
	    i = 16;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }

	    if ((command == GENERATE) || (command = RESEED)) {
		additional_input = alloc_value(&buf[i], &additional_len);
	    }
	    
	    fputs(buf, rngresp);
	    continue;
	}
    }
loser:
    fclose(rngreq);
}











void
rng_vst(char *reqfn)
{
    char buf[256];      



    FILE *rngreq;       
    FILE *rngresp;      
    unsigned int i, j;
    unsigned char Q[DSA_SUBPRIME_LEN];
    PRBool hasQ = PR_FALSE;
    unsigned int b;  
    unsigned char XKey[512/8];
    unsigned char XSeed[512/8];
    unsigned char GENX[2*SHA1_LENGTH];
    unsigned char DSAX[DSA_SUBPRIME_LEN];
    SECStatus rv;

    rngreq = fopen(reqfn, "r");
    rngresp = stdout;
    while (fgets(buf, sizeof buf, rngreq) != NULL) {
	
	if (buf[0] == '#' || buf[0] == '\n') {
	    fputs(buf, rngresp);
	    continue;
	}
	
	if (buf[0] == '[') {
	    fputs(buf, rngresp);
	    continue;
	}
	
	if (buf[0] == 'Q') {
	    i = 1;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    for (j=0; j<sizeof Q; i+=2,j++) {
		hex_to_byteval(&buf[i], &Q[j]);
	    }
	    fputs(buf, rngresp);
	    hasQ = PR_TRUE;
	    continue;
	}
	
	if (strncmp(buf, "COUNT", 5) == 0) {
	    
	    b = 0;
	    memset(XKey, 0, sizeof XKey);
	    memset(XSeed, 0, sizeof XSeed);
	    fputs(buf, rngresp);
	    continue;
	}
	
	if (buf[0] == 'b') {
	    i = 1;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    b = atoi(&buf[i]);
	    if (b < 160 || b > 512 || b%8 != 0) {
		goto loser;
	    }
	    fputs(buf, rngresp);
	    continue;
	}
	
	if (strncmp(buf, "XKey", 4) == 0) {
	    i = 4;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    for (j=0; j<b/8; i+=2,j++) {
		hex_to_byteval(&buf[i], &XKey[j]);
	    }
	    fputs(buf, rngresp);
	    continue;
	}
	
	if (strncmp(buf, "XSeed", 5) == 0) {
	    i = 5;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    for (j=0; j<b/8; i+=2,j++) {
		hex_to_byteval(&buf[i], &XSeed[j]);
	    }
	    fputs(buf, rngresp);

	    rv = FIPS186Change_GenerateX(XKey, XSeed, GENX);
	    if (rv != SECSuccess) {
		goto loser;
	    }
	    fputs("X = ", rngresp);
	    if (hasQ) {
		rv = FIPS186Change_ReduceModQForDSA(GENX, Q, DSAX);
		if (rv != SECSuccess) {
		    goto loser;
		}
		to_hex_str(buf, DSAX, sizeof DSAX);
	    } else {
		to_hex_str(buf, GENX, sizeof GENX);
	    }
	    fputs(buf, rngresp);
	    fputc('\n', rngresp);
	    continue;
	}
    }
loser:
    fclose(rngreq);
}











void
rng_mct(char *reqfn)
{
    char buf[256];      



    FILE *rngreq;       
    FILE *rngresp;      
    unsigned int i, j;
    unsigned char Q[DSA_SUBPRIME_LEN];
    PRBool hasQ = PR_FALSE;
    unsigned int b;  
    unsigned char XKey[512/8];
    unsigned char XSeed[512/8];
    unsigned char GENX[2*SHA1_LENGTH];
    unsigned char DSAX[DSA_SUBPRIME_LEN];
    SECStatus rv;

    rngreq = fopen(reqfn, "r");
    rngresp = stdout;
    while (fgets(buf, sizeof buf, rngreq) != NULL) {
	
	if (buf[0] == '#' || buf[0] == '\n') {
	    fputs(buf, rngresp);
	    continue;
	}
	
	if (buf[0] == '[') {
	    fputs(buf, rngresp);
	    continue;
	}
	
	if (buf[0] == 'Q') {
	    i = 1;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    for (j=0; j<sizeof Q; i+=2,j++) {
		hex_to_byteval(&buf[i], &Q[j]);
	    }
	    fputs(buf, rngresp);
	    hasQ = PR_TRUE;
	    continue;
	}
	
	if (strncmp(buf, "COUNT", 5) == 0) {
	    
	    b = 0;
	    memset(XKey, 0, sizeof XKey);
	    memset(XSeed, 0, sizeof XSeed);
	    fputs(buf, rngresp);
	    continue;
	}
	
	if (buf[0] == 'b') {
	    i = 1;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    b = atoi(&buf[i]);
	    if (b < 160 || b > 512 || b%8 != 0) {
		goto loser;
	    }
	    fputs(buf, rngresp);
	    continue;
	}
	
	if (strncmp(buf, "XKey", 4) == 0) {
	    i = 4;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    for (j=0; j<b/8; i+=2,j++) {
		hex_to_byteval(&buf[i], &XKey[j]);
	    }
	    fputs(buf, rngresp);
	    continue;
	}
	
	if (strncmp(buf, "XSeed", 5) == 0) {
	    unsigned int k;
	    i = 5;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    for (j=0; j<b/8; i+=2,j++) {
		hex_to_byteval(&buf[i], &XSeed[j]);
	    }
	    fputs(buf, rngresp);

	    for (k = 0; k < 10000; k++) {
		rv = FIPS186Change_GenerateX(XKey, XSeed, GENX);
		if (rv != SECSuccess) {
		    goto loser;
		}
	    }
	    fputs("X = ", rngresp);
	    if (hasQ) {
		rv = FIPS186Change_ReduceModQForDSA(GENX, Q, DSAX);
		if (rv != SECSuccess) {
		    goto loser;
		}
		to_hex_str(buf, DSAX, sizeof DSAX);
	    } else {
		to_hex_str(buf, GENX, sizeof GENX);
	    }
	    fputs(buf, rngresp);
	    fputc('\n', rngresp);
	    continue;
	}
    }
loser:
    fclose(rngreq);
}









SECStatus sha_calcMD(unsigned char *MD, unsigned int MDLen, unsigned char *msg, unsigned int msgLen) 
{    
    SECStatus   sha_status = SECFailure;

    if (MDLen == SHA1_LENGTH) {
        sha_status = SHA1_HashBuf(MD, msg, msgLen);
    } else if (MDLen == SHA256_LENGTH) {
        sha_status = SHA256_HashBuf(MD, msg, msgLen);
    } else if (MDLen == SHA384_LENGTH) {
        sha_status = SHA384_HashBuf(MD, msg, msgLen);
    } else if (MDLen == SHA512_LENGTH) {
        sha_status = SHA512_HashBuf(MD, msg, msgLen);
    }

    return sha_status;
}








SECStatus sha_mct_test(unsigned int MDLen, unsigned char *seed, FILE *resp) 
{
    int i, j;
    unsigned int msgLen = MDLen*3;
    unsigned char MD_i3[HASH_LENGTH_MAX];  
    unsigned char MD_i2[HASH_LENGTH_MAX];  
    unsigned char MD_i1[HASH_LENGTH_MAX];  
    unsigned char MD_i[HASH_LENGTH_MAX];   
    unsigned char msg[HASH_LENGTH_MAX*3];
    char buf[HASH_LENGTH_MAX*2 + 1];  

    for (j=0; j<100; j++) {
        
        memcpy(MD_i3, seed, MDLen);
        memcpy(MD_i2, seed, MDLen);
        memcpy(MD_i1, seed, MDLen);

        for (i=3; i < 1003; i++) {
            
            memcpy(msg, MD_i3, MDLen);
            memcpy(&msg[MDLen], MD_i2, MDLen);
            memcpy(&msg[MDLen*2], MD_i1,MDLen); 

            
            if (sha_calcMD(MD_i, MDLen,   
                           msg, msgLen) != SECSuccess) {
                return SECFailure;
            }

            
            memcpy(MD_i3, MD_i2, MDLen);
            memcpy(MD_i2, MD_i1, MDLen);
            memcpy(MD_i1, MD_i, MDLen);

        }

        
        memcpy(seed, MD_i, MDLen);

        sprintf(buf, "COUNT = %d\n", j);
        fputs(buf, resp);

        
        fputs("MD = ", resp);
        to_hex_str(buf, MD_i, MDLen);
        fputs(buf, resp);
        fputc('\n', resp);
    }

    return SECSuccess;
}








void sha_test(char *reqfn) 
{
    unsigned int i, j;
    unsigned int MDlen;   
    unsigned int msgLen;  
    unsigned char *msg = NULL; 
    size_t bufSize = 25608; 
    char *buf = NULL;      
    unsigned char seed[HASH_LENGTH_MAX];   
    unsigned char MD[HASH_LENGTH_MAX];     

    FILE *req = NULL;  
    FILE *resp;        

    buf = PORT_ZAlloc(bufSize);
    if (buf == NULL) {
        goto loser;
    }      

    
    memset(seed, 0, sizeof seed);

    req = fopen(reqfn, "r");
    resp = stdout;
    while (fgets(buf, bufSize, req) != NULL) {

        
        if (buf[0] == '#' || buf[0] == '\n') {
            fputs(buf, resp);
            continue;
        }
        
        if (buf[0] == '[') {
            if (strncmp(&buf[1], "L ", 1) == 0) {
                i = 2;
                while (isspace(buf[i]) || buf[i] == '=') {
                    i++;
                }
                MDlen = atoi(&buf[i]);
                fputs(buf, resp);
                continue;
            }
        }
        
        if (strncmp(buf, "Len", 3) == 0) {
            i = 3;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            if (msg) {
                PORT_ZFree(msg,msgLen);
                msg = NULL;
            }
            msgLen = atoi(&buf[i]); 
            if (msgLen%8 != 0) {
                fprintf(stderr, "SHA tests are incorrectly configured for "
                    "BIT oriented implementations\n");
                goto loser;
            }
            msgLen = msgLen/8; 
            fputs(buf, resp);
            msg = PORT_ZAlloc(msgLen);
            if (msg == NULL && msgLen != 0) {
                goto loser;
            } 
            continue;
        }
        
        if (strncmp(buf, "Msg", 3) == 0) {
            i = 3;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=0; j< msgLen; i+=2,j++) {
                hex_to_byteval(&buf[i], &msg[j]);
            }
           fputs(buf, resp);
            
           memset(MD, 0, sizeof MD);
           if (sha_calcMD(MD, MDlen,   
                          msg, msgLen) != SECSuccess) {
               goto loser;
           }

           fputs("MD = ", resp);
           to_hex_str(buf, MD, MDlen);
           fputs(buf, resp);
           fputc('\n', resp);

           continue;
        }
        
        if (strncmp(buf, "Seed", 4) == 0) {
            i = 4;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=0; j<sizeof seed; i+=2,j++) {
                hex_to_byteval(&buf[i], &seed[j]);
            }                                     

            fputs(buf, resp);
            fputc('\n', resp);

            
            if (sha_mct_test(MDlen, seed, resp) != SECSuccess) {
                goto loser; 
            }

            continue;
        }
    }
loser:
    if (req) {
        fclose(req);
    }  
    if (buf) {
        PORT_ZFree(buf, bufSize);
    }
    if (msg) {
        PORT_ZFree(msg, msgLen);
    }
}










static SECStatus
hmac_calc(unsigned char *hmac_computed,
          const unsigned int hmac_length,
          const unsigned char *secret_key,
          const unsigned int secret_key_length,
          const unsigned char *message,
          const unsigned int message_length,
          const HASH_HashType hashAlg )
{
    SECStatus hmac_status = SECFailure;
    HMACContext *cx = NULL;
    SECHashObject *hashObj = NULL;
    unsigned int bytes_hashed = 0;

    hashObj = (SECHashObject *) HASH_GetRawHashObject(hashAlg);
 
    if (!hashObj) 
        return( SECFailure );

    cx = HMAC_Create(hashObj, secret_key, 
                     secret_key_length, 
                     PR_TRUE);  

    if (cx == NULL) 
        return( SECFailure );

    HMAC_Begin(cx);
    HMAC_Update(cx, message, message_length);
    hmac_status = HMAC_Finish(cx, hmac_computed, &bytes_hashed, 
                              hmac_length);

    HMAC_Destroy(cx, PR_TRUE);

    return( hmac_status );
}








void hmac_test(char *reqfn) 
{
    unsigned int i, j;
    size_t bufSize =      288;    
    char *buf = NULL;  
    unsigned int keyLen;            
    unsigned char key[140];       
    unsigned int msgLen = 128;    
                                  
    unsigned char *msg = NULL;    
    unsigned int HMACLen;         
    unsigned char HMAC[HASH_LENGTH_MAX];  
    HASH_HashType hash_alg;       

    FILE *req = NULL;  
    FILE *resp;        

    buf = PORT_ZAlloc(bufSize);
    if (buf == NULL) {
        goto loser;
    }      
    msg = PORT_ZAlloc(msgLen);
    memset(msg, 0, msgLen);
    if (msg == NULL) {
        goto loser;
    } 

    req = fopen(reqfn, "r");
    resp = stdout;
    while (fgets(buf, bufSize, req) != NULL) {

        
        if (buf[0] == '#' || buf[0] == '\n') {
            fputs(buf, resp);
            continue;
        }
        
        if (buf[0] == '[') {
            if (strncmp(&buf[1], "L ", 1) == 0) {
                i = 2;
                while (isspace(buf[i]) || buf[i] == '=') {
                    i++;
                }
                
                HMACLen = atoi(&buf[i]);
                
                if (HMACLen == SHA1_LENGTH) {
                    hash_alg = HASH_AlgSHA1;
                } else if (HMACLen == SHA256_LENGTH) {
                    hash_alg = HASH_AlgSHA256;
                } else if (HMACLen == SHA384_LENGTH) {
                    hash_alg = HASH_AlgSHA384;
                } else if (HMACLen == SHA512_LENGTH) {
                    hash_alg = HASH_AlgSHA512;
                } else {
                    goto loser;
                }
                fputs(buf, resp);
                continue;
            }
        }
        
        if (strncmp(buf, "Count ", 5) == 0) {    
            
            fputs(buf, resp);
            
            keyLen = 0; 
            HMACLen = 0;
            memset(key, 0, sizeof key);     
            memset(msg, 0, sizeof msg);  
            memset(HMAC, 0, sizeof HMAC);
            continue;
        }
        
        if (strncmp(buf, "Klen", 4) == 0) {
            i = 4;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            keyLen = atoi(&buf[i]); 
            fputs(buf, resp);
            continue;
        }
        
        if (strncmp(buf, "Key", 3) == 0) {
            i = 3;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=0; j< keyLen; i+=2,j++) {
                hex_to_byteval(&buf[i], &key[j]);
            }
           fputs(buf, resp);
        }
        
        if (strncmp(buf, "Tlen", 4) == 0) {
            i = 4;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            HMACLen = atoi(&buf[i]); 
            fputs(buf, resp);
            continue;
        }
        
        if (strncmp(buf, "Msg", 3) == 0) {
            i = 3;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=0; j< msgLen; i+=2,j++) {
                hex_to_byteval(&buf[i], &msg[j]);
            }
           fputs(buf, resp);
            
           if (hmac_calc(HMAC, HMACLen, key, keyLen,   
                         msg, msgLen, hash_alg) != SECSuccess) {
               goto loser;
           }
           fputs("MAC = ", resp);
           to_hex_str(buf, HMAC, HMACLen);
           fputs(buf, resp);
           fputc('\n', resp);
           continue;
        }
    }
loser:
    if (req) {
        fclose(req);
    }
    if (buf) {
        PORT_ZFree(buf, bufSize);
    }
    if (msg) {
        PORT_ZFree(msg, msgLen);
    }
}








void
dsa_keypair_test(char *reqfn)
{
    char buf[260];       



    FILE *dsareq;     
    FILE *dsaresp;    
    int N;            
    int modulus;
    int i;
    PQGParams *pqg = NULL;
    PQGVerify *vfy = NULL;
    int keySizeIndex;   

    dsareq = fopen(reqfn, "r");
    dsaresp = stdout;
    while (fgets(buf, sizeof buf, dsareq) != NULL) {
        
        if (buf[0] == '#' || buf[0] == '\n') {
            fputs(buf, dsaresp);
            continue;
        }

        
        if (buf[0] == '[') {
            if(pqg!=NULL) {
                PQG_DestroyParams(pqg);
                pqg = NULL;
            }
            if(vfy!=NULL) {
                PQG_DestroyVerify(vfy);
                vfy = NULL;
            }

            if (sscanf(buf, "[mod = %d]", &modulus) != 1) {
                goto loser;
            }
            fputs(buf, dsaresp);
            fputc('\n', dsaresp);

            



            keySizeIndex = PQG_PBITS_TO_INDEX(modulus);
            if(keySizeIndex == -1 || modulus<512 || modulus>1024) {
               fprintf(dsaresp,
                    "DSA key size must be a multiple of 64 between 512 "
                    "and 1024, inclusive");
                goto loser;
            }

            
            if (PQG_ParamGenSeedLen(keySizeIndex, PQG_TEST_SEED_BYTES,
                &pqg, &vfy) != SECSuccess) {
                fprintf(dsaresp, "ERROR: Unable to generate PQG parameters");
                goto loser;
            }

            
            to_hex_str(buf, pqg->prime.data, pqg->prime.len);
            fprintf(dsaresp, "P = %s\n", buf);
            to_hex_str(buf, pqg->subPrime.data, pqg->subPrime.len);
            fprintf(dsaresp, "Q = %s\n", buf);
            to_hex_str(buf, pqg->base.data, pqg->base.len);
            fprintf(dsaresp, "G = %s\n\n", buf);
            continue;
        }
        
        if (buf[0] == 'N') {

            if (sscanf(buf, "N = %d", &N) != 1) {
                goto loser;
            }
            
            for (i = 0; i < N; i++) {
                DSAPrivateKey *dsakey = NULL;
                if (DSA_NewKey(pqg, &dsakey) != SECSuccess) {
                    fprintf(dsaresp, "ERROR: Unable to generate DSA key");
                    goto loser;
                }
                to_hex_str(buf, dsakey->privateValue.data,
                           dsakey->privateValue.len);
                fprintf(dsaresp, "X = %s\n", buf);
                to_hex_str(buf, dsakey->publicValue.data,
                           dsakey->publicValue.len);
                fprintf(dsaresp, "Y = %s\n\n", buf);
                PORT_FreeArena(dsakey->params.arena, PR_TRUE);
                dsakey = NULL;
            }
            continue;
        }

    }
loser:
    fclose(dsareq);
}








void
dsa_pqgver_test(char *reqfn)
{
    char buf[263];      



    FILE *dsareq;     
    FILE *dsaresp;    
    int modulus; 
    unsigned int i, j;
    PQGParams pqg;
    PQGVerify vfy;
    unsigned int pghSize;        

    dsareq = fopen(reqfn, "r");
    dsaresp = stdout;
    memset(&pqg, 0, sizeof(pqg));
    memset(&vfy, 0, sizeof(vfy));

    while (fgets(buf, sizeof buf, dsareq) != NULL) {
        
        if (buf[0] == '#' || buf[0] == '\n') {
            fputs(buf, dsaresp);
            continue;
        }

        
        if (buf[0] == '[') {

            if (sscanf(buf, "[mod = %d]", &modulus) != 1) {
                goto loser;
            }

            if (pqg.prime.data) { 
                SECITEM_ZfreeItem(&pqg.prime, PR_FALSE);
            }
            if (pqg.subPrime.data) { 
                SECITEM_ZfreeItem(&pqg.subPrime, PR_FALSE);
            }
            if (pqg.base.data) {    
                SECITEM_ZfreeItem(&pqg.base, PR_FALSE);
            }
            if (vfy.seed.data) {   
                SECITEM_ZfreeItem(&vfy.seed, PR_FALSE);
            }
            if (vfy.h.data) {     
                SECITEM_ZfreeItem(&vfy.h, PR_FALSE);
            }

            fputs(buf, dsaresp);

            
            pghSize = modulus/8;
            SECITEM_AllocItem(NULL, &pqg.prime, pghSize);
            SECITEM_AllocItem(NULL, &pqg.base, pghSize);
            SECITEM_AllocItem(NULL, &vfy.h, pghSize);
            pqg.prime.len = pqg.base.len = vfy.h.len = pghSize;
            
            SECITEM_AllocItem(NULL, &vfy.seed, 20);
            SECITEM_AllocItem(NULL, &pqg.subPrime, 20);
            vfy.seed.len = pqg.subPrime.len = 20;
            vfy.counter = 0;

            continue;
        }
        
        if (buf[0] == 'P') {
            i = 1;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=0; j< pqg.prime.len; i+=2,j++) {
                hex_to_byteval(&buf[i], &pqg.prime.data[j]);
            }

            fputs(buf, dsaresp);
            continue;
        }

        
        if (buf[0] == 'Q') {
            i = 1;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=0; j< pqg.subPrime.len; i+=2,j++) {
                hex_to_byteval(&buf[i], &pqg.subPrime.data[j]);
            }

            fputs(buf, dsaresp);
            continue;
        }

        
        if (buf[0] == 'G') {
            i = 1;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=0; j< pqg.base.len; i+=2,j++) {
                hex_to_byteval(&buf[i], &pqg.base.data[j]);
            }

            fputs(buf, dsaresp);
            continue;
        }

        
        if (strncmp(buf, "Seed", 4) == 0) {
            i = 4;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=0; j< vfy.seed.len; i+=2,j++) {
                hex_to_byteval(&buf[i], &vfy.seed.data[j]);
            }

            fputs(buf, dsaresp);
            continue;
        }

        
        if (buf[0] == 'c') {

            if (sscanf(buf, "c = %u", &vfy.counter) != 1) {
                goto loser;
            }

            fputs(buf, dsaresp);
            continue;
        }

        
        if (buf[0] == 'H') {
            SECStatus rv, result = SECFailure;

            i = 1;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=0; j< vfy.h.len; i+=2,j++) {
                hex_to_byteval(&buf[i], &vfy.h.data[j]);
            }
            fputs(buf, dsaresp);

            
            rv = PQG_VerifyParams(&pqg, &vfy, &result);
            if (rv != SECSuccess) {
                goto loser;
            }
            if (result == SECSuccess) {
                fprintf(dsaresp, "Result = P\n");
            } else {
                fprintf(dsaresp, "Result = F\n");
            }
            continue;
        }
    }
loser:
    fclose(dsareq);
    if (pqg.prime.data) { 
        SECITEM_ZfreeItem(&pqg.prime, PR_FALSE);
    }
    if (pqg.subPrime.data) { 
        SECITEM_ZfreeItem(&pqg.subPrime, PR_FALSE);
    }
    if (pqg.base.data) {    
        SECITEM_ZfreeItem(&pqg.base, PR_FALSE);
    }
    if (vfy.seed.data) {   
        SECITEM_ZfreeItem(&vfy.seed, PR_FALSE);
    }
    if (vfy.h.data) {     
        SECITEM_ZfreeItem(&vfy.h, PR_FALSE);
    }

}








void
dsa_pqggen_test(char *reqfn)
{
    char buf[263];      



    FILE *dsareq;     
    FILE *dsaresp;    
    int N;            
    int modulus; 
    int i;
    unsigned int j;
    PQGParams *pqg = NULL;
    PQGVerify *vfy = NULL;
    unsigned int keySizeIndex;

    dsareq = fopen(reqfn, "r");
    dsaresp = stdout;
    while (fgets(buf, sizeof buf, dsareq) != NULL) {
        
        if (buf[0] == '#' || buf[0] == '\n') {
            fputs(buf, dsaresp);
            continue;
        }

        
        if (buf[0] == '[') {

            if (sscanf(buf, "[mod = %d]", &modulus) != 1) {
                goto loser;
            }

            fputs(buf, dsaresp);
            fputc('\n', dsaresp);

            



            keySizeIndex = PQG_PBITS_TO_INDEX(modulus);
            if(keySizeIndex == -1 || modulus<512 || modulus>1024) {
               fprintf(dsaresp,
                    "DSA key size must be a multiple of 64 between 512 "
                    "and 1024, inclusive");
                goto loser;
            }

            continue;
        }
        
        if (buf[0] == 'N') {

            if (sscanf(buf, "N = %d", &N) != 1) {
                goto loser;
            }
            for (i = 0; i < N; i++) {
                if (PQG_ParamGenSeedLen(keySizeIndex, PQG_TEST_SEED_BYTES,
                    &pqg, &vfy) != SECSuccess) {
                    fprintf(dsaresp,
                            "ERROR: Unable to generate PQG parameters");
                    goto loser;
                }
                to_hex_str(buf, pqg->prime.data, pqg->prime.len);
                fprintf(dsaresp, "P = %s\n", buf);
                to_hex_str(buf, pqg->subPrime.data, pqg->subPrime.len);
                fprintf(dsaresp, "Q = %s\n", buf);
                to_hex_str(buf, pqg->base.data, pqg->base.len);
                fprintf(dsaresp, "G = %s\n", buf);
                to_hex_str(buf, vfy->seed.data, vfy->seed.len);
                fprintf(dsaresp, "Seed = %s\n", buf);
                fprintf(dsaresp, "c = %d\n", vfy->counter);
                to_hex_str(buf, vfy->h.data, vfy->h.len);
                fputs("H = ", dsaresp);
                for (j=vfy->h.len; j<pqg->prime.len; j++) {
                    fprintf(dsaresp, "00");
                }
                fprintf(dsaresp, "%s\n", buf);
                fputc('\n', dsaresp);
                if(pqg!=NULL) {
                    PQG_DestroyParams(pqg);
                    pqg = NULL;
                }
                if(vfy!=NULL) {
                    PQG_DestroyVerify(vfy);
                    vfy = NULL;
                }
            }

            continue;
        }

    }
loser:
    fclose(dsareq);
    if(pqg!=NULL) {
        PQG_DestroyParams(pqg);
    }
    if(vfy!=NULL) {
        PQG_DestroyVerify(vfy);
    }
}








void
dsa_siggen_test(char *reqfn)
{
    char buf[263];       



    FILE *dsareq;     
    FILE *dsaresp;    
    int modulus;          
    int i, j;
    PQGParams *pqg = NULL;
    PQGVerify *vfy = NULL;
    DSAPrivateKey *dsakey = NULL;
    int keySizeIndex;     
    unsigned char sha1[20];  
    unsigned char sig[DSA_SIGNATURE_LEN];
    SECItem digest, signature;

    dsareq = fopen(reqfn, "r");
    dsaresp = stdout;

    while (fgets(buf, sizeof buf, dsareq) != NULL) {
        
        if (buf[0] == '#' || buf[0] == '\n') {
            fputs(buf, dsaresp);
            continue;
        }

        
        if (buf[0] == '[') {
            if(pqg!=NULL) {
                PQG_DestroyParams(pqg);
                pqg = NULL;
            }
            if(vfy!=NULL) {
                PQG_DestroyVerify(vfy);
                vfy = NULL;
            }
            if (dsakey != NULL) {
                    PORT_FreeArena(dsakey->params.arena, PR_TRUE);
                    dsakey = NULL;
            }

            if (sscanf(buf, "[mod = %d]", &modulus) != 1) {
                goto loser;
            }
            fputs(buf, dsaresp);
            fputc('\n', dsaresp);

            



            keySizeIndex = PQG_PBITS_TO_INDEX(modulus);
            if(keySizeIndex == -1 || modulus<512 || modulus>1024) {
                fprintf(dsaresp,
                    "DSA key size must be a multiple of 64 between 512 "
                    "and 1024, inclusive");
                goto loser;
            }

            
            if (PQG_ParamGenSeedLen(keySizeIndex, PQG_TEST_SEED_BYTES,
                &pqg, &vfy) != SECSuccess) {
                fprintf(dsaresp, "ERROR: Unable to generate PQG parameters");
                goto loser;
            }
            to_hex_str(buf, pqg->prime.data, pqg->prime.len);
            fprintf(dsaresp, "P = %s\n", buf);
            to_hex_str(buf, pqg->subPrime.data, pqg->subPrime.len);
            fprintf(dsaresp, "Q = %s\n", buf);
            to_hex_str(buf, pqg->base.data, pqg->base.len);
            fprintf(dsaresp, "G = %s\n", buf);

            
            if (DSA_NewKey(pqg, &dsakey) != SECSuccess) {
                fprintf(dsaresp, "ERROR: Unable to generate DSA key");
                goto loser;
            }
            continue;
        }

        
        if (strncmp(buf, "Msg", 3) == 0) {
            unsigned char msg[128]; 
            unsigned int len = 0;

            memset(sha1, 0, sizeof sha1);
            memset(sig,  0, sizeof sig);

            i = 3;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=0; isxdigit(buf[i]); i+=2,j++) {
                hex_to_byteval(&buf[i], &msg[j]);
            }
            if (SHA1_HashBuf(sha1, msg, j) != SECSuccess) {
                 fprintf(dsaresp, "ERROR: Unable to generate SHA1 digest");
                 goto loser;
            }

            digest.type = siBuffer;
            digest.data = sha1;
            digest.len = sizeof sha1;
            signature.type = siBuffer;
            signature.data = sig;
            signature.len = sizeof sig;

            if (DSA_SignDigest(dsakey, &signature, &digest) != SECSuccess) {
                fprintf(dsaresp, "ERROR: Unable to generate DSA signature");
                goto loser;
            }
            len = signature.len;
            if (len%2 != 0) {
                goto loser;
            }
            len = len/2;

            
            fputs(buf, dsaresp);
            fputc('\n', dsaresp);
            to_hex_str(buf, dsakey->publicValue.data,
                       dsakey->publicValue.len);
            fprintf(dsaresp, "Y = %s\n", buf);
            to_hex_str(buf, &signature.data[0], len);
            fprintf(dsaresp, "R = %s\n", buf);
            to_hex_str(buf, &signature.data[len], len);
            fprintf(dsaresp, "S = %s\n", buf);
            continue;
        }

    }
loser:
    fclose(dsareq);
    if(pqg != NULL) {
        PQG_DestroyParams(pqg);
        pqg = NULL;
    }
    if(vfy != NULL) {
        PQG_DestroyVerify(vfy);
        vfy = NULL;
    }
    if (dsaKey) {
        PORT_FreeArena(dsakey->params.arena, PR_TRUE);
        dsakey = NULL;
    }
}

 






void
dsa_sigver_test(char *reqfn)
{
    char buf[263];       



    FILE *dsareq;     
    FILE *dsaresp;    
    int modulus;  
    unsigned int i, j;
    SECItem digest, signature;
    DSAPublicKey pubkey;
    unsigned int pgySize;        
    unsigned char sha1[20];  
    unsigned char sig[DSA_SIGNATURE_LEN];

    dsareq = fopen(reqfn, "r");
    dsaresp = stdout;
    memset(&pubkey, 0, sizeof(pubkey));

    while (fgets(buf, sizeof buf, dsareq) != NULL) {
        
        if (buf[0] == '#' || buf[0] == '\n') {
            fputs(buf, dsaresp);
            continue;
        }

        
        if (buf[0] == '[') {

            if (sscanf(buf, "[mod = %d]", &modulus) != 1) {
                goto loser;
            }

            if (pubkey.params.prime.data) { 
                SECITEM_ZfreeItem(&pubkey.params.prime, PR_FALSE);
            }
            if (pubkey.params.subPrime.data) { 
                SECITEM_ZfreeItem(&pubkey.params.subPrime, PR_FALSE);
            }
            if (pubkey.params.base.data) {    
                SECITEM_ZfreeItem(&pubkey.params.base, PR_FALSE);
            }
            if (pubkey.publicValue.data) {    
                SECITEM_ZfreeItem(&pubkey.publicValue, PR_FALSE);
            }
            fputs(buf, dsaresp);

            
            pgySize = modulus/8;
            SECITEM_AllocItem(NULL, &pubkey.params.prime, pgySize);
            SECITEM_AllocItem(NULL, &pubkey.params.base, pgySize);
            SECITEM_AllocItem(NULL, &pubkey.publicValue, pgySize);
            pubkey.params.prime.len = pubkey.params.base.len = pgySize;
            pubkey.publicValue.len = pgySize;

            
            SECITEM_AllocItem(NULL, &pubkey.params.subPrime, 20);
            pubkey.params.subPrime.len = 20;

            continue;
        }
        
        if (buf[0] == 'P') {
            i = 1;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            memset(pubkey.params.prime.data, 0, pubkey.params.prime.len);
            for (j=0; j< pubkey.params.prime.len; i+=2,j++) {
                hex_to_byteval(&buf[i], &pubkey.params.prime.data[j]);
            }

            fputs(buf, dsaresp);
            continue;
        }

        
        if (buf[0] == 'Q') {
            i = 1;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            memset(pubkey.params.subPrime.data, 0, pubkey.params.subPrime.len);
            for (j=0; j< pubkey.params.subPrime.len; i+=2,j++) {
                hex_to_byteval(&buf[i], &pubkey.params.subPrime.data[j]);
            }

            fputs(buf, dsaresp);
            continue;
        }

        
        if (buf[0] == 'G') {
            i = 1;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            memset(pubkey.params.base.data, 0, pubkey.params.base.len);
            for (j=0; j< pubkey.params.base.len; i+=2,j++) {
                hex_to_byteval(&buf[i], &pubkey.params.base.data[j]);
            }

            fputs(buf, dsaresp);
            continue;
        }

        
        if (strncmp(buf, "Msg", 3) == 0) {
            unsigned char msg[128]; 
            memset(sha1, 0, sizeof sha1);

            i = 3;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=0; isxdigit(buf[i]); i+=2,j++) {
                hex_to_byteval(&buf[i], &msg[j]);
            }
            if (SHA1_HashBuf(sha1, msg, j) != SECSuccess) {
                fprintf(dsaresp, "ERROR: Unable to generate SHA1 digest");
                goto loser;
            }

            fputs(buf, dsaresp);
            continue;
        }

        
        if (buf[0] == 'Y') {
            i = 1;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            memset(pubkey.publicValue.data, 0, pubkey.params.subPrime.len);
            for (j=0; j< pubkey.publicValue.len; i+=2,j++) {
                hex_to_byteval(&buf[i], &pubkey.publicValue.data[j]);
            }

            fputs(buf, dsaresp);
            continue;
        }

        
        if (buf[0] == 'R') {
            memset(sig,  0, sizeof sig);
            i = 1;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=0; j< DSA_SUBPRIME_LEN; i+=2,j++) {
                hex_to_byteval(&buf[i], &sig[j]);
            }

            fputs(buf, dsaresp);
            continue;
        }

        
        if (buf[0] == 'S') {
            i = 1;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=DSA_SUBPRIME_LEN; j< DSA_SIGNATURE_LEN; i+=2,j++) {
                hex_to_byteval(&buf[i], &sig[j]);
            }
            fputs(buf, dsaresp);

            digest.type = siBuffer;
            digest.data = sha1;
            digest.len = sizeof sha1;
            signature.type = siBuffer;
            signature.data = sig;
            signature.len = sizeof sig;

            if (DSA_VerifyDigest(&pubkey, &signature, &digest) == SECSuccess) {
                fprintf(dsaresp, "Result = P\n");
            } else {
                fprintf(dsaresp, "Result = F\n");
            }
            continue;
        }
    }
loser:
    fclose(dsareq);
    if (pubkey.params.prime.data) { 
        SECITEM_ZfreeItem(&pubkey.params.prime, PR_FALSE);
    }
    if (pubkey.params.subPrime.data) { 
        SECITEM_ZfreeItem(&pubkey.params.subPrime, PR_FALSE);
    }
    if (pubkey.params.base.data) {    
        SECITEM_ZfreeItem(&pubkey.params.base, PR_FALSE);
    }
    if (pubkey.publicValue.data) {    
        SECITEM_ZfreeItem(&pubkey.publicValue, PR_FALSE);
    }
}








void
rsa_siggen_test(char *reqfn)
{
    char buf[2*RSA_MAX_TEST_MODULUS_BYTES+1];
                        



    FILE *rsareq;     
    FILE *rsaresp;    
    int i, j;
    unsigned char  sha[HASH_LENGTH_MAX];    
    unsigned int   shaLength = 0;           
    HASH_HashType  shaAlg = HASH_AlgNULL;   
    SECOidTag      shaOid = SEC_OID_UNKNOWN;
    int modulus;                                
    int  publicExponent  = DEFAULT_RSA_PUBLIC_EXPONENT;
    SECItem pe = {0, 0, 0 };
    unsigned char pubEx[4];
    int peCount = 0;

    RSAPrivateKey  *rsaBlapiPrivKey = NULL;   

    RSAPublicKey   *rsaBlapiPublicKey = NULL; 

    rsareq = fopen(reqfn, "r");
    rsaresp = stdout;

    
    for (i=0; i < 4; i++) {
        if (peCount || (publicExponent &
                ((unsigned long)0xff000000L >> (i*8)))) {
            pubEx[peCount] =
                (unsigned char)((publicExponent >> (3-i)*8) & 0xff);
            peCount++;
        }
    }
    pe.len = peCount;
    pe.data = &pubEx[0];
    pe.type = siBuffer;

    while (fgets(buf, sizeof buf, rsareq) != NULL) {
        
        if (buf[0] == '#' || buf[0] == '\n') {
            fputs(buf, rsaresp);
            continue;
        }

        
        if (buf[0] == '[') {

            if (sscanf(buf, "[mod = %d]", &modulus) != 1) {
                goto loser;
            }
            if (modulus > RSA_MAX_TEST_MODULUS_BITS) {
                fprintf(rsaresp,"ERROR: modulus greater than test maximum\n");
                goto loser;
            }

            fputs(buf, rsaresp);

            if (rsaBlapiPrivKey != NULL) {
                PORT_FreeArena(rsaBlapiPrivKey->arena, PR_TRUE);
                rsaBlapiPrivKey = NULL;
                rsaBlapiPublicKey = NULL;
            }

            rsaBlapiPrivKey = RSA_NewKey(modulus, &pe);
            if (rsaBlapiPrivKey == NULL) {
                fprintf(rsaresp, "Error unable to create RSA key\n");
                goto loser;
            }

            to_hex_str(buf, rsaBlapiPrivKey->modulus.data,
                       rsaBlapiPrivKey->modulus.len);
            fprintf(rsaresp, "\nn = %s\n\n", buf);
            to_hex_str(buf, rsaBlapiPrivKey->publicExponent.data,
                       rsaBlapiPrivKey->publicExponent.len);
            fprintf(rsaresp, "e = %s\n", buf);
            

            rsaBlapiPublicKey = (RSAPublicKey *)PORT_ArenaAlloc(
                                                  rsaBlapiPrivKey->arena,
                                                  sizeof(RSAPublicKey));

            rsaBlapiPublicKey->modulus.len = rsaBlapiPrivKey->modulus.len;
            rsaBlapiPublicKey->modulus.data = rsaBlapiPrivKey->modulus.data;
            rsaBlapiPublicKey->publicExponent.len =
                rsaBlapiPrivKey->publicExponent.len;
            rsaBlapiPublicKey->publicExponent.data =
                rsaBlapiPrivKey->publicExponent.data;
            continue;
        }

        
        if (strncmp(buf, "SHAAlg", 6) == 0) {
           i = 6;
           while (isspace(buf[i]) || buf[i] == '=') {
               i++;
           }
           
           if (strncmp(&buf[i], "SHA1", 4) == 0) {
                shaAlg = HASH_AlgSHA1;
           } else if (strncmp(&buf[i], "SHA256", 6) == 0) {
                shaAlg = HASH_AlgSHA256;
           } else if (strncmp(&buf[i], "SHA384", 6)== 0) {
               shaAlg = HASH_AlgSHA384;
           } else if (strncmp(&buf[i], "SHA512", 6) == 0) {
               shaAlg = HASH_AlgSHA512;
           } else {
               fprintf(rsaresp, "ERROR: Unable to find SHAAlg type");
               goto loser;
           }
           fputs(buf, rsaresp);
           continue;

        }
        
        if (strncmp(buf, "Msg", 3) == 0) {

            unsigned char msg[128]; 
            unsigned int rsa_bytes_signed;
            unsigned char rsa_computed_signature[RSA_MAX_TEST_MODULUS_BYTES];
            SECStatus       rv = SECFailure;
            NSSLOWKEYPublicKey  * rsa_public_key;
            NSSLOWKEYPrivateKey * rsa_private_key;
            NSSLOWKEYPrivateKey   low_RSA_private_key = { NULL,
                                                NSSLOWKEYRSAKey, };
            NSSLOWKEYPublicKey    low_RSA_public_key = { NULL,
                                                NSSLOWKEYRSAKey, };

            low_RSA_private_key.u.rsa = *rsaBlapiPrivKey;
            low_RSA_public_key.u.rsa = *rsaBlapiPublicKey;

            rsa_private_key = &low_RSA_private_key;
            rsa_public_key = &low_RSA_public_key;

            memset(sha, 0, sizeof sha);
            memset(msg, 0, sizeof msg);
            rsa_bytes_signed = 0;
            memset(rsa_computed_signature, 0, sizeof rsa_computed_signature);

            i = 3;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=0; isxdigit(buf[i]) && j < sizeof(msg); i+=2,j++) {
                hex_to_byteval(&buf[i], &msg[j]);
            }

            if (shaAlg == HASH_AlgSHA1) {
                if (SHA1_HashBuf(sha, msg, j) != SECSuccess) {
                     fprintf(rsaresp, "ERROR: Unable to generate SHA1");
                     goto loser;
                }
                shaLength = SHA1_LENGTH;
                shaOid = SEC_OID_SHA1;
            } else if (shaAlg == HASH_AlgSHA256) {
                if (SHA256_HashBuf(sha, msg, j) != SECSuccess) {
                     fprintf(rsaresp, "ERROR: Unable to generate SHA256");
                     goto loser;
                }
                shaLength = SHA256_LENGTH;
                shaOid = SEC_OID_SHA256;
            } else if (shaAlg == HASH_AlgSHA384) {
                if (SHA384_HashBuf(sha, msg, j) != SECSuccess) {
                     fprintf(rsaresp, "ERROR: Unable to generate SHA384");
                     goto loser;
                }
                shaLength = SHA384_LENGTH;
                shaOid = SEC_OID_SHA384;
            } else if (shaAlg == HASH_AlgSHA512) {
                if (SHA512_HashBuf(sha, msg, j) != SECSuccess) {
                     fprintf(rsaresp, "ERROR: Unable to generate SHA512");
                     goto loser;
                }
                shaLength = SHA512_LENGTH;
                shaOid = SEC_OID_SHA512;
            } else {
                fprintf(rsaresp, "ERROR: SHAAlg not defined.");
                goto loser;
            }

            
            rv = RSA_HashSign( shaOid,
                               rsa_private_key,
                               rsa_computed_signature,
                               &rsa_bytes_signed,
                               nsslowkey_PrivateModulusLen(rsa_private_key),
                               sha,
                               shaLength);

            if( rv != SECSuccess ) {
                 fprintf(rsaresp, "ERROR: RSA_HashSign failed");
                 goto loser;
            }

            
            fputs(buf, rsaresp);
            to_hex_str(buf, rsa_computed_signature, rsa_bytes_signed);
            fprintf(rsaresp, "S = %s\n", buf);

            
            rv = RSA_HashCheckSign( shaOid,
                                    rsa_public_key,
                                    rsa_computed_signature,
                                    rsa_bytes_signed,
                                    sha,
                                    shaLength);
            if( rv != SECSuccess ) {
                 fprintf(rsaresp, "ERROR: RSA_HashCheckSign failed");
                 goto loser;
            }
            continue;
        }
    }
loser:
    fclose(rsareq);

    if (rsaBlapiPrivKey != NULL) {
        
        PORT_FreeArena(rsaBlapiPrivKey->arena, PR_TRUE);
        rsaBlapiPrivKey = NULL;
        rsaBlapiPublicKey = NULL;
    }

}







void
rsa_sigver_test(char *reqfn)
{
    char buf[2*RSA_MAX_TEST_MODULUS_BYTES+7];
                        



    FILE *rsareq;     
    FILE *rsaresp;    
    int i, j;
    unsigned char   sha[HASH_LENGTH_MAX];   
    unsigned int    shaLength = 0;              
    HASH_HashType   shaAlg = HASH_AlgNULL;
    SECOidTag       shaOid = SEC_OID_UNKNOWN;
    int modulus = 0;                            
    unsigned char   signature[513];    
    unsigned int    signatureLength = 0;   
    PRBool keyvalid = PR_TRUE;

    RSAPublicKey   rsaBlapiPublicKey; 

    rsareq = fopen(reqfn, "r");
    rsaresp = stdout;
    memset(&rsaBlapiPublicKey, 0, sizeof(RSAPublicKey));

    while (fgets(buf, sizeof buf, rsareq) != NULL) {
        
        if (buf[0] == '#' || buf[0] == '\n') {
            fputs(buf, rsaresp);
            continue;
        }

        
        if (buf[0] == '[') {
            unsigned int flen;  

            if (rsaBlapiPublicKey.modulus.data) { 
                SECITEM_ZfreeItem(&rsaBlapiPublicKey.modulus, PR_FALSE);
            }
            if (sscanf(buf, "[mod = %d]", &modulus) != 1) {
                goto loser;
            }

            if (modulus > RSA_MAX_TEST_MODULUS_BITS) {
                fprintf(rsaresp,"ERROR: modulus greater than test maximum\n");
                goto loser;
            }

            fputs(buf, rsaresp);

            signatureLength = flen = modulus/8;

            SECITEM_AllocItem(NULL, &rsaBlapiPublicKey.modulus, flen);
            if (rsaBlapiPublicKey.modulus.data == NULL) {
                goto loser;
            }
            continue;
        }

        
        if (buf[0] == 'n') {
            i = 1;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            keyvalid = from_hex_str(&rsaBlapiPublicKey.modulus.data[0],
                                    rsaBlapiPublicKey.modulus.len,
                                    &buf[i]);

            if (!keyvalid) {
                fprintf(rsaresp, "ERROR: rsa_sigver n not valid.\n");
                                 goto loser;
            }
            fputs(buf, rsaresp);
            continue;
        }

        
        if (strncmp(buf, "SHAAlg", 6) == 0) {
           i = 6;
           while (isspace(buf[i]) || buf[i] == '=') {
               i++;
           }
           
           if (strncmp(&buf[i], "SHA1", 4) == 0) {
                shaAlg = HASH_AlgSHA1;
           } else if (strncmp(&buf[i], "SHA256", 6) == 0) {
                shaAlg = HASH_AlgSHA256;
           } else if (strncmp(&buf[i], "SHA384", 6) == 0) {
               shaAlg = HASH_AlgSHA384;
           } else if (strncmp(&buf[i], "SHA512", 6) == 0) {
               shaAlg = HASH_AlgSHA512;
           } else {
               fprintf(rsaresp, "ERROR: Unable to find SHAAlg type");
               goto loser;
           }
           fputs(buf, rsaresp);
           continue;
        }

        
        if (buf[0] == 'e') {
            unsigned char data[RSA_MAX_TEST_EXPONENT_BYTES];
            unsigned char t;

            memset(data, 0, sizeof data);

            if (rsaBlapiPublicKey.publicExponent.data) { 
                SECITEM_ZfreeItem(&rsaBlapiPublicKey.publicExponent, PR_FALSE);
            }

            i = 1;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            
            while (isxdigit(buf[i])) {
                hex_to_byteval(&buf[i], &t);
                if (t == 0) {
                    i+=2;
                } else break;
            }
        
            
            for (j=0; isxdigit(buf[i]) && j < sizeof data; i+=2,j++) {
                hex_to_byteval(&buf[i], &data[j]);
            }

            if (j == 0) { j = 1; }  

            SECITEM_AllocItem(NULL, &rsaBlapiPublicKey.publicExponent,  j);
            if (rsaBlapiPublicKey.publicExponent.data == NULL) {
                goto loser;
            }

            for (i=0; i < j; i++) {
                rsaBlapiPublicKey.publicExponent.data[i] = data[i];
            }

            fputs(buf, rsaresp);
            continue;
        }

        
        if (strncmp(buf, "Msg", 3) == 0) {
            unsigned char msg[128]; 

            memset(sha, 0, sizeof sha);
            memset(msg, 0, sizeof msg);

            i = 3;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }

            for (j=0; isxdigit(buf[i]) && j < sizeof msg; i+=2,j++) {
                hex_to_byteval(&buf[i], &msg[j]);
            }

            if (shaAlg == HASH_AlgSHA1) {
                if (SHA1_HashBuf(sha, msg, j) != SECSuccess) {
                     fprintf(rsaresp, "ERROR: Unable to generate SHA1");
                     goto loser;
                }
                shaLength = SHA1_LENGTH;
                shaOid = SEC_OID_SHA1;
            } else if (shaAlg == HASH_AlgSHA256) {
                if (SHA256_HashBuf(sha, msg, j) != SECSuccess) {
                     fprintf(rsaresp, "ERROR: Unable to generate SHA256");
                     goto loser;
                }
                shaLength = SHA256_LENGTH;
                shaOid = SEC_OID_SHA256;
            } else if (shaAlg == HASH_AlgSHA384) {
                if (SHA384_HashBuf(sha, msg, j) != SECSuccess) {
                     fprintf(rsaresp, "ERROR: Unable to generate SHA384");
                     goto loser;
                }
                shaLength = SHA384_LENGTH;
                shaOid = SEC_OID_SHA384;
            } else if (shaAlg == HASH_AlgSHA512) {
                if (SHA512_HashBuf(sha, msg, j) != SECSuccess) {
                     fprintf(rsaresp, "ERROR: Unable to generate SHA512");
                     goto loser;
                }
                shaLength = SHA512_LENGTH;
                shaOid = SEC_OID_SHA512;
            } else {
                fprintf(rsaresp, "ERROR: SHAAlg not defined.");
                goto loser;
            }

            fputs(buf, rsaresp);
            continue;

        }

        
        if (buf[0] == 'S') {
            SECStatus rv = SECFailure;
            NSSLOWKEYPublicKey  * rsa_public_key;
            NSSLOWKEYPublicKey    low_RSA_public_key = { NULL,
                                                  NSSLOWKEYRSAKey, };

            
            low_RSA_public_key.u.rsa = rsaBlapiPublicKey;
            rsa_public_key = &low_RSA_public_key;

            memset(signature, 0, sizeof(signature));
            i = 1;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }

            for (j=0; isxdigit(buf[i]) && j < sizeof signature; i+=2,j++) {
                hex_to_byteval(&buf[i], &signature[j]);
            }

            signatureLength = j;
            fputs(buf, rsaresp);

            
            rv = RSA_HashCheckSign( shaOid,
                                    rsa_public_key,
                                    signature,
                                    signatureLength,
                                    sha,
                                    shaLength);
            if( rv == SECSuccess ) {
                fputs("Result = P\n", rsaresp);
            } else {
                fputs("Result = F\n", rsaresp);
            }
            continue;
        }
    }
loser:
    fclose(rsareq);
    if (rsaBlapiPublicKey.modulus.data) { 
        SECITEM_ZfreeItem(&rsaBlapiPublicKey.modulus, PR_FALSE);
    }
    if (rsaBlapiPublicKey.publicExponent.data) { 
        SECITEM_ZfreeItem(&rsaBlapiPublicKey.publicExponent, PR_FALSE);
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) exit (-1);
    NSS_NoDB_Init(NULL);
    
    
    
    if (strcmp(argv[1], "tdea") == 0) {
        
        if (strcmp(argv[2], "kat") == 0) {
            
            tdea_kat_mmt(argv[4]);     
        } else if (strcmp(argv[2], "mmt") == 0) {
            
                tdea_kat_mmt(argv[4]);
        } else if (strcmp(argv[2], "mct") == 0) {
                
                if (strcmp(argv[3], "ecb") == 0) {
                    
                    tdea_mct(NSS_DES_EDE3, argv[4]); 
                } else if (strcmp(argv[3], "cbc") == 0) {
                    
                    tdea_mct(NSS_DES_EDE3_CBC, argv[4]);
                }
        }
    
    
    
    } else if (strcmp(argv[1], "aes") == 0) {
	
	if (       strcmp(argv[2], "kat") == 0) {
	    
	    aes_kat_mmt(argv[4]);
	} else if (strcmp(argv[2], "mmt") == 0) {
	    
	    aes_kat_mmt(argv[4]);
	} else if (strcmp(argv[2], "mct") == 0) {
	    
	    if (       strcmp(argv[3], "ecb") == 0) {
		
		aes_ecb_mct(argv[4]);
	    } else if (strcmp(argv[3], "cbc") == 0) {
		
		aes_cbc_mct(argv[4]);
	    }
	}
    
    
    
    } else if (strcmp(argv[1], "sha") == 0) {
        sha_test(argv[2]);
    
    
    
    } else if (strcmp(argv[1], "rsa") == 0) {
        
        
        if (strcmp(argv[2], "siggen") == 0) {
            
            rsa_siggen_test(argv[3]);
        } else if (strcmp(argv[2], "sigver") == 0) {
            
            rsa_sigver_test(argv[3]);
        }
    
    
    
    } else if (strcmp(argv[1], "hmac") == 0) {
        hmac_test(argv[2]);
    
    
    
    } else if (strcmp(argv[1], "dsa") == 0) {
        
        
        if (strcmp(argv[2], "keypair") == 0) {
            
            dsa_keypair_test(argv[3]);
        } else if (strcmp(argv[2], "pqggen") == 0) {
        
            dsa_pqggen_test(argv[3]);
        } else if (strcmp(argv[2], "pqgver") == 0) {
                
            dsa_pqgver_test(argv[3]);
        } else if (strcmp(argv[2], "siggen") == 0) {
            
            dsa_siggen_test(argv[3]);
        } else if (strcmp(argv[2], "sigver") == 0) {
            
            dsa_sigver_test(argv[3]);
        }
#ifdef NSS_ENABLE_ECC
    
    
    
    } else if (strcmp(argv[1], "ecdsa") == 0) {
	
	if (       strcmp(argv[2], "keypair") == 0) {
	    
	    ecdsa_keypair_test(argv[3]);
	} else if (strcmp(argv[2], "pkv") == 0) {
	    
	    ecdsa_pkv_test(argv[3]);
	} else if (strcmp(argv[2], "siggen") == 0) {
	    
	    ecdsa_siggen_test(argv[3]);
	} else if (strcmp(argv[2], "sigver") == 0) {
	    
	    ecdsa_sigver_test(argv[3]);
	}
#endif



    } else if (strcmp(argv[1], "rng") == 0) {
	
	if (       strcmp(argv[2], "vst") == 0) {
	    
	    rng_vst(argv[3]);
	} else if (strcmp(argv[2], "mct") == 0) {
	    
	    rng_mct(argv[3]);
	}
    } else if (strcmp(argv[1], "drbg") == 0) {
	
	drbg(argv[2]);
    }
    return 0;
}

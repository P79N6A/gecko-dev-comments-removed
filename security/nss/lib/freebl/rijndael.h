




































#ifndef _RIJNDAEL_H_
#define _RIJNDAEL_H_ 1

#define RIJNDAEL_MIN_BLOCKSIZE 16 /* bytes */
#define RIJNDAEL_MAX_BLOCKSIZE 32 /* bytes */

typedef SECStatus AESFunc(AESContext *cx, unsigned char *output,
                          unsigned int *outputLen, unsigned int maxOutputLen,
                          const unsigned char *input, unsigned int inputLen, 
                          unsigned int blocksize);

typedef SECStatus AESBlockFunc(AESContext *cx, 
                               unsigned char *output,
                               const unsigned char *input);







#define RIJNDAEL_NUM_ROUNDS(Nk, Nb) \
    (PR_MAX(Nk, Nb) + 6)






#define RIJNDAEL_MAX_STATE_SIZE 32






#define RIJNDAEL_MAX_EXP_KEY_SIZE (8 * 15)











struct AESContextStr
{
    unsigned int   Nb;
    unsigned int   Nr;
    AESFunc       *worker;
    unsigned char iv[RIJNDAEL_MAX_BLOCKSIZE];
    PRUint32      expandedKey[RIJNDAEL_MAX_EXP_KEY_SIZE];
};

#endif 

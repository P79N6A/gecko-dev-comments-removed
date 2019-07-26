




#ifndef _RIJNDAEL_H_
#define _RIJNDAEL_H_ 1

#include "blapii.h"

#define RIJNDAEL_MIN_BLOCKSIZE 16 /* bytes */
#define RIJNDAEL_MAX_BLOCKSIZE 32 /* bytes */

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
    freeblCipherFunc worker;
    

    unsigned char iv[RIJNDAEL_MAX_BLOCKSIZE];
    PRUint32      expandedKey[RIJNDAEL_MAX_EXP_KEY_SIZE];
    freeblDestroyFunc destroy;
    void	      *worker_cx;
    PRBool	      isBlock;
};

#endif 

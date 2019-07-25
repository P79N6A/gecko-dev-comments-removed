





































#ifndef MAR_PRIVATE_H__
#define MAR_PRIVATE_H__

#include "prtypes.h"
#include "limits.h"



PR_STATIC_ASSERT(sizeof(PRUint32) == 4);
PR_STATIC_ASSERT(sizeof(PRUint64) == 8);

#define BLOCKSIZE 4096
#define ROUND_UP(n, incr) (((n) / (incr) + 1) * (incr))

#define MAR_ID "MAR1"
#define MAR_ID_SIZE 4



#define SIGNATURE_BLOCK_OFFSET 16



#define MAX_SIGNATURES 8



#define MAX_SIZE_OF_MAR_FILE ((PRInt64)524288000)



PR_STATIC_ASSERT(MAX_SIZE_OF_MAR_FILE < ((PRInt64)LONG_MAX));



PR_STATIC_ASSERT(sizeof(BLOCKSIZE) < \
  (SIGNATURE_BLOCK_OFFSET + sizeof(PRUint32)));



#define MAX_SIGNATURE_LENGTH 2048

#define MAR_ITEM_SIZE(namelen) (3*sizeof(PRUint32) + (namelen) + 1)




#ifdef XP_WIN
#include <winsock2.h>
#define ftello _ftelli64
#define fseeko _fseeki64
#else
#define _FILE_OFFSET_BITS 64
#include <netinet/in.h>
#include <unistd.h>
#endif

#include <stdio.h>

#define HOST_TO_NETWORK64(x) ( \
  ((((PRUint64) x) & 0xFF) << 56) | \
  ((((PRUint64) x) >> 8) & 0xFF) << 48) | \
  (((((PRUint64) x) >> 16) & 0xFF) << 40) | \
  (((((PRUint64) x) >> 24) & 0xFF) << 32) | \
  (((((PRUint64) x) >> 32) & 0xFF) << 24) | \
  (((((PRUint64) x) >> 40) & 0xFF) << 16) | \
  (((((PRUint64) x) >> 48) & 0xFF) << 8) | \
  (((PRUint64) x) >> 56)
#define NETWORK_TO_HOST64 HOST_TO_NETWORK64

#endif  

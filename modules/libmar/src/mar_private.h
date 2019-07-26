





#ifndef MAR_PRIVATE_H__
#define MAR_PRIVATE_H__

#include "limits.h"
#include "mozilla/Assertions.h"
#include "mozilla/StandardInteger.h"

#define BLOCKSIZE 4096
#define ROUND_UP(n, incr) (((n) / (incr) + 1) * (incr))

#define MAR_ID "MAR1"
#define MAR_ID_SIZE 4



#define SIGNATURE_BLOCK_OFFSET 16



#define MAX_SIZE_OF_MAR_FILE ((int64_t)524288000)



MOZ_STATIC_ASSERT(MAX_SIZE_OF_MAR_FILE < ((int64_t)LONG_MAX),
                  "max mar file size is too big");



MOZ_STATIC_ASSERT(sizeof(BLOCKSIZE) < \
                  (SIGNATURE_BLOCK_OFFSET + sizeof(uint32_t)),
                  "BLOCKSIZE is too big");



#define MAX_SIGNATURE_LENGTH 2048



#define PRODUCT_INFO_BLOCK_ID 1

#define MAR_ITEM_SIZE(namelen) (3*sizeof(uint32_t) + (namelen) + 1)


#define PIB_MAX_MAR_CHANNEL_ID_SIZE 63
#define PIB_MAX_PRODUCT_VERSION_SIZE 31




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
  ((((uint64_t) x) & 0xFF) << 56) | \
  ((((uint64_t) x) >> 8) & 0xFF) << 48) | \
  (((((uint64_t) x) >> 16) & 0xFF) << 40) | \
  (((((uint64_t) x) >> 24) & 0xFF) << 32) | \
  (((((uint64_t) x) >> 32) & 0xFF) << 24) | \
  (((((uint64_t) x) >> 40) & 0xFF) << 16) | \
  (((((uint64_t) x) >> 48) & 0xFF) << 8) | \
  (((uint64_t) x) >> 56)
#define NETWORK_TO_HOST64 HOST_TO_NETWORK64

#endif  

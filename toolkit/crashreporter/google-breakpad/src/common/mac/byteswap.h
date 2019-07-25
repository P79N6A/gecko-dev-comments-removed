


































#ifndef COMMON_MAC_BYTESWAP_H_
#define COMMON_MAC_BYTESWAP_H_

#include <libkern/OSByteOrder.h>

static inline uint16_t ByteSwap(uint16_t v) { return OSSwapInt16(v); }
static inline uint32_t ByteSwap(uint32_t v) { return OSSwapInt32(v); }
static inline uint64_t ByteSwap(uint64_t v) { return OSSwapInt64(v); }
static inline int16_t  ByteSwap(int16_t  v) { return OSSwapInt16(v); }
static inline int32_t  ByteSwap(int32_t  v) { return OSSwapInt32(v); }
static inline int64_t  ByteSwap(int64_t  v) { return OSSwapInt64(v); }

#endif  

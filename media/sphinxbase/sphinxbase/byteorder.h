



























































#ifndef __S2_BYTEORDER_H__
#define __S2_BYTEORDER_H__	1


#define SWAP_INT16(x)	*(x) = ((0x00ff & (*(x))>>8) | (0xff00 & (*(x))<<8))


#define SWAP_INT32(x)	*(x) = ((0x000000ff & (*(x))>>24) | \
				(0x0000ff00 & (*(x))>>8) | \
				(0x00ff0000 & (*(x))<<8) | \
				(0xff000000 & (*(x))<<24))


#define SWAP_FLOAT32(x)	SWAP_INT32((int32 *) x)


#define SWAP_FLOAT64(x)	{ int *low = (int *) (x), *high = (int *) (x) + 1,\
			      temp;\
			  SWAP_INT32(low);  SWAP_INT32(high);\
			  temp = *low; *low = *high; *high = temp;}

#ifdef WORDS_BIGENDIAN
#define SWAP_BE_64(x)
#define SWAP_BE_32(x)
#define SWAP_BE_16(x)
#define SWAP_LE_64(x) SWAP_FLOAT64(x)
#define SWAP_LE_32(x) SWAP_INT32(x)
#define SWAP_LE_16(x) SWAP_INT16(x)
#else
#define SWAP_LE_64(x)
#define SWAP_LE_32(x)
#define SWAP_LE_16(x)
#define SWAP_BE_64(x) SWAP_FLOAT64(x)
#define SWAP_BE_32(x) SWAP_INT32(x)
#define SWAP_BE_16(x) SWAP_INT16(x)
#endif

#endif

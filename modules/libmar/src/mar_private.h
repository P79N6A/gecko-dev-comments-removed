





































#ifndef MAR_PRIVATE_H__
#define MAR_PRIVATE_H__

#define BLOCKSIZE 4096
#define ROUND_UP(n, incr) (((n) / (incr) + 1) * (incr))

#define MAR_ID "MAR1"
#define MAR_ID_SIZE 4

#define MAR_ITEM_SIZE(namelen) (3*sizeof(PRUint32) + (namelen) + 1)

#endif  

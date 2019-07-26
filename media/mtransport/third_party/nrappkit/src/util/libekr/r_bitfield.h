














































#ifndef _r_bitfield_h
#define _r_bitfield_h

typedef struct r_bitfield_ {
     UINT4 *data;
     UINT4 len;
     UINT4 base;
} r_bitfield;

int r_bitfield_set(r_bitfield *,int bit);
int r_bitfield_isset(r_bitfield *,int bit);
int r_bitfield_create(r_bitfield **setp,UINT4 size);
int r_bitfield_destroy(r_bitfield **setp);

#endif


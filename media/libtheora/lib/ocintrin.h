

















#include <math.h>
#if !defined(_ocintrin_H)
# define _ocintrin_H (1)




























#define OC_MAXI(_a,_b)      ((_a)-((_a)-(_b)&-((_b)>(_a))))
#define OC_MINI(_a,_b)      ((_a)+((_b)-(_a)&-((_b)<(_a))))






#define OC_CLAMPI(_a,_b,_c) (OC_MAXI(_a,OC_MINI(_b,_c)))
#define OC_CLAMP255(_x)     ((unsigned char)((((_x)<0)-1)&((_x)|-((_x)>255))))




#define OC_SIGNI(_a)        (((_a)>0)-((_a)<0))



#define OC_SIGNMASK(_a)     (-((_a)<0))




#define OC_DIV_POW2(_dividend,_shift,_rmask)\
  ((_dividend)+(OC_SIGNMASK(_dividend)&(_rmask))>>(_shift))

#define OC_DIV2_16(_x) OC_DIV_POW2(_x,16,0xFFFF)

#define OC_DIV2(_x) OC_DIV_POW2(_x,1,0x1)

#define OC_DIV8(_x) OC_DIV_POW2(_x,3,0x7)

#define OC_DIV16(_x) OC_DIV_POW2(_x,4,0xF)




#define OC_DIV_ROUND_POW2(_dividend,_shift,_rval)\
  ((_dividend)+OC_SIGNMASK(_dividend)+(_rval)>>(_shift))

#define OC_DIV2_RE(_x) ((_x)+((_x)>>1&1)>>1)

#define OC_DIV_POW2_RE(_x,_shift) \
  ((_x)+((_x)>>(_shift)&1)+((1<<(_shift))-1>>1)>>(_shift))

#define OC_SORT2I(_a,_b) \
  do{ \
    int t__; \
    t__=((_a)^(_b))&-((_b)<(_a)); \
    (_a)^=t__; \
    (_b)^=t__; \
  } \
  while(0)



#define OC_BYTE_TABLE32(_a,_b,_c,_d,_i) \
  ((signed char) \
   (((_a)&0xFF|((_b)&0xFF)<<8|((_c)&0xFF)<<16|((_d)&0xFF)<<24)>>(_i)*8))


#define OC_UNIBBLE_TABLE32(_a,_b,_c,_d,_e,_f,_g,_h,_i) \
  ((((_a)&0xF|((_b)&0xF)<<4|((_c)&0xF)<<8|((_d)&0xF)<<12| \
   ((_e)&0xF)<<16|((_f)&0xF)<<20|((_g)&0xF)<<24|((_h)&0xF)<<28)>>(_i)*4)&0xF)




#define OC_MAXF(_a,_b)      ((_a)<(_b)?(_b):(_a))
#define OC_MINF(_a,_b)      ((_a)>(_b)?(_b):(_a))
#define OC_CLAMPF(_a,_b,_c) (OC_MINF(_a,OC_MAXF(_b,_c)))
#define OC_FABSF(_f)        ((float)fabs(_f))
#define OC_SQRTF(_f)        ((float)sqrt(_f))
#define OC_POWF(_b,_e)      ((float)pow(_b,_e))
#define OC_LOGF(_f)         ((float)log(_f))
#define OC_IFLOORF(_f)      ((int)floor(_f))
#define OC_ICEILF(_f)       ((int)ceil(_f))

#endif



















#include <math.h>
#if !defined(_ocintrin_H)
# define _ocintrin_H (1)














#define OC_MAXI(_a,_b)      ((_a)<(_b)?(_b):(_a))
#define OC_MINI(_a,_b)      ((_a)>(_b)?(_b):(_a))






#define OC_CLAMPI(_a,_b,_c) (OC_MAXI(_a,OC_MINI(_b,_c)))
#define OC_CLAMP255(_x)     ((unsigned char)((((_x)<0)-1)&((_x)|-((_x)>255))))




#define OC_DIV_POW2(_dividend,_shift,_rmask)\
  ((_dividend)+(((_dividend)>>sizeof(_dividend)*8-1)&(_rmask))>>(_shift))

#define OC_DIV2_16(_x) OC_DIV_POW2(_x,16,0xFFFF)

#define OC_DIV2(_x) OC_DIV_POW2(_x,1,0x1)

#define OC_DIV8(_x) OC_DIV_POW2(_x,3,0x7)

#define OC_DIV16(_x) OC_DIV_POW2(_x,4,0xF)




#define OC_DIV_ROUND_POW2(_dividend,_shift,_rval)\
  ((_dividend)+((_dividend)>>sizeof(_dividend)*8-1)+(_rval)>>(_shift))

#define OC_SORT2I(_a,_b)\
  if((_a)>(_b)){\
    int t__;\
    t__=(_a);\
    (_a)=(_b);\
    (_b)=t__;\
  }




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

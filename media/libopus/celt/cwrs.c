




































#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "os_support.h"
#include "cwrs.h"
#include "mathops.h"
#include "arch.h"

#ifdef CUSTOM_MODES





int log2_frac(opus_uint32 val, int frac)
{
  int l;
  l=EC_ILOG(val);
  if(val&(val-1)){
    

    if(l>16)val=(val>>(l-16))+(((val&((1<<(l-16))-1))+(1<<(l-16))-1)>>(l-16));
    else val<<=16-l;
    l=(l-1)<<frac;
    

    do{
      int b;
      b=(int)(val>>16);
      l+=b<<frac;
      val=(val+b)>>b;
      val=(val*val+0x7FFF)>>15;
    }
    while(frac-->0);
    
    return l+(val>0x8000);
  }
  
  else return (l-1)<<frac;
}
#endif

#ifndef SMALL_FOOTPRINT

#define MASK32 (0xFFFFFFFF)


static const opus_uint32 INV_TABLE[53]={
  0x00000001,0xAAAAAAAB,0xCCCCCCCD,0xB6DB6DB7,
  0x38E38E39,0xBA2E8BA3,0xC4EC4EC5,0xEEEEEEEF,
  0xF0F0F0F1,0x286BCA1B,0x3CF3CF3D,0xE9BD37A7,
  0xC28F5C29,0x684BDA13,0x4F72C235,0xBDEF7BDF,
  0x3E0F83E1,0x8AF8AF8B,0x914C1BAD,0x96F96F97,
  0xC18F9C19,0x2FA0BE83,0xA4FA4FA5,0x677D46CF,
  0x1A1F58D1,0xFAFAFAFB,0x8C13521D,0x586FB587,
  0xB823EE09,0xA08AD8F3,0xC10C9715,0xBEFBEFBF,
  0xC0FC0FC1,0x07A44C6B,0xA33F128D,0xE327A977,
  0xC7E3F1F9,0x962FC963,0x3F2B3885,0x613716AF,
  0x781948B1,0x2B2E43DB,0xFCFCFCFD,0x6FD0EB67,
  0xFA3F47E9,0xD2FD2FD3,0x3F4FD3F5,0xD4E25B9F,
  0x5F02A3A1,0xBF5A814B,0x7C32B16D,0xD3431B57,
  0xD8FD8FD9,
};





static inline opus_uint32 imusdiv32odd(opus_uint32 _a,opus_uint32 _b,
 opus_uint32 _c,int _d){
  celt_assert(_d<=52);
  return (_a*_b-_c)*INV_TABLE[_d]&MASK32;
}








static inline opus_uint32 imusdiv32even(opus_uint32 _a,opus_uint32 _b,
 opus_uint32 _c,int _d){
  opus_uint32 inv;
  int           mask;
  int           shift;
  int           one;
  celt_assert(_d>0);
  celt_assert(_d<=54);
  shift=EC_ILOG(_d^(_d-1));
  inv=INV_TABLE[(_d-1)>>shift];
  shift--;
  one=1<<shift;
  mask=one-1;
  return (_a*(_b>>shift)-(_c>>shift)+
   ((_a*(_b&mask)+one-(_c&mask))>>shift)-1)*inv&MASK32;
}

#endif 
























































































































#ifndef SMALL_FOOTPRINT


static inline unsigned ucwrs2(unsigned _k){
  celt_assert(_k>0);
  return _k+(_k-1);
}


static inline opus_uint32 ncwrs2(int _k){
  celt_assert(_k>0);
  return 4*(opus_uint32)_k;
}



static inline opus_uint32 ucwrs3(unsigned _k){
  celt_assert(_k>0);
  return (2*(opus_uint32)_k-2)*_k+1;
}


static inline opus_uint32 ncwrs3(int _k){
  celt_assert(_k>0);
  return 2*(2*(unsigned)_k*(opus_uint32)_k+1);
}


static inline opus_uint32 ucwrs4(int _k){
  celt_assert(_k>0);
  return imusdiv32odd(2*_k,(2*_k-3)*(opus_uint32)_k+4,3,1);
}


static inline opus_uint32 ncwrs4(int _k){
  celt_assert(_k>0);
  return ((_k*(opus_uint32)_k+2)*_k)/3<<3;
}

#endif 




static inline void unext(opus_uint32 *_ui,unsigned _len,opus_uint32 _ui0){
  opus_uint32 ui1;
  unsigned      j;
  

  j=1; do {
    ui1=UADD32(UADD32(_ui[j],_ui[j-1]),_ui0);
    _ui[j-1]=_ui0;
    _ui0=ui1;
  } while (++j<_len);
  _ui[j-1]=_ui0;
}




static inline void uprev(opus_uint32 *_ui,unsigned _n,opus_uint32 _ui0){
  opus_uint32 ui1;
  unsigned      j;
  

  j=1; do {
    ui1=USUB32(USUB32(_ui[j],_ui[j-1]),_ui0);
    _ui[j-1]=_ui0;
    _ui0=ui1;
  } while (++j<_n);
  _ui[j-1]=_ui0;
}



static opus_uint32 ncwrs_urow(unsigned _n,unsigned _k,opus_uint32 *_u){
  opus_uint32 um2;
  unsigned      len;
  unsigned      k;
  len=_k+2;
  
  celt_assert(len>=3);
  _u[0]=0;
  _u[1]=um2=1;
#ifndef SMALL_FOOTPRINT
  

  if(_n<=6)
#endif
 {
    
    
    celt_assert(_n>=2);
    
    celt_assert(_k>0);
    k=2;
    do _u[k]=(k<<1)-1;
    while(++k<len);
    for(k=2;k<_n;k++)unext(_u+1,_k+1,1);
  }
#ifndef SMALL_FOOTPRINT
  else{
    opus_uint32 um1;
    opus_uint32 n2m1;
    _u[2]=n2m1=um1=(_n<<1)-1;
    for(k=3;k<len;k++){
      
      _u[k]=um2=imusdiv32even(n2m1,um1,um2,k-1)+um2;
      if(++k>=len)break;
      _u[k]=um1=imusdiv32odd(n2m1,um2,um1,(k-1)>>1)+um1;
    }
  }
#endif 
  return _u[_k]+_u[_k+1];
}

#ifndef SMALL_FOOTPRINT




static inline void cwrsi1(int _k,opus_uint32 _i,int *_y){
  int s;
  s=-(int)_i;
  _y[0]=(_k+s)^s;
}




static inline void cwrsi2(int _k,opus_uint32 _i,int *_y){
  opus_uint32 p;
  int           s;
  int           yj;
  p=ucwrs2(_k+1U);
  s=-(_i>=p);
  _i-=p&s;
  yj=_k;
  _k=(_i+1)>>1;
  p=_k?ucwrs2(_k):0;
  _i-=p;
  yj-=_k;
  _y[0]=(yj+s)^s;
  cwrsi1(_k,_i,_y+1);
}




static void cwrsi3(int _k,opus_uint32 _i,int *_y){
  opus_uint32 p;
  int           s;
  int           yj;
  p=ucwrs3(_k+1U);
  s=-(_i>=p);
  _i-=p&s;
  yj=_k;
  

  _k=_i>0?(isqrt32(2*_i-1)+1)>>1:0;
  p=_k?ucwrs3(_k):0;
  _i-=p;
  yj-=_k;
  _y[0]=(yj+s)^s;
  cwrsi2(_k,_i,_y+1);
}




static void cwrsi4(int _k,opus_uint32 _i,int *_y){
  opus_uint32 p;
  int           s;
  int           yj;
  int           kl;
  int           kr;
  p=ucwrs4(_k+1);
  s=-(_i>=p);
  _i-=p&s;
  yj=_k;
  


  kl=0;
  kr=_k;
  for(;;){
    _k=(kl+kr)>>1;
    p=_k?ucwrs4(_k):0;
    if(p<_i){
      if(_k>=kr)break;
      kl=_k+1;
    }
    else if(p>_i)kr=_k-1;
    else break;
  }
  _i-=p;
  yj-=_k;
  _y[0]=(yj+s)^s;
  cwrsi3(_k,_i,_y+1);
}

#endif 






static void cwrsi(int _n,int _k,opus_uint32 _i,int *_y,opus_uint32 *_u){
  int j;
  celt_assert(_n>0);
  j=0;
  do{
    opus_uint32 p;
    int           s;
    int           yj;
    p=_u[_k+1];
    s=-(_i>=p);
    _i-=p&s;
    yj=_k;
    p=_u[_k];
    while(p>_i)p=_u[--_k];
    _i-=p;
    yj-=_k;
    _y[j]=(yj+s)^s;
    uprev(_u,_k+2,0);
  }
  while(++j<_n);
}





static inline opus_uint32 icwrs1(const int *_y,int *_k){
  *_k=abs(_y[0]);
  return _y[0]<0;
}

#ifndef SMALL_FOOTPRINT





static inline opus_uint32 icwrs2(const int *_y,int *_k){
  opus_uint32 i;
  int           k;
  i=icwrs1(_y+1,&k);
  i+=k?ucwrs2(k):0;
  k+=abs(_y[0]);
  if(_y[0]<0)i+=ucwrs2(k+1U);
  *_k=k;
  return i;
}





static inline opus_uint32 icwrs3(const int *_y,int *_k){
  opus_uint32 i;
  int           k;
  i=icwrs2(_y+1,&k);
  i+=k?ucwrs3(k):0;
  k+=abs(_y[0]);
  if(_y[0]<0)i+=ucwrs3(k+1U);
  *_k=k;
  return i;
}





static inline opus_uint32 icwrs4(const int *_y,int *_k){
  opus_uint32 i;
  int           k;
  i=icwrs3(_y+1,&k);
  i+=k?ucwrs4(k):0;
  k+=abs(_y[0]);
  if(_y[0]<0)i+=ucwrs4(k+1);
  *_k=k;
  return i;
}

#endif 





static inline opus_uint32 icwrs(int _n,int _k,opus_uint32 *_nc,const int *_y,
 opus_uint32 *_u){
  opus_uint32 i;
  int           j;
  int           k;
  
  celt_assert(_n>=2);
  _u[0]=0;
  for(k=1;k<=_k+1;k++)_u[k]=(k<<1)-1;
  i=icwrs1(_y+_n-1,&k);
  j=_n-2;
  i+=_u[k];
  k+=abs(_y[j]);
  if(_y[j]<0)i+=_u[k+1];
  while(j-->0){
    unext(_u,_k+2,0);
    i+=_u[k];
    k+=abs(_y[j]);
    if(_y[j]<0)i+=_u[k+1];
  }
  *_nc=_u[k]+_u[k+1];
  return i;
}

#ifdef CUSTOM_MODES
void get_required_bits(opus_int16 *_bits,int _n,int _maxk,int _frac){
  int k;
  
  celt_assert(_maxk>0);
  _bits[0]=0;
  if (_n==1)
  {
    for (k=1;k<=_maxk;k++)
      _bits[k] = 1<<_frac;
  }
  else {
    VARDECL(opus_uint32,u);
    SAVE_STACK;
    ALLOC(u,_maxk+2U,opus_uint32);
    ncwrs_urow(_n,_maxk,u);
    for(k=1;k<=_maxk;k++)
      _bits[k]=log2_frac(u[k]+u[k+1],_frac);
    RESTORE_STACK;
  }
}
#endif 

void encode_pulses(const int *_y,int _n,int _k,ec_enc *_enc){
  opus_uint32 i;
  celt_assert(_k>0);
#ifndef SMALL_FOOTPRINT
  switch(_n){
    case 2:{
      i=icwrs2(_y,&_k);
      ec_enc_uint(_enc,i,ncwrs2(_k));
    }break;
    case 3:{
      i=icwrs3(_y,&_k);
      ec_enc_uint(_enc,i,ncwrs3(_k));
    }break;
    case 4:{
      i=icwrs4(_y,&_k);
      ec_enc_uint(_enc,i,ncwrs4(_k));
    }break;
     default:
    {
#endif
      VARDECL(opus_uint32,u);
      opus_uint32 nc;
      SAVE_STACK;
      ALLOC(u,_k+2U,opus_uint32);
      i=icwrs(_n,_k,&nc,_y,u);
      ec_enc_uint(_enc,i,nc);
      RESTORE_STACK;
#ifndef SMALL_FOOTPRINT
    }
    break;
  }
#endif
}

void decode_pulses(int *_y,int _n,int _k,ec_dec *_dec)
{
  celt_assert(_k>0);
#ifndef SMALL_FOOTPRINT
   switch(_n){
    case 2:cwrsi2(_k,ec_dec_uint(_dec,ncwrs2(_k)),_y);break;
    case 3:cwrsi3(_k,ec_dec_uint(_dec,ncwrs3(_k)),_y);break;
    case 4:cwrsi4(_k,ec_dec_uint(_dec,ncwrs4(_k)),_y);break;
    default:
    {
#endif
      VARDECL(opus_uint32,u);
      SAVE_STACK;
      ALLOC(u,_k+2U,opus_uint32);
      cwrsi(_n,_k,ec_dec_uint(_dec,ncwrs_urow(_n,_k,u)),_y,u);
      RESTORE_STACK;
#ifndef SMALL_FOOTPRINT
    }
    break;
  }
#endif
}

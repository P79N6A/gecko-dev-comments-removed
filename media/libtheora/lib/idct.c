
















#include <string.h>
#include "internal.h"
#include "dct.h"









static void idct8(ogg_int16_t *_y,const ogg_int16_t _x[8]){
  ogg_int32_t t[8];
  ogg_int32_t r;
  
  
  t[0]=OC_C4S4*(ogg_int16_t)(_x[0]+_x[4])>>16;
  t[1]=OC_C4S4*(ogg_int16_t)(_x[0]-_x[4])>>16;
  
  t[2]=(OC_C6S2*_x[2]>>16)-(OC_C2S6*_x[6]>>16);
  t[3]=(OC_C2S6*_x[2]>>16)+(OC_C6S2*_x[6]>>16);
  
  t[4]=(OC_C7S1*_x[1]>>16)-(OC_C1S7*_x[7]>>16);
  
  t[5]=(OC_C3S5*_x[5]>>16)-(OC_C5S3*_x[3]>>16);
  t[6]=(OC_C5S3*_x[5]>>16)+(OC_C3S5*_x[3]>>16);
  t[7]=(OC_C1S7*_x[1]>>16)+(OC_C7S1*_x[7]>>16);
  
  
  r=t[4]+t[5];
  t[5]=OC_C4S4*(ogg_int16_t)(t[4]-t[5])>>16;
  t[4]=r;
  
  r=t[7]+t[6];
  t[6]=OC_C4S4*(ogg_int16_t)(t[7]-t[6])>>16;
  t[7]=r;
  
  
  r=t[0]+t[3];
  t[3]=t[0]-t[3];
  t[0]=r;
  
  r=t[1]+t[2];
  t[2]=t[1]-t[2];
  t[1]=r;
  
  r=t[6]+t[5];
  t[5]=t[6]-t[5];
  t[6]=r;
  
  
  _y[0<<3]=(ogg_int16_t)(t[0]+t[7]);
  
  _y[1<<3]=(ogg_int16_t)(t[1]+t[6]);
  
  _y[2<<3]=(ogg_int16_t)(t[2]+t[5]);
  
  _y[3<<3]=(ogg_int16_t)(t[3]+t[4]);
  _y[4<<3]=(ogg_int16_t)(t[3]-t[4]);
  _y[5<<3]=(ogg_int16_t)(t[2]-t[5]);
  _y[6<<3]=(ogg_int16_t)(t[1]-t[6]);
  _y[7<<3]=(ogg_int16_t)(t[0]-t[7]);
}










static void idct8_4(ogg_int16_t *_y,const ogg_int16_t _x[8]){
  ogg_int32_t t[8];
  ogg_int32_t r;
  
  t[0]=OC_C4S4*_x[0]>>16;
  t[2]=OC_C6S2*_x[2]>>16;
  t[3]=OC_C2S6*_x[2]>>16;
  t[4]=OC_C7S1*_x[1]>>16;
  t[5]=-(OC_C5S3*_x[3]>>16);
  t[6]=OC_C3S5*_x[3]>>16;
  t[7]=OC_C1S7*_x[1]>>16;
  
  r=t[4]+t[5];
  t[5]=OC_C4S4*(ogg_int16_t)(t[4]-t[5])>>16;
  t[4]=r;
  r=t[7]+t[6];
  t[6]=OC_C4S4*(ogg_int16_t)(t[7]-t[6])>>16;
  t[7]=r;
  
  t[1]=t[0]+t[2];
  t[2]=t[0]-t[2];
  r=t[0]+t[3];
  t[3]=t[0]-t[3];
  t[0]=r;
  r=t[6]+t[5];
  t[5]=t[6]-t[5];
  t[6]=r;
  
  _y[0<<3]=(ogg_int16_t)(t[0]+t[7]);
  _y[1<<3]=(ogg_int16_t)(t[1]+t[6]);
  _y[2<<3]=(ogg_int16_t)(t[2]+t[5]);
  _y[3<<3]=(ogg_int16_t)(t[3]+t[4]);
  _y[4<<3]=(ogg_int16_t)(t[3]-t[4]);
  _y[5<<3]=(ogg_int16_t)(t[2]-t[5]);
  _y[6<<3]=(ogg_int16_t)(t[1]-t[6]);
  _y[7<<3]=(ogg_int16_t)(t[0]-t[7]);
}










static void idct8_3(ogg_int16_t *_y,const ogg_int16_t _x[8]){
  ogg_int32_t t[8];
  ogg_int32_t r;
  
  t[0]=OC_C4S4*_x[0]>>16;
  t[2]=OC_C6S2*_x[2]>>16;
  t[3]=OC_C2S6*_x[2]>>16;
  t[4]=OC_C7S1*_x[1]>>16;
  t[7]=OC_C1S7*_x[1]>>16;
  
  t[5]=OC_C4S4*t[4]>>16;
  t[6]=OC_C4S4*t[7]>>16;
  
  t[1]=t[0]+t[2];
  t[2]=t[0]-t[2];
  r=t[0]+t[3];
  t[3]=t[0]-t[3];
  t[0]=r;
  r=t[6]+t[5];
  t[5]=t[6]-t[5];
  t[6]=r;
  
  _y[0<<3]=(ogg_int16_t)(t[0]+t[7]);
  _y[1<<3]=(ogg_int16_t)(t[1]+t[6]);
  _y[2<<3]=(ogg_int16_t)(t[2]+t[5]);
  _y[3<<3]=(ogg_int16_t)(t[3]+t[4]);
  _y[4<<3]=(ogg_int16_t)(t[3]-t[4]);
  _y[5<<3]=(ogg_int16_t)(t[2]-t[5]);
  _y[6<<3]=(ogg_int16_t)(t[1]-t[6]);
  _y[7<<3]=(ogg_int16_t)(t[0]-t[7]);
}










static void idct8_2(ogg_int16_t *_y,const ogg_int16_t _x[8]){
  ogg_int32_t t[8];
  ogg_int32_t r;
  
  t[0]=OC_C4S4*_x[0]>>16;
  t[4]=OC_C7S1*_x[1]>>16;
  t[7]=OC_C1S7*_x[1]>>16;
  
  t[5]=OC_C4S4*t[4]>>16;
  t[6]=OC_C4S4*t[7]>>16;
  
  r=t[6]+t[5];
  t[5]=t[6]-t[5];
  t[6]=r;
  
  _y[0<<3]=(ogg_int16_t)(t[0]+t[7]);
  _y[1<<3]=(ogg_int16_t)(t[0]+t[6]);
  _y[2<<3]=(ogg_int16_t)(t[0]+t[5]);
  _y[3<<3]=(ogg_int16_t)(t[0]+t[4]);
  _y[4<<3]=(ogg_int16_t)(t[0]-t[4]);
  _y[5<<3]=(ogg_int16_t)(t[0]-t[5]);
  _y[6<<3]=(ogg_int16_t)(t[0]-t[6]);
  _y[7<<3]=(ogg_int16_t)(t[0]-t[7]);
}










static void idct8_1(ogg_int16_t *_y,const ogg_int16_t _x[1]){
  _y[0<<3]=_y[1<<3]=_y[2<<3]=_y[3<<3]=
   _y[4<<3]=_y[5<<3]=_y[6<<3]=_y[7<<3]=(ogg_int16_t)(OC_C4S4*_x[0]>>16);
}
















static void oc_idct8x8_3(ogg_int16_t _y[64],ogg_int16_t _x[64]){
  ogg_int16_t w[64];
  int         i;
  
  idct8_2(w,_x);
  idct8_1(w+1,_x+8);
  
  for(i=0;i<8;i++)idct8_2(_y+i,w+i*8);
  
  for(i=0;i<64;i++)_y[i]=(ogg_int16_t)(_y[i]+8>>4);
  
  if(_x!=_y)_x[0]=_x[1]=_x[8]=0;
}
















static void oc_idct8x8_10(ogg_int16_t _y[64],ogg_int16_t _x[64]){
  ogg_int16_t w[64];
  int         i;
  
  idct8_4(w,_x);
  idct8_3(w+1,_x+8);
  idct8_2(w+2,_x+16);
  idct8_1(w+3,_x+24);
  
  for(i=0;i<8;i++)idct8_4(_y+i,w+i*8);
  
  for(i=0;i<64;i++)_y[i]=(ogg_int16_t)(_y[i]+8>>4);
  
  if(_x!=_y)_x[0]=_x[1]=_x[2]=_x[3]=_x[8]=_x[9]=_x[10]=_x[16]=_x[17]=_x[24]=0;
}







static void oc_idct8x8_slow(ogg_int16_t _y[64],ogg_int16_t _x[64]){
  ogg_int16_t w[64];
  int         i;
  
  for(i=0;i<8;i++)idct8(w+i,_x+i*8);
  
  for(i=0;i<8;i++)idct8(_y+i,w+i*8);
  
  for(i=0;i<64;i++)_y[i]=(ogg_int16_t)(_y[i]+8>>4);
  if(_x!=_y)for(i=0;i<64;i++)_x[i]=0;
}




void oc_idct8x8_c(ogg_int16_t _y[64],ogg_int16_t _x[64],int _last_zzi){
  























  
  if(_last_zzi<=3)oc_idct8x8_3(_y,_x);
  else if(_last_zzi<=10)oc_idct8x8_10(_y,_x);
  else oc_idct8x8_slow(_y,_x);
}

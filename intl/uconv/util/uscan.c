



#include "unicpriv.h"
#define CHK_GR94(b) ( (uint8_t) 0xa0 < (uint8_t) (b) && (uint8_t) (b) < (uint8_t) 0xff )
#define CHK_GR94_2Byte(b1,b2) (CHK_GR94(b1) && CHK_GR94(b2))



typedef  int (*uSubScannerFunc) (unsigned char* in, uint16_t* out);




typedef int (*uScannerFunc) (
                             int32_t*    state,
                             unsigned char  *in,
                             uint16_t    *out,
                             uint32_t     inbuflen,
                             uint32_t*    inscanlen
                             );

int uScan(
          uScanClassID scanClass,
          int32_t*    state,
          unsigned char  *in,
          uint16_t    *out,
          uint32_t     inbuflen,
          uint32_t*    inscanlen
          );

#define uSubScanner(sub,in,out) (* m_subscanner[sub])((in),(out))

int uCheckAndScanAlways1Byte(
                            int32_t*    state,
                            unsigned char  *in,
                            uint16_t    *out,
                            uint32_t     inbuflen,
                            uint32_t*    inscanlen
                            );
int uCheckAndScanAlways2Byte(
                             int32_t*    state,
                             unsigned char  *in,
                             uint16_t    *out,
                             uint32_t     inbuflen,
                             uint32_t*    inscanlen
                             );
int uCheckAndScanAlways2ByteShiftGR(
                                    int32_t*    state,
                                    unsigned char  *in,
                                    uint16_t    *out,
                                    uint32_t     inbuflen,
                                    uint32_t*    inscanlen
                                    );
int uCheckAndScanAlways2ByteGR128(
                                  int32_t*    state,
                                  unsigned char  *in,
                                  uint16_t    *out,
                                  uint32_t     inbuflen,
                                  uint32_t*    inscanlen
                                          );
int uScanShift(
               uShiftInTable    *shift,
               int32_t*    state,
               unsigned char  *in,
               uint16_t    *out,
               uint32_t     inbuflen,
               uint32_t*    inscanlen
               );

int uCheckAndScan2ByteGRPrefix8F(
                                 int32_t*    state,
                                 unsigned char  *in,
                                 uint16_t    *out,
                                 uint32_t     inbuflen,
                                 uint32_t*    inscanlen
                                 );
int uCheckAndScan2ByteGRPrefix8EA2(
                                   int32_t*    state,
                                   unsigned char  *in,
                                   uint16_t    *out,
                                   uint32_t     inbuflen,
                                   uint32_t*    inscanlen
                                   );
int uCheckAndScan2ByteGRPrefix8EA3(
                                   int32_t*    state,
                                   unsigned char  *in,
                                   uint16_t    *out,
                                   uint32_t     inbuflen,
                                   uint32_t*    inscanlen
                                   );
int uCheckAndScan2ByteGRPrefix8EA4(
                                   int32_t*    state,
                                   unsigned char  *in,
                                   uint16_t    *out,
                                   uint32_t     inbuflen,
                                   uint32_t*    inscanlen
                                   );
int uCheckAndScan2ByteGRPrefix8EA5(
                                   int32_t*    state,
                                   unsigned char  *in,
                                   uint16_t    *out,
                                   uint32_t     inbuflen,
                                   uint32_t*    inscanlen
                                   );
int uCheckAndScan2ByteGRPrefix8EA6(
                                   int32_t*    state,
                                   unsigned char  *in,
                                   uint16_t    *out,
                                   uint32_t     inbuflen,
                                   uint32_t*    inscanlen
                                   );
int uCheckAndScan2ByteGRPrefix8EA7(
                                   int32_t*    state,
                                   unsigned char  *in,
                                   uint16_t    *out,
                                   uint32_t     inbuflen,
                                   uint32_t*    inscanlen
                                   );
int uCnSAlways8BytesDecomposedHangul(
                                     int32_t*    state,
                                     unsigned char  *in,
                                     uint16_t    *out,
                                     uint32_t     inbuflen,
                                     uint32_t*    inscanlen
                                     );
int uCheckAndScanJohabHangul(
                             int32_t*    state,
                             unsigned char  *in,
                             uint16_t    *out,
                             uint32_t     inbuflen,
                             uint32_t*    inscanlen
                             );
int uCheckAndScanJohabSymbol(
                             int32_t*    state,
                             unsigned char  *in,
                             uint16_t    *out,
                             uint32_t     inbuflen,
                             uint32_t*    inscanlen
                             );

int uCheckAndScan4BytesGB18030(
                               int32_t*    state,
                               unsigned char  *in,
                               uint16_t    *out,
                               uint32_t     inbuflen,
                               uint32_t*    inscanlen
                               );

int uScanAlways2Byte(
                     unsigned char*  in,
                     uint16_t*    out
                     );
int uScanAlways2ByteShiftGR(
                            unsigned char*  in,
                            uint16_t*    out
                            );
int uScanAlways1Byte(
                     unsigned char*  in,
                     uint16_t*    out
                     );
int uScanAlways1BytePrefix8E(
                             unsigned char*  in,
                             uint16_t*    out
                             );



const uScannerFunc m_scanner[uNumOfCharsetType] =
{
    uCheckAndScanAlways1Byte,
    uCheckAndScanAlways2Byte,
    uCheckAndScanAlways2ByteShiftGR,
    uCheckAndScan2ByteGRPrefix8F,
    uCheckAndScan2ByteGRPrefix8EA2,
    uCheckAndScan2ByteGRPrefix8EA3,
    uCheckAndScan2ByteGRPrefix8EA4,
    uCheckAndScan2ByteGRPrefix8EA5,
    uCheckAndScan2ByteGRPrefix8EA6,
    uCheckAndScan2ByteGRPrefix8EA7,
    uCnSAlways8BytesDecomposedHangul,
    uCheckAndScanJohabHangul,
    uCheckAndScanJohabSymbol,
    uCheckAndScan4BytesGB18030,
    uCheckAndScanAlways2ByteGR128
};





const uSubScannerFunc m_subscanner[uNumOfCharType] =
{
    uScanAlways1Byte,
    uScanAlways2Byte,
    uScanAlways2ByteShiftGR,
    uScanAlways1BytePrefix8E
};



int uScan(
          uScanClassID scanClass,
          int32_t*    state,
          unsigned char  *in,
          uint16_t    *out,
          uint32_t     inbuflen,
          uint32_t*    inscanlen
          )
{
  return (* m_scanner[scanClass]) (state,in,out,inbuflen,inscanlen);
}



int uScanAlways1Byte(
                     unsigned char*  in,
                     uint16_t*    out
                     )
{
  *out = (uint16_t) in[0];
  return 1;
}




int uScanAlways2Byte(
                     unsigned char*  in,
                     uint16_t*    out
                     )
{
  *out = (uint16_t) (( in[0] << 8) | (in[1]));
  return 1;
}



int uScanAlways2ByteShiftGR(
                            unsigned char*  in,
                            uint16_t*    out
                            )
{
  *out = (uint16_t) ((( in[0] << 8) | (in[1])) &  0x7F7F);
  return 1;
}




int uScanAlways1BytePrefix8E(
                             unsigned char*  in,
                             uint16_t*    out
                             )
{
  *out = (uint16_t) in[1];
  return 1;
}



int uCheckAndScanAlways1Byte(
                             int32_t*    state,
                             unsigned char  *in,
                             uint16_t    *out,
                             uint32_t     inbuflen,
                             uint32_t*    inscanlen
                             )
{
  
  *inscanlen = 1;
  *out = (uint16_t) in[0];
  
  return 1;
}




int uCheckAndScanAlways2Byte(
                             int32_t*    state,
                             unsigned char  *in,
                             uint16_t    *out,
                             uint32_t     inbuflen,
                             uint32_t*    inscanlen
                             )
{
  if(inbuflen < 2)
    return 0;
  else
  {
    *inscanlen = 2;
    *out = ((in[0] << 8) | ( in[1])) ;
    return 1;
  }
}



int uCheckAndScanAlways2ByteShiftGR(
                                    int32_t*    state,
                                    unsigned char  *in,
                                    uint16_t    *out,
                                    uint32_t     inbuflen,
                                    uint32_t*    inscanlen
                                    )
{
  





  if(inbuflen < 2)    
    return 0;
  else if (! CHK_GR94(in[1]))  
  {
    *inscanlen = 2; 
    *out = 0xFF;  
    return 1;
  }
  else
  {
    *inscanlen = 2;
    *out = (((in[0] << 8) | ( in[1]))  & 0x7F7F);
    return 1;
  }
}



int uCheckAndScanAlways2ByteGR128(
                                  int32_t*    state,
                                  unsigned char  *in,
                                  uint16_t    *out,
                                  uint32_t     inbuflen,
                                  uint32_t*    inscanlen
                                  )
{
  






  if(inbuflen < 2)    
    return 0;
  else if (in[1] < 0x41)     
  {
    *inscanlen = 2; 
    *out = 0xFF;  
    return 1;
  }
  else
  {
    *inscanlen = 2;
    *out = (in[0] << 8) |  in[1];
    return 1;
  }
}



int uScanShift(
               uShiftInTable    *shift,
               int32_t*    state,
               unsigned char  *in,
               uint16_t    *out,
               uint32_t     inbuflen,
               uint32_t*    inscanlen
               )
{
  int16_t i;
  const uShiftInCell* cell = &(shift->shiftcell[0]);
  int16_t itemnum = shift->numOfItem;
  for(i=0;i<itemnum;i++)
  {
    if( ( in[0] >=  cell[i].shiftin_Min) &&
      ( in[0] <=  cell[i].shiftin_Max))
    {
      if(inbuflen < cell[i].reserveLen)
        return 0;
      else
      {
        *inscanlen = cell[i].reserveLen;
        return (uSubScanner(cell[i].classID,in,out));
      }
    }
  }
  return 0;
}



int uCheckAndScan2ByteGRPrefix8F(
                                 int32_t*    state,
                                 unsigned char  *in,
                                 uint16_t    *out,
                                 uint32_t     inbuflen,
                                 uint32_t*    inscanlen
                                 )
{
  if((inbuflen < 3) ||(in[0] != 0x8F)) 
    return 0;
  else if (! CHK_GR94(in[1]))  
  {
    *inscanlen = 2; 
    *out = 0xFF;  
    return 1;
  }
  else if (! CHK_GR94(in[2]))  
  {
    *inscanlen = 3; 
    *out = 0xFF;  
    return 1;
  }
  else
  {
    *inscanlen = 3;
    *out = (((in[1] << 8) | ( in[2]))  & 0x7F7F);
    return 1;
  }
}







#define CNS_8EAX_4BYTE(PREFIX)                    \
  if((inbuflen < 4) || (in[0] != 0x8E))           \
    return 0;                                     \
  else if((in[1] != (PREFIX)))                    \
  {                                               \
    *inscanlen = 2;                               \
    *out = 0xFF;                                  \
    return 1;                                     \
  }                                               \
  else if(! CHK_GR94(in[2]))                      \
  {                                               \
    *inscanlen = 3;                               \
    *out = 0xFF;                                  \
    return 1;                                     \
  }                                               \
  else if(! CHK_GR94(in[3]))                      \
  {                                               \
    *inscanlen = 4;                               \
    *out = 0xFF;                                  \
    return 1;                                     \
  }                                               \
  else                                            \
  {                                               \
    *inscanlen = 4;                               \
    *out = (((in[2] << 8) | ( in[3]))  & 0x7F7F); \
    return 1;                                     \
  }    

int uCheckAndScan2ByteGRPrefix8EA2(
                                   int32_t*    state,
                                   unsigned char  *in,
                                   uint16_t    *out,
                                   uint32_t     inbuflen,
                                   uint32_t*    inscanlen
                                   )
{
  CNS_8EAX_4BYTE(0xA2)
}




int uCheckAndScan2ByteGRPrefix8EA3(
                                   int32_t*    state,
                                   unsigned char  *in,
                                   uint16_t    *out,
                                   uint32_t     inbuflen,
                                   uint32_t*    inscanlen
                                   )
{
  CNS_8EAX_4BYTE(0xA3)
}



int uCheckAndScan2ByteGRPrefix8EA4(
                                   int32_t*    state,
                                   unsigned char  *in,
                                   uint16_t    *out,
                                   uint32_t     inbuflen,
                                   uint32_t*    inscanlen
                                   )
{
  CNS_8EAX_4BYTE(0xA4)
}



int uCheckAndScan2ByteGRPrefix8EA5(
                                   int32_t*    state,
                                   unsigned char  *in,
                                   uint16_t    *out,
                                   uint32_t     inbuflen,
                                   uint32_t*    inscanlen
                                   )
{
  CNS_8EAX_4BYTE(0xA5)
}



int uCheckAndScan2ByteGRPrefix8EA6(
                                   int32_t*    state,
                                   unsigned char  *in,
                                   uint16_t    *out,
                                   uint32_t     inbuflen,
                                   uint32_t*    inscanlen
                                   )
{
  CNS_8EAX_4BYTE(0xA6)
}



int uCheckAndScan2ByteGRPrefix8EA7(
                                   int32_t*    state,
                                   unsigned char  *in,
                                   uint16_t    *out,
                                   uint32_t     inbuflen,
                                   uint32_t*    inscanlen
                                   )
{
  CNS_8EAX_4BYTE(0xA7)
}



#define SBase 0xAC00
#define SCount 11172
#define LCount 19
#define VCount 21
#define TCount 28
#define NCount (VCount * TCount)

int uCnSAlways8BytesDecomposedHangul(
                                     int32_t*    state,
                                     unsigned char  *in,
                                     uint16_t    *out,
                                     uint32_t     inbuflen,
                                     uint32_t*    inscanlen
                                     )
{
  
  uint16_t LIndex, VIndex, TIndex;
  
  if((inbuflen < 8) || (0xa4 != in[0]) || (0xd4 != in[1]) ||
    (0xa4 != in[2] ) || (0xa4 != in[4]) || (0xa4 != in[6]))
    return 0;
  
  
  if((in[3] < 0xa1) || (in[3] > 0xbe)) { 
    return 0;
  } 
  else {
    static const uint8_t lMap[] = {
      
      0,   1,0xff,   2,0xff,0xff,   3,
        
        4,   5,0xff,0xff,0xff,0xff,0xff,0xff,
        
        0xff,   6,   7,   8,0xff,   9,  10,  11,
        
        12,  13,  14,  15,  16,  17,  18     
    };
    
    LIndex = lMap[in[3] - 0xa1];
    if(0xff == (0xff & LIndex))
      return 0;
  }
  
  
  if((in[5] < 0xbf) || (in[5] > 0xd3)) { 
    return 0;
  } 
  else {
    VIndex = in[5] - 0xbf;
  }
  
  
  if(0xd4 == in[7])  
  {
    TIndex = 0;
  } 
  else if((in[7] < 0xa1) || (in[7] > 0xbe)) {
    return 0;
  } 
  else {
    static const uint8_t tMap[] = {
      
      1,   2,   3,   4,   5,   6,   7,
        
        0xff,   8,   9,  10,  11,  12,  13,  14,
        
        15,  16,  17,0xff,  18,  19,  20,  21,
        
        22,0xff,  23,  24,  25,  26,  27     
    };
    TIndex = tMap[in[7] - 0xa1];
    if(0xff == (0xff & TIndex))
      return 0;
  }
  
  *inscanlen = 8;
  
  *out = ( LIndex * VCount + VIndex) * TCount + TIndex + SBase;
  
  return 1;
}




int uCheckAndScanJohabHangul(
                             int32_t*    state,
                             unsigned char  *in,
                             uint16_t    *out,
                             uint32_t     inbuflen,
                             uint32_t*    inscanlen
                             )
{


  if(inbuflen < 2)
    return 0;
  else {
  



    static const uint8_t lMap[32]={ 
      0xff,0xff,0,   1,   2,   3,   4,   5,    
        6,   7,   8,   9,   10,  11,  12,  13,   
        14,  15,  16,  17,  18,  0xff,0xff,0xff, 
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff  
    };
    static const uint8_t vMap[32]={ 
      0xff,0xff,0xff,0,   1,   2,   3,   4,    
        0xff,0xff,5,   6,   7,   8,   9,   10,   
        0xff,0xff,11,  12,  13,  14,  15,  16,   
        0xff,0xff,17,  18,  19,  20,  0xff,0xff  
    };
    static const uint8_t tMap[32]={ 
      0xff,0,   1,   2,   3,   4,   5,   6,    
        7,   8,   9,   10,  11,  12,  13,  14,   
        15,  16,  0xff,17,  18,  19,  20,  21,   
        22,  23,  24,  25,  26,  27,  0xff,0xff  
    };
    uint16_t ch = (in[0] << 8) | in[1];
    uint16_t LIndex, VIndex, TIndex;
    if(0 == (0x8000 & ch))
      return 0;
    LIndex=lMap[(ch>>10)& 0x1F];
    VIndex=vMap[(ch>>5) & 0x1F];
    TIndex=tMap[(ch>>0) & 0x1F];
    if((0xff==(LIndex)) || 
      (0xff==(VIndex)) || 
      (0xff==(TIndex)))
      return 0;
    
    *out = ( LIndex * VCount + VIndex) * TCount + TIndex + SBase;
    *inscanlen = 2;
    return 1;
  }
}
int uCheckAndScanJohabSymbol(
                             int32_t*    state,
                             unsigned char  *in,
                             uint16_t    *out,
                             uint32_t     inbuflen,
                             uint32_t*    inscanlen
                             )
{
  if(inbuflen < 2)
    return 0;
  else {
  























 
    unsigned char hi = in[0];
    unsigned char lo = in[1];
    uint16_t offset = (( hi > 223 ) && ( hi < 250)) ? 1 : 0;
    uint16_t d8_off = 0;
    if(216 == hi) {
      if( lo > 160)
        d8_off = 94;
      else
        d8_off = 42;
    }
    
    *out = (((((hi - ((hi < 223) ? 200 : 187)) << 1) -
      (lo < 161 ? 1 : 0) + offset) + d8_off) << 8 ) |
      (lo - ((lo < 161) ? ((lo > 126) ? 34 : 16) : 
    128));
    *inscanlen = 2;
    return 1;
  }
}
int uCheckAndScan4BytesGB18030(
                               int32_t*    state,
                               unsigned char  *in,
                               uint16_t    *out,
                               uint32_t     inbuflen,
                               uint32_t*    inscanlen
                               )
{
  uint32_t  data;
  if(inbuflen < 4) 
    return 0;
  
  if((in[0] < 0x81 ) || (0xfe < in[0])) 
    return 0;
  if((in[1] < 0x30 ) || (0x39 < in[1])) 
    return 0;
  if((in[2] < 0x81 ) || (0xfe < in[2])) 
    return 0;
  if((in[3] < 0x30 ) || (0x39 < in[3])) 
    return 0;
  
  data = (((((in[0] - 0x81) * 10 + (in[1] - 0x30)) * 126) + 
    (in[2] - 0x81)) * 10 ) + (in[3] - 0x30);
  
  *inscanlen = 4;
  *out = (data < 0x00010000) ? data : 0xFFFD;
  return 1;
}

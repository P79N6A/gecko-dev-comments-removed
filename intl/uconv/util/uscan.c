




































#include "unicpriv.h"
#define CHK_GR94(b) ( (PRUint8) 0xa0 < (PRUint8) (b) && (PRUint8) (b) < (PRUint8) 0xff )
#define CHK_GR94_2Byte(b1,b2) (CHK_GR94(b1) && CHK_GR94(b2))



typedef  PRBool (*uSubScannerFunc) (unsigned char* in, PRUint16* out);




typedef PRBool (*uScannerFunc) (
                                PRInt32*    state,
                                unsigned char  *in,
                                PRUint16    *out,
                                PRUint32     inbuflen,
                                PRUint32*    inscanlen
                                );

MODULE_PRIVATE PRBool uScan(  
                            uScanClassID scanClass,
                            PRInt32*    state,
                            unsigned char  *in,
                            PRUint16    *out,
                            PRUint32     inbuflen,
                            PRUint32*    inscanlen
                            );

#define uSubScanner(sub,in,out) (* m_subscanner[sub])((in),(out))

PRIVATE PRBool uCheckAndScanAlways1Byte(
                                        PRInt32*    state,
                                        unsigned char  *in,
                                        PRUint16    *out,
                                        PRUint32     inbuflen,
                                        PRUint32*    inscanlen
                                        );
PRIVATE PRBool uCheckAndScanAlways2Byte(
                                        PRInt32*    state,
                                        unsigned char  *in,
                                        PRUint16    *out,
                                        PRUint32     inbuflen,
                                        PRUint32*    inscanlen
                                        );
PRIVATE PRBool uCheckAndScanAlways2ByteShiftGR(
                                               PRInt32*    state,
                                               unsigned char  *in,
                                               PRUint16    *out,
                                               PRUint32     inbuflen,
                                               PRUint32*    inscanlen
                                               );
PRIVATE PRBool uCheckAndScanAlways2ByteGR128(
                                               PRInt32*    state,
                                               unsigned char  *in,
                                               PRUint16    *out,
                                               PRUint32     inbuflen,
                                               PRUint32*    inscanlen
                                               );
MODULE_PRIVATE PRBool uScanShift(  
                                 uShiftInTable    *shift,
                                 PRInt32*    state,
                                 unsigned char  *in,
                                 PRUint16    *out,
                                 PRUint32     inbuflen,
                                 PRUint32*    inscanlen
                                 );

PRIVATE PRBool uCheckAndScan2ByteGRPrefix8F(
                                            PRInt32*    state,
                                            unsigned char  *in,
                                            PRUint16    *out,
                                            PRUint32     inbuflen,
                                            PRUint32*    inscanlen
                                            );
PRIVATE PRBool uCheckAndScan2ByteGRPrefix8EA2(
                                              PRInt32*    state,
                                              unsigned char  *in,
                                              PRUint16    *out,
                                              PRUint32     inbuflen,
                                              PRUint32*    inscanlen
                                              );
PRIVATE PRBool uCheckAndScan2ByteGRPrefix8EA3(
                                              PRInt32*    state,
                                              unsigned char  *in,
                                              PRUint16    *out,
                                              PRUint32     inbuflen,
                                              PRUint32*    inscanlen
                                              );
PRIVATE PRBool uCheckAndScan2ByteGRPrefix8EA4(
                                              PRInt32*    state,
                                              unsigned char  *in,
                                              PRUint16    *out,
                                              PRUint32     inbuflen,
                                              PRUint32*    inscanlen
                                              );
PRIVATE PRBool uCheckAndScan2ByteGRPrefix8EA5(
                                              PRInt32*    state,
                                              unsigned char  *in,
                                              PRUint16    *out,
                                              PRUint32     inbuflen,
                                              PRUint32*    inscanlen
                                              );
PRIVATE PRBool uCheckAndScan2ByteGRPrefix8EA6(
                                              PRInt32*    state,
                                              unsigned char  *in,
                                              PRUint16    *out,
                                              PRUint32     inbuflen,
                                              PRUint32*    inscanlen
                                              );
PRIVATE PRBool uCheckAndScan2ByteGRPrefix8EA7(
                                              PRInt32*    state,
                                              unsigned char  *in,
                                              PRUint16    *out,
                                              PRUint32     inbuflen,
                                              PRUint32*    inscanlen
                                              );
PRIVATE PRBool uCnSAlways8BytesDecomposedHangul(
                                              PRInt32*    state,
                                              unsigned char  *in,
                                              PRUint16    *out,
                                              PRUint32     inbuflen,
                                              PRUint32*    inscanlen
                                              );
PRIVATE PRBool uCheckAndScanJohabHangul(
                                        PRInt32*    state,
                                        unsigned char  *in,
                                        PRUint16    *out,
                                        PRUint32     inbuflen,
                                        PRUint32*    inscanlen
                                        );
PRIVATE PRBool uCheckAndScanJohabSymbol(
                                        PRInt32*    state,
                                        unsigned char  *in,
                                        PRUint16    *out,
                                        PRUint32     inbuflen,
                                        PRUint32*    inscanlen
                                        );

PRIVATE PRBool uCheckAndScan4BytesGB18030(
                                          PRInt32*    state,
                                          unsigned char  *in,
                                          PRUint16    *out,
                                          PRUint32     inbuflen,
                                          PRUint32*    inscanlen
                                          );

PRIVATE PRBool uScanAlways2Byte(
                                unsigned char*  in,
                                PRUint16*    out
                                );
PRIVATE PRBool uScanAlways2ByteShiftGR(
                                       unsigned char*  in,
                                       PRUint16*    out
                                       );
PRIVATE PRBool uScanAlways1Byte(
                                unsigned char*  in,
                                PRUint16*    out
                                );
PRIVATE PRBool uScanAlways1BytePrefix8E(
                                        unsigned char*  in,
                                        PRUint16*    out
                                        );
                                    


PRIVATE const uScannerFunc m_scanner[uNumOfCharsetType] =
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





PRIVATE const uSubScannerFunc m_subscanner[uNumOfCharType] =
{
    uScanAlways1Byte,
    uScanAlways2Byte,
    uScanAlways2ByteShiftGR,
    uScanAlways1BytePrefix8E
};



MODULE_PRIVATE PRBool uScan(  
                            uScanClassID scanClass,
                            PRInt32*    state,
                            unsigned char  *in,
                            PRUint16    *out,
                            PRUint32     inbuflen,
                            PRUint32*    inscanlen
                            )
{
  return (* m_scanner[scanClass]) (state,in,out,inbuflen,inscanlen);
}



PRIVATE PRBool uScanAlways1Byte(
                                unsigned char*  in,
                                PRUint16*    out
                                )
{
  *out = (PRUint16) in[0];
  return PR_TRUE;
}




PRIVATE PRBool uScanAlways2Byte(
                                unsigned char*  in,
                                PRUint16*    out
                                )
{
  *out = (PRUint16) (( in[0] << 8) | (in[1]));
  return PR_TRUE;
}



PRIVATE PRBool uScanAlways2ByteShiftGR(
                                       unsigned char*  in,
                                       PRUint16*    out
                                       )
{
  *out = (PRUint16) ((( in[0] << 8) | (in[1])) &  0x7F7F);
  return PR_TRUE;
}




PRIVATE PRBool uScanAlways1BytePrefix8E(
                                        unsigned char*  in,
                                        PRUint16*    out
                                        )
{
  *out = (PRUint16) in[1];
  return PR_TRUE;
}



PRIVATE PRBool uCheckAndScanAlways1Byte(
                                        PRInt32*    state,
                                        unsigned char  *in,
                                        PRUint16    *out,
                                        PRUint32     inbuflen,
                                        PRUint32*    inscanlen
                                        )
{
  
  *inscanlen = 1;
  *out = (PRUint16) in[0];
  
  return PR_TRUE;
}




PRIVATE PRBool uCheckAndScanAlways2Byte(
                                        PRInt32*    state,
                                        unsigned char  *in,
                                        PRUint16    *out,
                                        PRUint32     inbuflen,
                                        PRUint32*    inscanlen
                                        )
{
  if(inbuflen < 2)
    return PR_FALSE;
  else
  {
    *inscanlen = 2;
    *out = ((in[0] << 8) | ( in[1])) ;
    return PR_TRUE;
  }
}



PRIVATE PRBool uCheckAndScanAlways2ByteShiftGR(
                                               PRInt32*    state,
                                               unsigned char  *in,
                                               PRUint16    *out,
                                               PRUint32     inbuflen,
                                               PRUint32*    inscanlen
                                               )
{
  





  if(inbuflen < 2)    
    return PR_FALSE;
  else if (! CHK_GR94(in[1]))  
  {
    *inscanlen = 2; 
    *out = 0xFF;  
    return PR_TRUE;
  }
  else
  {
    *inscanlen = 2;
    *out = (((in[0] << 8) | ( in[1]))  & 0x7F7F);
    return PR_TRUE;
  }
}



PRIVATE PRBool uCheckAndScanAlways2ByteGR128(
                                               PRInt32*    state,
                                               unsigned char  *in,
                                               PRUint16    *out,
                                               PRUint32     inbuflen,
                                               PRUint32*    inscanlen
                                               )
{
  






  if(inbuflen < 2)    
    return PR_FALSE;
  else if (! in[1] & 0x80)     
  {
    *inscanlen = 2; 
    *out = 0xFF;  
    return PR_TRUE;
  }
  else
  {
    *inscanlen = 2;
    *out = (in[0] << 8) |  in[1];
    return PR_TRUE;
  }
}



PRIVATE PRBool uScanShift(
                                    uShiftInTable    *shift,
                                    PRInt32*    state,
                                    unsigned char  *in,
                                    PRUint16    *out,
                                    PRUint32     inbuflen,
                                    PRUint32*    inscanlen
                                    )
{
  PRInt16 i;
  const uShiftInCell* cell = &(shift->shiftcell[0]);
  PRInt16 itemnum = shift->numOfItem;
  for(i=0;i<itemnum;i++)
  {
    if( ( in[0] >=  cell[i].shiftin_Min) &&
      ( in[0] <=  cell[i].shiftin_Max))
    {
      if(inbuflen < cell[i].reserveLen)
        return PR_FALSE;
      else
      {
        *inscanlen = cell[i].reserveLen;
        return (uSubScanner(cell[i].classID,in,out));
      }
    }
  }
  return PR_FALSE;
}



PRIVATE PRBool uCheckAndScan2ByteGRPrefix8F(
                                            PRInt32*    state,
                                            unsigned char  *in,
                                            PRUint16    *out,
                                            PRUint32     inbuflen,
                                            PRUint32*    inscanlen
                                            )
{
  if((inbuflen < 3) ||(in[0] != 0x8F)) 
    return PR_FALSE;
  else if (! CHK_GR94(in[1]))  
  {
    *inscanlen = 2; 
    *out = 0xFF;  
    return PR_TRUE;
  }
  else if (! CHK_GR94(in[2]))  
  {
    *inscanlen = 3; 
    *out = 0xFF;  
    return PR_TRUE;
  }
  else
  {
    *inscanlen = 3;
    *out = (((in[1] << 8) | ( in[2]))  & 0x7F7F);
    return PR_TRUE;
  }
}







#define CNS_8EAX_4BYTE(PREFIX)                    \
  if((inbuflen < 4) || (in[0] != 0x8E))           \
    return PR_FALSE;                              \
  else if((in[1] != (PREFIX)))                    \
  {                                               \
    *inscanlen = 2;                               \
    *out = 0xFF;                                  \
    return PR_TRUE;                               \
  }                                               \
  else if(! CHK_GR94(in[2]))                      \
  {                                               \
    *inscanlen = 3;                               \
    *out = 0xFF;                                  \
    return PR_TRUE;                               \
  }                                               \
  else if(! CHK_GR94(in[3]))                      \
  {                                               \
    *inscanlen = 4;                               \
    *out = 0xFF;                                  \
    return PR_TRUE;                               \
  }                                               \
  else                                            \
  {                                               \
    *inscanlen = 4;                               \
    *out = (((in[2] << 8) | ( in[3]))  & 0x7F7F); \
    return PR_TRUE;                               \
  }    

PRIVATE PRBool uCheckAndScan2ByteGRPrefix8EA2(
                                              PRInt32*    state,
                                              unsigned char  *in,
                                              PRUint16    *out,
                                              PRUint32     inbuflen,
                                              PRUint32*    inscanlen
                                              )
{
  CNS_8EAX_4BYTE(0xA2)
}




PRIVATE PRBool uCheckAndScan2ByteGRPrefix8EA3(
                                              PRInt32*    state,
                                              unsigned char  *in,
                                              PRUint16    *out,
                                              PRUint32     inbuflen,
                                              PRUint32*    inscanlen
                                              )
{
  CNS_8EAX_4BYTE(0xA3)
}



PRIVATE PRBool uCheckAndScan2ByteGRPrefix8EA4(
                                              PRInt32*    state,
                                              unsigned char  *in,
                                              PRUint16    *out,
                                              PRUint32     inbuflen,
                                              PRUint32*    inscanlen
                                              )
{
  CNS_8EAX_4BYTE(0xA4)
}



PRIVATE PRBool uCheckAndScan2ByteGRPrefix8EA5(
                                              PRInt32*    state,
                                              unsigned char  *in,
                                              PRUint16    *out,
                                              PRUint32     inbuflen,
                                              PRUint32*    inscanlen
                                              )
{
  CNS_8EAX_4BYTE(0xA5)
}



PRIVATE PRBool uCheckAndScan2ByteGRPrefix8EA6(
                                              PRInt32*    state,
                                              unsigned char  *in,
                                              PRUint16    *out,
                                              PRUint32     inbuflen,
                                              PRUint32*    inscanlen
                                              )
{
  CNS_8EAX_4BYTE(0xA6)
}



PRIVATE PRBool uCheckAndScan2ByteGRPrefix8EA7(
                                              PRInt32*    state,
                                              unsigned char  *in,
                                              PRUint16    *out,
                                              PRUint32     inbuflen,
                                              PRUint32*    inscanlen
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

PRIVATE PRBool uCnSAlways8BytesDecomposedHangul(
                                              PRInt32*    state,
                                              unsigned char  *in,
                                              PRUint16    *out,
                                              PRUint32     inbuflen,
                                              PRUint32*    inscanlen
                                              )
{
  
  PRUint16 LIndex, VIndex, TIndex;
  
  if((inbuflen < 8) || (0xa4 != in[0]) || (0xd4 != in[1]) ||
    (0xa4 != in[2] ) || (0xa4 != in[4]) || (0xa4 != in[6]))
    return PR_FALSE;
  
  
  if((in[3] < 0xa1) && (in[3] > 0xbe)) { 
    return PR_FALSE;
  } 
  else {
    static const PRUint8 lMap[] = {
      
      0,   1,0xff,   2,0xff,0xff,   3,
        
        4,   5,0xff,0xff,0xff,0xff,0xff,0xff,
        
        0xff,   6,   7,   8,0xff,   9,  10,  11,
        
        12,  13,  14,  15,  16,  17,  18     
    };
    
    LIndex = lMap[in[3] - 0xa1];
    if(0xff == (0xff & LIndex))
      return PR_FALSE;
  }
  
  
  if((in[5] < 0xbf) && (in[5] > 0xd3)) { 
    return PR_FALSE;
  } 
  else {
    VIndex = in[5] - 0xbf;
  }
  
  
  if(0xd4 == in[7])  
  {
    TIndex = 0;
  } 
  else if((in[7] < 0xa1) && (in[7] > 0xbe)) {
    return PR_FALSE;
  } 
  else {
    static const PRUint8 tMap[] = {
      
      1,   2,   3,   4,   5,   6,   7,
        
        0xff,   8,   9,  10,  11,  12,  13,  14,
        
        15,  16,  17,0xff,  18,  19,  20,  21,
        
        22,0xff,  23,  24,  25,  26,  27     
    };
    TIndex = tMap[in[7] - 0xa1];
    if(0xff == (0xff & TIndex))
      return PR_FALSE;
  }
  
  *inscanlen = 8;
  
  *out = ( LIndex * VCount + VIndex) * TCount + TIndex + SBase;
  
  return PR_TRUE;
}




PRIVATE PRBool uCheckAndScanJohabHangul(
                                        PRInt32*    state,
                                        unsigned char  *in,
                                        PRUint16    *out,
                                        PRUint32     inbuflen,
                                        PRUint32*    inscanlen
                                        )
{


  if(inbuflen < 2)
    return PR_FALSE;
  else {
  



    static const PRUint8 lMap[32]={ 
      0xff,0xff,0,   1,   2,   3,   4,   5,    
        6,   7,   8,   9,   10,  11,  12,  13,   
        14,  15,  16,  17,  18,  0xff,0xff,0xff, 
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff  
    };
    static const PRUint8 vMap[32]={ 
      0xff,0xff,0xff,0,   1,   2,   3,   4,    
        0xff,0xff,5,   6,   7,   8,   9,   10,   
        0xff,0xff,11,  12,  13,  14,  15,  16,   
        0xff,0xff,17,  18,  19,  20,  0xff,0xff  
    };
    static const PRUint8 tMap[32]={ 
      0xff,0,   1,   2,   3,   4,   5,   6,    
        7,   8,   9,   10,  11,  12,  13,  14,   
        15,  16,  0xff,17,  18,  19,  20,  21,   
        22,  23,  24,  25,  26,  27,  0xff,0xff  
    };
    PRUint16 ch = (in[0] << 8) | in[1];
    PRUint16 LIndex, VIndex, TIndex;
    if(0 == (0x8000 & ch))
      return PR_FALSE;
    LIndex=lMap[(ch>>10)& 0x1F];
    VIndex=vMap[(ch>>5) & 0x1F];
    TIndex=tMap[(ch>>0) & 0x1F];
    if((0xff==(LIndex)) || 
      (0xff==(VIndex)) || 
      (0xff==(TIndex)))
      return PR_FALSE;
    
    *out = ( LIndex * VCount + VIndex) * TCount + TIndex + SBase;
    *inscanlen = 2;
    return PR_TRUE;
  }
}
PRIVATE PRBool uCheckAndScanJohabSymbol(
                                        PRInt32*    state,
                                        unsigned char  *in,
                                        PRUint16    *out,
                                        PRUint32     inbuflen,
                                        PRUint32*    inscanlen
                                        )
{
  if(inbuflen < 2)
    return PR_FALSE;
  else {
  























 
    unsigned char hi = in[0];
    unsigned char lo = in[1];
    PRUint16 offset = (( hi > 223 ) && ( hi < 250)) ? 1 : 0;
    PRUint16 d8_off = 0;
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
    return PR_TRUE;
  }
}
PRIVATE PRBool uCheckAndScan4BytesGB18030(
                                          PRInt32*    state,
                                          unsigned char  *in,
                                          PRUint16    *out,
                                          PRUint32     inbuflen,
                                          PRUint32*    inscanlen
                                          )
{
  PRUint32  data;
  if(inbuflen < 4) 
    return PR_FALSE;
  
  if((in[0] < 0x81 ) || (0xfe < in[0])) 
    return PR_FALSE;
  if((in[1] < 0x30 ) || (0x39 < in[1])) 
    return PR_FALSE;
  if((in[2] < 0x81 ) || (0xfe < in[2])) 
    return PR_FALSE;
  if((in[3] < 0x30 ) || (0x39 < in[3])) 
    return PR_FALSE;
  
  data = (((((in[0] - 0x81) * 10 + (in[1] - 0x30)) * 126) + 
    (in[2] - 0x81)) * 10 ) + (in[3] - 0x30);
  
  *inscanlen = 4;
  if(data >= 0x00010000)  
    return PR_FALSE;
  *out = (PRUint16) data;
  return PR_TRUE;
}

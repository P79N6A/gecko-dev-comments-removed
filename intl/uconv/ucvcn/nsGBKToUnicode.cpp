











#include "nsGBKToUnicode.h"
#include "gbku.h"





class nsGBKUnique2BytesToUnicode : public nsTableDecoderSupport 
{
public:
  nsGBKUnique2BytesToUnicode();
  virtual ~nsGBKUnique2BytesToUnicode() 
    { }
protected:
};

static const uint16_t g_utGBKUnique2Bytes[] = {
#include "gbkuniq2b.ut"
};
nsGBKUnique2BytesToUnicode::nsGBKUnique2BytesToUnicode() 
  : nsTableDecoderSupport(u2BytesCharset, nullptr,
        (uMappingTable*) &g_utGBKUnique2Bytes, 1) 
{
}




class nsGB18030Unique2BytesToUnicode : public nsTableDecoderSupport 
{
public:
  nsGB18030Unique2BytesToUnicode();
  virtual ~nsGB18030Unique2BytesToUnicode() 
    { }
protected:
};

static const uint16_t g_utGB18030Unique2Bytes[] = {
#include "gb18030uniq2b.ut"
};
nsGB18030Unique2BytesToUnicode::nsGB18030Unique2BytesToUnicode() 
  : nsTableDecoderSupport(u2BytesCharset, nullptr,
        (uMappingTable*) &g_utGB18030Unique2Bytes, 1) 
{
}




class nsGB18030Unique4BytesToUnicode : public nsTableDecoderSupport 
{
public:
  nsGB18030Unique4BytesToUnicode();
  virtual ~nsGB18030Unique4BytesToUnicode() 
    { }
protected:
};

static const uint16_t g_utGB18030Unique4Bytes[] = {
#include "gb180304bytes.ut"
};
nsGB18030Unique4BytesToUnicode::nsGB18030Unique4BytesToUnicode() 
  : nsTableDecoderSupport(u4BytesGB18030Charset, nullptr,
        (uMappingTable*) &g_utGB18030Unique4Bytes, 1) 
{
}








#define LEGAL_GBK_MULTIBYTE_FIRST_BYTE(c)  \
      (UINT8_IN_RANGE(0x81, (c), 0xFE))
#define FIRST_BYTE_IS_SURROGATE(c)  \
      (UINT8_IN_RANGE(0x90, (c), 0xFE))
#define LEGAL_GBK_2BYTE_SECOND_BYTE(c) \
      (UINT8_IN_RANGE(0x40, (c), 0x7E)|| UINT8_IN_RANGE(0x80, (c), 0xFE))
#define LEGAL_GBK_4BYTE_SECOND_BYTE(c) \
      (UINT8_IN_RANGE(0x30, (c), 0x39))
#define LEGAL_GBK_4BYTE_THIRD_BYTE(c)  \
      (UINT8_IN_RANGE(0x81, (c), 0xFE))
#define LEGAL_GBK_4BYTE_FORTH_BYTE(c) \
      (UINT8_IN_RANGE(0x30, (c), 0x39))

NS_IMETHODIMP nsGBKToUnicode::ConvertNoBuff(const char* aSrc,
                                            int32_t * aSrcLength,
                                            PRUnichar *aDest,
                                            int32_t * aDestLength)
{
  int32_t i=0;
  int32_t iSrcLength = (*aSrcLength);
  int32_t iDestlen = 0;
  nsresult rv=NS_OK;
  *aSrcLength = 0;
  
  for (i=0;i<iSrcLength;i++)
  {
    if ( iDestlen >= (*aDestLength) )
    {
      rv = NS_OK_UDEC_MOREOUTPUT;
      break;
    }
    
    if(LEGAL_GBK_MULTIBYTE_FIRST_BYTE(*aSrc))
    {
      if(i+1 >= iSrcLength) 
      {
        rv = NS_OK_UDEC_MOREINPUT;
        break;
      }
      
      
      if(LEGAL_GBK_2BYTE_SECOND_BYTE(aSrc[1]))
      {
        
        *aDest = mUtil.GBKCharToUnicode(aSrc[0], aSrc[1]);
        if(UCS2_NO_MAPPING == *aDest)
        { 
          
          
          
          if(! TryExtensionDecoder(aSrc, aDest))
          {
            *aDest = UCS2_NO_MAPPING;
          }
        }
        aSrc += 2;
        i++;
      }
      else if (LEGAL_GBK_4BYTE_SECOND_BYTE(aSrc[1]))
      {
        
        if(i+3 >= iSrcLength)  
        {
          rv = NS_OK_UDEC_MOREINPUT;
          break;
        }
        
        
        
 
        if (LEGAL_GBK_4BYTE_THIRD_BYTE(aSrc[2]) &&
            LEGAL_GBK_4BYTE_FORTH_BYTE(aSrc[3]))
        {
           if ( ! FIRST_BYTE_IS_SURROGATE(aSrc[0])) 
           {
             
             if(! Try4BytesDecoder(aSrc, aDest))
               *aDest = UCS2_NO_MAPPING;
           } else {
              
             if ( (iDestlen+1) < (*aDestLength) )
             {
               if(DecodeToSurrogate(aSrc, aDest))
               {
                 
                 iDestlen++;
                 aDest++;
               }  else {
                 *aDest = UCS2_NO_MAPPING;
              }
             } else {
               if (*aDestLength < 2) {
                 NS_ERROR("insufficient space in output buffer");
                 *aDest = UCS2_NO_MAPPING;
               } else {
                 rv = NS_OK_UDEC_MOREOUTPUT;
                 break;
               }
             }
           }
           aSrc += 4;
           i += 3;
        } else {
          *aDest = UCS2_NO_MAPPING; 
          
          
          
          
          aSrc++;
        }
      }
      else if ((uint8_t) aSrc[0] == (uint8_t)0xA0 )
      {
        
        
        *aDest = CAST_CHAR_TO_UNICHAR(*aSrc);
        aSrc++;
      } else {
        
        *aDest = UCS2_NO_MAPPING;
        aSrc++;
      }
    } else {
      if(IS_ASCII(*aSrc))
      {
        
        *aDest = CAST_CHAR_TO_UNICHAR(*aSrc);
        aSrc++;
      } else {
        if(IS_GBK_EURO(*aSrc)) {
          *aDest = UCS2_EURO;
        } else {
          *aDest = UCS2_NO_MAPPING;
        }
        aSrc++;
      }
    }
    iDestlen++;
    aDest++;
    *aSrcLength = i+1;
  }
  *aDestLength = iDestlen;
  return rv;
}


void nsGBKToUnicode::CreateExtensionDecoder()
{
  mExtensionDecoder = new nsGBKUnique2BytesToUnicode();
}
void nsGBKToUnicode::Create4BytesDecoder()
{
  m4BytesDecoder =  nullptr;
}
void nsGB18030ToUnicode::CreateExtensionDecoder()
{
  mExtensionDecoder = new nsGB18030Unique2BytesToUnicode();
}
void nsGB18030ToUnicode::Create4BytesDecoder()
{
  m4BytesDecoder = new nsGB18030Unique4BytesToUnicode();
}
bool nsGB18030ToUnicode::DecodeToSurrogate(const char* aSrc, PRUnichar* aOut)
{
  NS_ASSERTION(FIRST_BYTE_IS_SURROGATE(aSrc[0]),       "illegal first byte");
  NS_ASSERTION(LEGAL_GBK_4BYTE_SECOND_BYTE(aSrc[1]),   "illegal second byte");
  NS_ASSERTION(LEGAL_GBK_4BYTE_THIRD_BYTE(aSrc[2]),    "illegal third byte");
  NS_ASSERTION(LEGAL_GBK_4BYTE_FORTH_BYTE(aSrc[3]),    "illegal forth byte");
  if(! FIRST_BYTE_IS_SURROGATE(aSrc[0]))
    return false;
  if(! LEGAL_GBK_4BYTE_SECOND_BYTE(aSrc[1]))
    return false;
  if(! LEGAL_GBK_4BYTE_THIRD_BYTE(aSrc[2]))
    return false;
  if(! LEGAL_GBK_4BYTE_FORTH_BYTE(aSrc[3]))
    return false;

  uint8_t a1 = (uint8_t) aSrc[0];
  uint8_t a2 = (uint8_t) aSrc[1];
  uint8_t a3 = (uint8_t) aSrc[2];
  uint8_t a4 = (uint8_t) aSrc[3];
  a1 -= (uint8_t)0x90;
  a2 -= (uint8_t)0x30;
  a3 -= (uint8_t)0x81;
  a4 -= (uint8_t)0x30;
  uint32_t idx = (((a1 * 10 + a2 ) * 126 + a3) * 10) + a4;
  
  if (idx > 0x000FFFFF)
    return false;

  *aOut++ = 0xD800 | (idx >> 10);
  *aOut = 0xDC00 | (0x000003FF & idx);

  return true;
}
bool nsGBKToUnicode::TryExtensionDecoder(const char* aSrc, PRUnichar* aOut)
{
  if(!mExtensionDecoder)
    CreateExtensionDecoder();
  NS_ASSERTION(mExtensionDecoder, "cannot creqte 2 bytes unique converter");
  if(mExtensionDecoder)
  {
    nsresult res = mExtensionDecoder->Reset();
    NS_ASSERTION(NS_SUCCEEDED(res), "2 bytes unique conversoin reset failed");
    int32_t len = 2;
    int32_t dstlen = 1;
    res = mExtensionDecoder->Convert(aSrc,&len, aOut, &dstlen); 
    NS_ASSERTION(NS_FAILED(res) || ((len==2) && (dstlen == 1)), 
       "some strange conversion result");
     
     
    if(NS_SUCCEEDED(res)) 
      return true;
  }
  return  false;
}
bool nsGBKToUnicode::DecodeToSurrogate(const char* aSrc, PRUnichar* aOut)
{
  return false;
}
bool nsGBKToUnicode::Try4BytesDecoder(const char* aSrc, PRUnichar* aOut)
{
  if(!m4BytesDecoder)
    Create4BytesDecoder();
  if(m4BytesDecoder)
  {
    nsresult res = m4BytesDecoder->Reset();
    NS_ASSERTION(NS_SUCCEEDED(res), "4 bytes unique conversoin reset failed");
    int32_t len = 4;
    int32_t dstlen = 1;
    res = m4BytesDecoder->Convert(aSrc,&len, aOut, &dstlen); 
    NS_ASSERTION(NS_FAILED(res) || ((len==4) && (dstlen == 1)), 
       "some strange conversion result");
     
     
    if(NS_SUCCEEDED(res)) 
      return true;
  }
  return  false;
}





































 










#include "nsUnicodeToGBK.h"
#include "nsICharRepresentable.h"
#include "nsUCvCnDll.h"
#include "gbku.h"
#include "uconvutil.h"
#include "nsUnicharUtils.h"









static const PRUint16 g_uf_gb18030_2bytes[] = {
#include "gb18030uniq2b.uf"
};
class nsUnicodeToGB18030Uniq2Bytes : public nsTableEncoderSupport
{
public: 
  nsUnicodeToGB18030Uniq2Bytes() 
    : nsTableEncoderSupport(u2BytesCharset,
                            (uMappingTable*) &g_uf_gb18030_2bytes, 2) {};
protected: 
};




static const PRUint16 g_uf_gb18030_4bytes[] = {
#include "gb180304bytes.uf"
};
class nsUnicodeTo4BytesGB18030 : public nsTableEncoderSupport
{
public: 
  nsUnicodeTo4BytesGB18030()
    : nsTableEncoderSupport(u4BytesGB18030Charset, 
                             (uMappingTable*) &g_uf_gb18030_4bytes, 4) {};
protected: 
};




static const PRUint16 g_uf_gbk_2bytes[] = {
#include "gbkuniq2b.uf"
};
class nsUnicodeToGBKUniq2Bytes : public nsTableEncoderSupport
{
public: 
  nsUnicodeToGBKUniq2Bytes()
    : nsTableEncoderSupport(u2BytesCharset, 
                             (uMappingTable*) &g_uf_gbk_2bytes, 2) {};
protected: 
};



void nsUnicodeToGB18030::CreateExtensionEncoder()
{
  mExtensionEncoder = new nsUnicodeToGB18030Uniq2Bytes();
}
void nsUnicodeToGB18030::Create4BytesEncoder()
{
  m4BytesEncoder = new nsUnicodeTo4BytesGB18030();
}

PRBool nsUnicodeToGB18030::EncodeSurrogate(
  PRUnichar aSurrogateHigh,
  PRUnichar aSurrogateLow,
  char* aOut)
{
  if( NS_IS_HIGH_SURROGATE(aSurrogateHigh) && 
      NS_IS_LOW_SURROGATE(aSurrogateLow) )
  {
    
    PRUint32 idx = ((aSurrogateHigh - (PRUnichar)0xD800) << 10 ) |
                   (aSurrogateLow - (PRUnichar) 0xDC00);

    unsigned char *out = (unsigned char*) aOut;
    
    out[0] = (idx / (10*126*10)) + 0x90; 
    idx %= (10*126*10);
    out[1] = (idx / (10*126)) + 0x30;
    idx %= (10*126);
    out[2] = (idx / (10)) + 0x81;
    out[3] = (idx % 10) + 0x30;
    return PR_TRUE;
  } 
  return PR_FALSE; 
} 

#ifdef MOZ_EXTRA_X11CONVERTERS



NS_IMETHODIMP nsUnicodeToGB18030Font0::GetMaxLength(const PRUnichar * aSrc, 
                                              PRInt32 aSrcLength,
                                              PRInt32 * aDestLength)
{
  *aDestLength = 2 * aSrcLength;
  return NS_OK_UDEC_EXACTLENGTH; 
}
void nsUnicodeToGB18030Font0::Create4BytesEncoder()
{
  m4BytesEncoder = nsnull;
}

NS_IMETHODIMP nsUnicodeToGB18030Font0::FillInfo(PRUint32 *aInfo)
{
  nsresult rv = nsUnicodeToGB18030::FillInfo(aInfo); 
  if(NS_SUCCEEDED(rv))
  {
    
    aInfo[0] = aInfo[1] = aInfo[2] = aInfo[3] = 0;
  }
  return rv;
}




nsUnicodeToGB18030Font1::nsUnicodeToGB18030Font1()
  : nsTableEncoderSupport(u2BytesCharset,
                             (uMappingTable*) &g_uf_gb18030_4bytes, 4)
{

}

NS_IMETHODIMP nsUnicodeToGB18030Font1::FillInfo(PRUint32 *aInfo)
{
  nsresult res = nsTableEncoderSupport::FillInfo(aInfo);
  PRUint32 i;

  for(i = (0x0000 >> 5); i<(0x0600 >> 5); i++)
    aInfo[i] = 0;

  
  for(i = (0x0600 >> 5); i<(0x06e0 >> 5); i++)
    aInfo[i] = 0;
  SET_REPRESENTABLE(aInfo, 0x060c);
  SET_REPRESENTABLE(aInfo, 0x061b);
  SET_REPRESENTABLE(aInfo, 0x061f);
  for(i = 0x0626;i <=0x0628;i++)
    SET_REPRESENTABLE(aInfo, i);
  SET_REPRESENTABLE(aInfo, 0x062a);
  for(i = 0x062c;i <=0x062f;i++)
    SET_REPRESENTABLE(aInfo, i);
  for(i = 0x0631;i <=0x0634;i++)
    SET_REPRESENTABLE(aInfo, i);
  for(i = 0x0639;i <=0x063a;i++)
    SET_REPRESENTABLE(aInfo, i);
  for(i = 0x0640;i <=0x064a;i++)
    SET_REPRESENTABLE(aInfo, i);
  for(i = 0x0674;i <=0x0678;i++)
    SET_REPRESENTABLE(aInfo, i);
  SET_REPRESENTABLE(aInfo, 0x067e);
  SET_REPRESENTABLE(aInfo, 0x0686);
  SET_REPRESENTABLE(aInfo, 0x0698);
  SET_REPRESENTABLE(aInfo, 0x06a9);
  SET_REPRESENTABLE(aInfo, 0x06ad);
  SET_REPRESENTABLE(aInfo, 0x06af);
  SET_REPRESENTABLE(aInfo, 0x06be);
  for(i = 0x06c5;i <=0x06c9;i++)
    SET_REPRESENTABLE(aInfo, i);
  for(i = 0x06cb;i <=0x06cc;i++)
    SET_REPRESENTABLE(aInfo, i);
  SET_REPRESENTABLE(aInfo, 0x06d0);
  SET_REPRESENTABLE(aInfo, 0x06d5);
  

  for(i = (0x06e0 >> 5); i<(0x0f00 >> 5); i++)
    aInfo[i] = 0;

  
  CLEAR_REPRESENTABLE(aInfo, 0x0f48);
  for(i = 0x0f6b;i <0x0f71;i++)
    CLEAR_REPRESENTABLE(aInfo, i);
  for(i = 0x0f8c;i <0x0f90;i++)
    CLEAR_REPRESENTABLE(aInfo, i);
  CLEAR_REPRESENTABLE(aInfo, 0x0f98);
  CLEAR_REPRESENTABLE(aInfo, 0x0fbd);
  CLEAR_REPRESENTABLE(aInfo, 0x0fcd);
  CLEAR_REPRESENTABLE(aInfo, 0x0fce);
  for(i = 0x0fd0;i <0x0fe0;i++)
    CLEAR_REPRESENTABLE(aInfo, i);
  

  for(i = (0x0fe0 >> 5); i<(0x1800 >> 5); i++)
    aInfo[i] = 0;

  
  CLEAR_REPRESENTABLE(aInfo, 0x180f);
  for(i = 0x181a;i <0x1820;i++)
    CLEAR_REPRESENTABLE(aInfo, i);
  for(i = 0x1878;i <0x1880;i++)
    CLEAR_REPRESENTABLE(aInfo, i);
  for(i = 0x18aa;i <0x18c0;i++)
    CLEAR_REPRESENTABLE(aInfo, i);
  

  for(i = (0x18c0 >> 5); i<(0x3400 >> 5); i++)
    aInfo[i] = 0;

  
  
  
  for(i = 0x4db6;i <0x4dc0;i++)
    CLEAR_REPRESENTABLE(aInfo, i);
  

  
  for(i = (0x4dc0 >> 5);i < (0xa000>>5) ; i++)
    aInfo[i] = 0;

  
  for(i = 0xa48d;i <0xa490;i++)
    CLEAR_REPRESENTABLE(aInfo, i);

  CLEAR_REPRESENTABLE(aInfo, 0xa4a2);
  CLEAR_REPRESENTABLE(aInfo, 0xa4a3);
  CLEAR_REPRESENTABLE(aInfo, 0xa4b4);
  CLEAR_REPRESENTABLE(aInfo, 0xa4c1);
  CLEAR_REPRESENTABLE(aInfo, 0xa4c5);

  for(i = 0xa4c7;i <0xa4e0;i++)
    CLEAR_REPRESENTABLE(aInfo, i);
  

  for(i = (0xa4e0 >> 5);i < (0xfb00>>5) ; i++)
    aInfo[i] = 0;

  
  for(i = (0xfb00 >> 5);i < (0xfc00>>5) ; i++)
    aInfo[i] = 0;
  for(i = 0xfb56;i <=0xfb59;i++)
    SET_REPRESENTABLE(aInfo, i);
  for(i = 0xfb7a;i <=0xfb95;i++)
    SET_REPRESENTABLE(aInfo, i);
  for(i = 0xfbaa;i <=0xfbad;i++)
    SET_REPRESENTABLE(aInfo, i);
  for(i = 0xfbd3;i <=0xfbff;i++)
    SET_REPRESENTABLE(aInfo, i);
  

  for(i = (0xfc00 >> 5);i < (0xfe80>>5) ; i++)
    aInfo[i] = 0;

  
  for(i = (0xfe80 >> 5);i < (0x10000>>5) ; i++)
    aInfo[i] = 0;
  for(i = 0xfe89;i <=0xfe98;i++)
    SET_REPRESENTABLE(aInfo, i);
  for(i = 0xfe9d;i <=0xfeaa;i++)
    SET_REPRESENTABLE(aInfo, i);
  SET_REPRESENTABLE(aInfo, 0xfead);
  for(i = 0xfeae;i <=0xfeb8;i++)
    SET_REPRESENTABLE(aInfo, i);
  for(i = 0xfec9;i <=0xfef4;i++)
    SET_REPRESENTABLE(aInfo, i);
  SET_REPRESENTABLE(aInfo, 0xfefb);
  SET_REPRESENTABLE(aInfo, 0xfefc);
  
  return res;
}
#endif



nsUnicodeToGBK::nsUnicodeToGBK(PRUint32 aMaxLength) :
  nsEncoderSupport(aMaxLength)
{
  mExtensionEncoder = nsnull;
  m4BytesEncoder = nsnull;
  mUtil.InitToGBKTable();
  mSurrogateHigh = 0;
}
void nsUnicodeToGBK::CreateExtensionEncoder()
{
  mExtensionEncoder = new nsUnicodeToGBKUniq2Bytes();
}
void nsUnicodeToGBK::Create4BytesEncoder()
{
  m4BytesEncoder = nsnull;
}
PRBool nsUnicodeToGBK::TryExtensionEncoder(
  PRUnichar aChar,
  char* aOut,
  PRInt32 *aOutLen
)
{
  if( NS_IS_HIGH_SURROGATE(aChar) || 
      NS_IS_LOW_SURROGATE(aChar) )
  {
    
    return PR_FALSE;
  }
  if(! mExtensionEncoder )
    CreateExtensionEncoder();
  if(mExtensionEncoder) 
  {
    PRInt32 len = 1;
    nsresult res = NS_OK;
    res = mExtensionEncoder->Convert(&aChar, &len, aOut, aOutLen);
    if(NS_SUCCEEDED(res) && (*aOutLen > 0))
      return PR_TRUE;
  }
  return PR_FALSE;
}

PRBool nsUnicodeToGBK::Try4BytesEncoder(
  PRUnichar aChar,
  char* aOut,
  PRInt32 *aOutLen
)
{
  if( NS_IS_HIGH_SURROGATE(aChar) || 
      NS_IS_LOW_SURROGATE(aChar) )
  {
    
    return PR_FALSE;
  }
  if(! m4BytesEncoder )
    Create4BytesEncoder();
  if(m4BytesEncoder) 
  {
    PRInt32 len = 1;
    nsresult res = NS_OK;
    res = m4BytesEncoder->Convert(&aChar, &len, aOut, aOutLen);
    NS_ASSERTION(NS_FAILED(res) || ((1 == len) && (4 == *aOutLen)),
      "unexpect conversion length");
    if(NS_SUCCEEDED(res) && (*aOutLen > 0))
      return PR_TRUE;
  }
  return PR_FALSE;
}
PRBool nsUnicodeToGBK::EncodeSurrogate(
  PRUnichar aSurrogateHigh,
  PRUnichar aSurrogateLow,
  char* aOut)
{
  return PR_FALSE; 
} 

NS_IMETHODIMP nsUnicodeToGBK::ConvertNoBuff(
  const PRUnichar * aSrc, 
  PRInt32 * aSrcLength, 
  char * aDest, 
  PRInt32 * aDestLength)
{
  PRInt32 iSrcLength = 0;
  PRInt32 iDestLength = 0;
  PRUnichar unicode;
  nsresult res = NS_OK;
  while (iSrcLength < *aSrcLength )
  {
    unicode = *aSrc;
    
    if(IS_ASCII(unicode))
    {
      
      *aDest = CAST_UNICHAR_TO_CHAR(*aSrc);
      aDest++; 
      iDestLength +=1;
    } else {
      char byte1, byte2;
      if(mUtil.UnicodeToGBKChar( unicode, PR_FALSE, &byte1, &byte2))
      {
        
        if(iDestLength+2 > *aDestLength)
        {
          res = NS_OK_UENC_MOREOUTPUT;
          break;
        }
        aDest[0] = byte1;
        aDest[1] = byte2;
        aDest += 2;	
        iDestLength +=2;
      } else {
        PRInt32 aOutLen = 2;
        
        if(iDestLength+2 > *aDestLength)
        {
          res = NS_OK_UENC_MOREOUTPUT;
          break;
        }
        
        
        
        if(TryExtensionEncoder(unicode, aDest, &aOutLen))
        {
          iDestLength += aOutLen;
          aDest += aOutLen;
        } else {
          
          if(iDestLength+4 > *aDestLength)
          {
            res = NS_OK_UENC_MOREOUTPUT;
            break;
          }
          
          
          aOutLen = 4;
          if( NS_IS_HIGH_SURROGATE(unicode) )
          {
            if((iSrcLength+1) < *aSrcLength ) {
              if(EncodeSurrogate(aSrc[0],aSrc[1], aDest)) {
                
                iSrcLength++ ; 
                aSrc++;
                iDestLength += aOutLen;
                aDest += aOutLen;
              } else {
                
                res = NS_ERROR_UENC_NOMAPPING;
                iSrcLength++;   
                break;
              }
            } else {
              mSurrogateHigh = aSrc[0];
              break; 
            }
          } else {
            if( NS_IS_LOW_SURROGATE(unicode) )
            {
              if(NS_IS_HIGH_SURROGATE(mSurrogateHigh)) {
                if(EncodeSurrogate(mSurrogateHigh, aSrc[0], aDest)) {
                  iDestLength += aOutLen;
                  aDest += aOutLen;
                } else {
                  
                  res = NS_ERROR_UENC_NOMAPPING;
                  iSrcLength++;   
                  break;
                }
              } else {
                
                res = NS_ERROR_UENC_NOMAPPING;
                iSrcLength++;   
                break;
              }
            } else {
              if(Try4BytesEncoder(unicode, aDest, &aOutLen))
              {
                NS_ASSERTION((aOutLen == 4), "we should always generate 4 bytes here");
                iDestLength += aOutLen;
                aDest += aOutLen;
              } else {
                res = NS_ERROR_UENC_NOMAPPING;
                iSrcLength++;   
                break;
              }
            }
          }
        }
      } 
    }
    iSrcLength++ ; 
    mSurrogateHigh = 0;
    aSrc++;
    if ( iDestLength >= (*aDestLength) && (iSrcLength < *aSrcLength) )
    {
      res = NS_OK_UENC_MOREOUTPUT;
      break;
    }
  }

  *aDestLength = iDestLength;
  *aSrcLength = iSrcLength;
  return res;
}




NS_IMETHODIMP nsUnicodeToGBK::FillInfo(PRUint32 *aInfo)
{
  mUtil.FillInfo(aInfo, 0x81, 0xFE, 0x40, 0xFE);
  if(! mExtensionEncoder )
    CreateExtensionEncoder();
  if(mExtensionEncoder) 
  {
    nsCOMPtr<nsICharRepresentable> aRep = do_QueryInterface(mExtensionEncoder);
    aRep->FillInfo(aInfo);
  }
  
  if(! m4BytesEncoder )
    Create4BytesEncoder();
  if(m4BytesEncoder) 
  {
    nsCOMPtr<nsICharRepresentable> aRep = do_QueryInterface(m4BytesEncoder);
    aRep->FillInfo(aInfo);
  }

  
  for (PRUint16 SrcUnicode = 0x0000; SrcUnicode <= 0x007F; SrcUnicode++)
    SET_REPRESENTABLE(aInfo, SrcUnicode);
  SET_REPRESENTABLE(aInfo, 0x20ac); 
  return NS_OK;
}









































#include "nsUnicodeToTSCII.h"
#include "nsMemory.h"
#include "tamil.h"














  
NS_IMPL_ISUPPORTS2(nsUnicodeToTSCII, nsIUnicodeEncoder, nsICharRepresentable)











static const PRUint8 UnicharToTSCII[] =
{
     0,    0,    0, 0xb7,    0, 0xab, 0xac, 0xfe, 
  0xae, 0xaf, 0xb0,    0,    0,    0, 0xb1, 0xb2, 
  0xb3,    0, 0xb4, 0xb5, 0xb6, 0xb8,    0,    0, 
     0, 0xb9, 0xba,    0, 0x83,    0, 0xbb, 0xbc, 
     0,    0,    0, 0xbd, 0xbe,    0,    0,    0, 
  0xbf, 0xc9, 0xc0,    0,    0,    0, 0xc1, 0xc2, 
  0xc3, 0xc8, 0xc4, 0xc7, 0xc6, 0xc5,    0, 0x84, 
  0x85, 0x86,    0,    0,    0,    0, 0xa1, 0xa2, 
  0xa3, 0xa4, 0xa5,    0,    0,    0, 0xa6, 0xa7, 
  0xa8,    0,    0,    0,    0,    0,    0,    0, 
     0,    0,    0,    0,    0,    0,    0, 0xaa, 
     0,    0,    0,    0,    0,    0,    0,    0, 
     0,    0,    0,    0,    0,    0, 0x80, 0x81, 
  0x8d, 0x8e, 0x8f, 0x90, 0x95, 0x96, 0x97, 0x98, 
  0x9d, 0x9e, 0x9f,    0,    0,    0,    0,    0, 
     0,    0,    0,    0,    0,    0,    0,    0  
};

static const PRUint8 consonant_with_u[] =
{
  0xcc, 0x99, 0xcd, 0x9a, 0xce, 0xcf, 0xd0, 0xd1, 0xd2,
  0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb
};

static const PRUint8 consonant_with_uu[] =
{
  0xdc, 0x9b, 0xdd, 0x9c, 0xde, 0xdf, 0xe0, 0xe1, 0xe2,
  0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb
};

static const PRUint8 consonant_with_virama[18] =
{
  0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1, 0xf2, 0xf3, 0xf4,
  0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd
};





NS_IMETHODIMP 
nsUnicodeToTSCII::Convert(const PRUnichar * aSrc, PRInt32 * aSrcLength, 
                          char * aDest, PRInt32 * aDestLength)
{
  const PRUnichar * src = aSrc;
  const PRUnichar * srcEnd = aSrc + *aSrcLength;
  char * dest = aDest;
  char * destEnd = dest + *aDestLength;

  nsresult rv = NS_OK;
                      
  while (src < srcEnd && dest < destEnd) {
    PRUnichar ch = *src;
    if (mBuffer) {                        
      
      PRUint32 last = mBuffer;
                            
      
      if (IS_TSC_CONSONANT(last)) {                      
        if (ch == UNI_VOWELSIGN_U && IS_TSC_CONSONANT1(last)) {                      
          *dest++ = consonant_with_u[last - TSC_KA];
          mBuffer = 0;                  
          ++src;
          continue;
        }                      
  
        if (ch == UNI_VOWELSIGN_UU && IS_TSC_CONSONANT1(last)) {                      
          *dest++ = consonant_with_uu[last - TSC_KA];          
          mBuffer = 0;                  
          ++src;                  
          continue;                  
        }                      
  
        
        if (IS_UNI_LEFT_VOWELSIGN(ch)) {                      
          if (dest + 2 > destEnd)
            goto error_more_output;
          *dest++ = TSC_LEFT_VOWELSIGN(ch);
          *dest++ = last;                
          mBuffer = 0;                
          ++src;                  
          continue;                  
        }                      
  
        
        if (IS_UNI_2PARTS_VOWELSIGN(ch)) {                      
          if (dest + 3 > destEnd)
            goto error_more_output;
          *dest++ = TSC_LEFT_VOWEL_PART(ch);
          *dest++ = last;                
          *dest++ = TSC_RIGHT_VOWEL_PART(ch);
          mBuffer = 0;                
          ++src;                  
          continue;                  
        }                      
  
        
        if (ch == UNI_VIRAMA) {                      
          
          
          if (last == TSC_KA) {                 
            mBuffer = TSC_KA_DEAD;
          }
          
          
          else if (last == TSC_SA) {
            mBuffer = TSC_SA_DEAD;                
          }
          else {                    
            *dest++ = IS_TSC_CONSONANT1(last) ?
              consonant_with_virama[last - TSC_KA] : last + 5;
            mBuffer = 0;                
          }                    
          ++src;                  
          continue;                  
        }                      

        
        if (last == TSC_TA && (ch == UNI_VOWELSIGN_I || ch == UNI_VOWELSIGN_II)) {                      
          *dest++ = ch - (UNI_VOWELSIGN_I - TSC_TI_LIGA);
          mBuffer = 0;                  
          ++src;                  
          continue;                  
        }                      
      }                      
      else if (last == TSC_KA_DEAD) {                      
        
        if (ch == UNI_SSA) {                      
          mBuffer = TSC_KSSA; 
          ++src;                  
          continue;                  
        }                      
      }                      
      else if (last == TSC_SA_DEAD) {                      
        
        if (ch == UNI_RA) {                      
          mBuffer = 0xc38a;                
          ++src;                  
          continue;                  
        }                      
      }                      
      else if (last == TSC_KSSA) {                      
        if (ch == UNI_VIRAMA) {
          *dest++ = (char) TSC_KSSA_DEAD;
          mBuffer = 0;                  
          ++src;                  
          continue;                  
        }                      

        
        
        if (IS_UNI_LEFT_VOWELSIGN(ch)) {                      
          if (dest + 2 > destEnd)
            goto error_more_output;
          *dest++ = TSC_LEFT_VOWELSIGN(ch);
          *dest++ = last;                
          mBuffer = 0;                
          ++src;                  
          continue;                  
        }                      
  
        
        if (IS_UNI_2PARTS_VOWELSIGN(ch)) {                      
          if (dest + 3 > destEnd)
            goto error_more_output;
          *dest++ = TSC_LEFT_VOWEL_PART(ch);
          *dest++ = last;                
          *dest++ = TSC_RIGHT_VOWEL_PART(ch);
          mBuffer = 0;                
          ++src;                  
          continue;                  
        }                      
      }                      
      else {
        NS_ASSERTION(last == 0xc38a, "No other value can be buffered");
        if (ch == UNI_VOWELSIGN_II) {                      
          *dest++ = (char) TSC_SRII_LIGA;
          mBuffer = 0;                  
          ++src;                  
          continue;                  
        }                      
        else {
          
          *dest++ = (char) TSC_SA_DEAD;
          mBuffer = TSC_RA;
          ++src;                  
          continue;                  
        }  
      }                      
                          
                    
      if (last >> 8) {                      
        if (dest + 2 >  destEnd)
          goto error_more_output;
        *dest++ = last & 0xff;              
        *dest++ = (last >> 8) & 0xff;              
      }                      
      else                      
        *dest++ = last & 0xff;                
      mBuffer = 0;                    
      continue;                    
    }                        
                        
    if (ch < 0x80)   
      *dest++ = (char)ch;                    
    else if (IS_UNI_TAMIL(ch)) {                        
      PRUint8 t = UnicharToTSCII[ch - UNI_TAMIL_START];
                            
      if (t != 0) {                      
          if (IS_TSC_CONSONANT(t))
            mBuffer = (PRUint32) t;              
          else                    
            *dest++ = t;                  
      }                      
      else if (IS_UNI_2PARTS_VOWELSIGN(ch)) {   
          
          if (dest + 2 > destEnd)
            goto error_more_output;

          *dest++ = TSC_LEFT_VOWEL_PART(ch);
          *dest++ = TSC_RIGHT_VOWEL_PART(ch);
      }                      
      else {
        *aDestLength = dest - aDest;
        return NS_ERROR_UENC_NOMAPPING;
      }                      
    }                        
    else if (ch == 0x00A9)                  
      *dest++ = (char)ch;                    
    else if (IS_UNI_SINGLE_QUOTE(ch))
      *dest++ = ch - UNI_LEFT_SINGLE_QUOTE + TSC_LEFT_SINGLE_QUOTE;
    else if (IS_UNI_DOUBLE_QUOTE(ch))
      *dest++ = ch - UNI_LEFT_DOUBLE_QUOTE + TSC_LEFT_DOUBLE_QUOTE;
    else {
      *aDestLength = dest - aDest;
      return NS_ERROR_UENC_NOMAPPING;
    }                        
                        
            
    ++src;                      
  }

  
  if (mBuffer >> 8) {                      
    
    if (dest + 2 > destEnd)
      goto error_more_output;
    *dest++ = (mBuffer >> 8) & 0xff;            
    *dest++ = mBuffer & 0xff;              
    mBuffer = 0;
  }                      
  else if (mBuffer) {
    
    if (dest >= destEnd)
      goto error_more_output;
    *dest++ = mBuffer & 0xff;              
    mBuffer = 0;
  }                      

  *aSrcLength = src - aSrc;
  *aDestLength = dest - aDest;
  return rv;

error_more_output:
  *aSrcLength = src - aSrc;
  *aDestLength = dest - aDest;
  return NS_OK_UENC_MOREOUTPUT;
}

NS_IMETHODIMP 
nsUnicodeToTSCII::Finish(char* aDest, PRInt32* aDestLength)
{
  if (!mBuffer) {
    *aDestLength = 0;
    return NS_OK;
  }

  if (mBuffer >> 8) {                      
    
    if (*aDestLength < 2) {
      *aDestLength = 0;
      return NS_OK_UENC_MOREOUTPUT;
    }
    *aDest++ = (mBuffer >> 8) & 0xff;            
    *aDest++ = mBuffer & 0xff;              
    mBuffer = 0;
    *aDestLength = 2;
  }                      
  else {                      
    
    if (*aDestLength < 1) {                    
      *aDestLength = 0;
      return NS_OK_UENC_MOREOUTPUT;
    }
    *aDest++ = mBuffer & 0xff;              
    mBuffer = 0;
    *aDestLength = 1;
  }                      
  return NS_OK;
}


NS_IMETHODIMP 
nsUnicodeToTSCII::Reset()
{
  mBuffer = 0;
  return NS_OK;
}

NS_IMETHODIMP 
nsUnicodeToTSCII::GetMaxLength(const PRUnichar * aSrc, PRInt32 aSrcLength,
                                 PRInt32 * aDestLength)
{
  
  *aDestLength = aSrcLength *  2;
  return NS_OK;
}


NS_IMETHODIMP 
nsUnicodeToTSCII::FillInfo(PRUint32* aInfo)
{
  
  static const PRUint8 coverage[] = {
    0xe8, 
    0xc7, 
    0x3d, 
    0xd6, 
    0x18, 
    0xc7, 
    0xbf, 
    0xc7, 
    0xc7, 
    0x3d, 
    0x80, 
    0x00, 
    0x80, 
    0xff, 
    0x07, 
  };

  PRUnichar i;
  for(i = 0; i <  0x78; i++)
    if (coverage[i / 8] & (1 << (i % 8)))
      SET_REPRESENTABLE(aInfo, i + UNI_TAMIL_START);

  
  for(i = 0x20; i < 0x7f; i++)
     SET_REPRESENTABLE(aInfo, i);

  
  SET_REPRESENTABLE(aInfo, 0xA9);   
  SET_REPRESENTABLE(aInfo, UNI_LEFT_SINGLE_QUOTE);
  SET_REPRESENTABLE(aInfo, UNI_RIGHT_SINGLE_QUOTE);
  SET_REPRESENTABLE(aInfo, UNI_LEFT_DOUBLE_QUOTE);
  SET_REPRESENTABLE(aInfo, UNI_RIGHT_DOUBLE_QUOTE);

  return NS_OK;
}

NS_IMETHODIMP 
nsUnicodeToTSCII::SetOutputErrorBehavior(PRInt32 aBehavior, 
                                           nsIUnicharEncoder *aEncoder, 
                                           PRUnichar aChar)
{
  return NS_OK;
}



const static PRUnichar gTSCIIToTTF[] = {
  0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
  0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008D, 0x017D, 0x008F,
  0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
  0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x017E, 0x0178
};




NS_IMPL_ISUPPORTS_INHERITED0(nsUnicodeToTamilTTF, nsUnicodeToTSCII)

NS_IMETHODIMP 
nsUnicodeToTamilTTF::Convert(const PRUnichar * aSrc, 
                             PRInt32 * aSrcLength, char * aDest, 
                             PRInt32 * aDestLength)
{

  PRInt32 medLen, destLen;
  char *med;

  GetMaxLength(aSrc, *aSrcLength, &destLen);
  NS_ASSERTION(destLen  <= *aDestLength, "insufficient dest. buffer size");

  
  
  medLen = destLen / 2; 

  if (medLen > CHAR_BUFFER_SIZE) {
    med = (char *) nsMemory::Alloc(medLen);
    if (!med)
      return NS_ERROR_OUT_OF_MEMORY;
  }
  else 
    med = mStaticBuffer;

  nsresult rv = nsUnicodeToTSCII::Convert(aSrc, aSrcLength, med, &medLen);

  if (NS_FAILED(rv)) {
    if (med != mStaticBuffer)
      nsMemory::Free(med);
    return rv;
  }

  PRInt32 i, j;

  
  for (i = 0, j = 0; i < medLen; i++) {
    
    PRUnichar ucs2 = (med[i] & 0xe0) == 0x80 ? 
                     gTSCIIToTTF[med[i] & 0x7f] : PRUint8(med[i]);
    
    
    if (ucs2 == 0xfe) ucs2 = 0xad;
    aDest[j++] = PRUint8((ucs2 & 0xff00) >> 8);
    aDest[j++] = PRUint8(ucs2 & 0x00ff);
  }

  *aDestLength = j;

  if (med != mStaticBuffer)
    nsMemory::Free(med);

  return NS_OK;
}

NS_IMETHODIMP
nsUnicodeToTamilTTF::GetMaxLength(const PRUnichar * aSrc, PRInt32 aSrcLength, PRInt32 * aDestLength)
{
  
  
  
  *aDestLength = (aSrcLength + 1) *  4; 
  
  return NS_OK;
}

NS_IMETHODIMP 
nsUnicodeToTamilTTF::SetOutputErrorBehavior(PRInt32 aBehavior, 
                                            nsIUnicharEncoder *aEncoder, 
                                            PRUnichar aChar)
{
  if (aBehavior == kOnError_CallBack && aEncoder == nsnull)
    return NS_ERROR_NULL_POINTER;
  mErrEncoder = aEncoder;
  mErrBehavior = aBehavior;
  mErrChar = aChar;
  return NS_OK;
}


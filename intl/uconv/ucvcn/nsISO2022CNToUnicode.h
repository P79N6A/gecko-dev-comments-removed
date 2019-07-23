




































#ifndef nsISO2022CNToUnicode_h__
#define nsISO2022CNToUnicode_h__
#include "nsCOMPtr.h"
#include "nsISupports.h"
#include "nsUCSupport.h"

#define MBYTE       0x8e
#undef PMASK
#define PMASK       0xa0

#define SI          0x0f 
#define SO          0x0e
#define ESC         0x1b
#define SS2         0x4e
#define SS3         0x4f

class nsISO2022CNToUnicode : public nsBasicDecoderSupport
{
public:
  nsISO2022CNToUnicode() : 
        mState(eState_ASCII), 
        mPlaneID(0) { }

  virtual ~nsISO2022CNToUnicode() {}

  NS_IMETHOD Convert(const char *aSrc, PRInt32 * aSrcLength,
     PRUnichar * aDest, PRInt32 * aDestLength) ;

  NS_IMETHOD GetMaxLength(const char * aSrc, PRInt32 aSrcLength,
     PRInt32 * aDestLength)
  {
    *aDestLength = aSrcLength;
    return NS_OK;
  }

  NS_IMETHOD Reset()
  {
    mState = eState_ASCII;
    mPlaneID = 0;

    return NS_OK;
  }

private:
  
  enum {
    eState_ASCII,
    eState_ESC,                           
    eState_ESC_24,                        

    eState_ESC_24_29,                     
    eState_ESC_24_29_A,                   
    eState_GB2312_1980,                   
    eState_GB2312_1980_2ndbyte,           
    eState_ESC_24_29_A_SO_SI,             
    eState_ESC_24_29_G,                   
    eState_CNS11643_1,                    
    eState_CNS11643_1_2ndbyte,            
    eState_ESC_24_29_G_SO_SI,             

    eState_ESC_24_2A,                     
    eState_ESC_24_2A_H,                   
    eState_ESC_24_2A_H_ESC,               
    eState_CNS11643_2,                    
    eState_CNS11643_2_2ndbyte,            
    eState_ESC_24_2A_H_ESC_SS2_SI,        
    eState_ESC_24_2A_H_ESC_SS2_SI_ESC,    

    eState_ESC_24_2B,                     
    eState_ESC_24_2B_I,                   
    eState_ESC_24_2B_I_ESC,               
    eState_CNS11643_3,                    
    eState_CNS11643_3_2ndbyte,            
    eState_ESC_24_2B_I_ESC_SS3_SI,        
    eState_ESC_24_2B_I_ESC_SS3_SI_ESC,    
    eState_ERROR
  } mState;

  char mData;

  
  int mPlaneID;

  
  nsCOMPtr<nsIUnicodeDecoder> mGB2312_Decoder;
  nsCOMPtr<nsIUnicodeDecoder> mEUCTW_Decoder;

  NS_IMETHOD GB2312_To_Unicode(unsigned char *aSrc, PRInt32 aSrcLength,
     PRUnichar * aDest, PRInt32 * aDestLength) ;

  NS_IMETHOD EUCTW_To_Unicode(unsigned char *aSrc, PRInt32 aSrcLength,
     PRUnichar * aDest, PRInt32 * aDestLength) ;
};
#endif

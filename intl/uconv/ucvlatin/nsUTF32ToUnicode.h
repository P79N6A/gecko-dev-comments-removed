







































#ifndef nsUTF32ToUnicode_h___
#define nsUTF32ToUnicode_h___











class nsUTF32ToUnicode : public nsBasicDecoderSupport
{

public:

  


  nsUTF32ToUnicode();

protected:

  
  PRUint16 mState;  
  
  PRUint8  mBufferInc[4];

  
  

  NS_IMETHOD GetMaxLength(const char * aSrc, PRInt32 aSrcLength, 
                          PRInt32 * aDestLength);

  NS_IMETHOD Reset();

};











class nsUTF32BEToUnicode : public nsUTF32ToUnicode
{
public:


  
  

  NS_IMETHOD Convert(const char * aSrc, PRInt32 * aSrcLength, 
                     PRUnichar * aDest, PRInt32 * aDestLength);


};











class nsUTF32LEToUnicode : public nsUTF32ToUnicode
{
public:


  
  

  NS_IMETHOD Convert(const char * aSrc, PRInt32 * aSrcLength, 
                     PRUnichar * aDest, PRInt32 * aDestLength);

};

#endif 


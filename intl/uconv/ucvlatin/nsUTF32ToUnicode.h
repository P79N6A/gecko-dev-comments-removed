







































#ifndef nsUTF32ToUnicode_h___
#define nsUTF32ToUnicode_h___











class nsUTF32ToUnicodeBase : public nsBasicDecoderSupport
{

protected:

  


  nsUTF32ToUnicodeBase();

  
  PRUint16 mState;
  
  PRUint8  mBufferInc[4];

  
  

  NS_IMETHOD GetMaxLength(const char * aSrc, PRInt32 aSrcLength, 
                          PRInt32 * aDestLength);

  NS_IMETHOD Reset();

};











class nsUTF32BEToUnicode : public nsUTF32ToUnicodeBase
{
public:


  
  

  NS_IMETHOD Convert(const char * aSrc, PRInt32 * aSrcLength, 
                     PRUnichar * aDest, PRInt32 * aDestLength);


};











class nsUTF32LEToUnicode : public nsUTF32ToUnicodeBase
{
public:


  
  

  NS_IMETHOD Convert(const char * aSrc, PRInt32 * aSrcLength, 
                     PRUnichar * aDest, PRInt32 * aDestLength);

};











class nsUTF32ToUnicode : public nsUTF32ToUnicodeBase
{
public:

  


  nsUTF32ToUnicode() { Reset(); }

  
  

  NS_IMETHOD Convert(const char * aSrc, PRInt32 * aSrcLength, 
                     PRUnichar * aDest, PRInt32 * aDestLength);

  
  

  NS_IMETHOD Reset();

private:

  enum Endian {kUnknown, kBigEndian, kLittleEndian};
  Endian  mEndian; 
  PRBool  mFoundBOM;
};

#endif


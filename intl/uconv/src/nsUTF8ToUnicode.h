




































#ifndef nsUTF8ToUnicode_h___
#define nsUTF8ToUnicode_h___



#define NS_UTF8TOUNICODE_CID \
  { 0x5534ddc0, 0xdd96, 0x11d2, {0x8a, 0xac, 0x0, 0x60, 0x8, 0x11, 0xa8, 0x36}}

#define NS_UTF8TOUNICODE_CONTRACTID "@mozilla.org/intl/unicode/decoder;1?charset=UTF-8"
















class nsUTF8ToUnicode : public nsBasicDecoderSupport
{
public:

  


  nsUTF8ToUnicode();

protected:

  PRUint32 mUcs4; 
  PRUint8 mState; 
  PRUint8 mBytes;
  PRPackedBool mFirst;

  
  

  NS_IMETHOD GetMaxLength(const char * aSrc, PRInt32 aSrcLength, 
      PRInt32 * aDestLength);

  
  

  NS_IMETHOD Convert(const char * aSrc, PRInt32 * aSrcLength, 
      PRUnichar * aDest, PRInt32 * aDestLength);

  
  

  NS_IMETHOD Reset();

};

#endif 


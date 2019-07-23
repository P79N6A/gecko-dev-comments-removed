




































#ifndef nsUnicodeToUTF8_h___
#define nsUnicodeToUTF8_h___



#define NS_UNICODETOUTF8_CID \
  { 0x7c657d18, 0xec5e, 0x11d2, {0x8a, 0xac, 0x0, 0x60, 0x8, 0x11, 0xa8, 0x36}}

#define NS_UNICODETOUTF8_CONTRACTID "@mozilla.org/intl/unicode/encoder;1?charset=UTF-8"













class nsUnicodeToUTF8 : public nsIUnicodeEncoder
{
  NS_DECL_ISUPPORTS

public:

  


  nsUnicodeToUTF8() {mHighSurrogate = 0;};

  NS_IMETHOD FillInfo(PRUint32* aInfo);

  NS_IMETHOD Convert(const PRUnichar * aSrc, 
                     PRInt32 * aSrcLength, 
                     char * aDest, 
                     PRInt32 * aDestLength);

  NS_IMETHOD Finish(char * aDest, PRInt32 * aDestLength);

  NS_IMETHOD GetMaxLength(const PRUnichar * aSrc, PRInt32 aSrcLength, 
      PRInt32 * aDestLength);

  NS_IMETHOD Reset() {mHighSurrogate = 0; return NS_OK;}

  NS_IMETHOD SetOutputErrorBehavior(PRInt32 aBehavior, 
    nsIUnicharEncoder * aEncoder, PRUnichar aChar) {return NS_OK;};

protected:
  PRUnichar mHighSurrogate;

};

#endif 

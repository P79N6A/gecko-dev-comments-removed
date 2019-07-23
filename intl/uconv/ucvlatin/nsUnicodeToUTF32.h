







































#ifndef nsUnicodeToUTF32_h___
#define nsUnicodeToUTF32_h___











class nsUnicodeToUTF32Base : public nsIUnicodeEncoder
{
   NS_DECL_ISUPPORTS

protected:

 


  nsUnicodeToUTF32Base() {mBOM = 0; mHighSurrogate = 0;}
  virtual ~nsUnicodeToUTF32Base() {}

  PRUnichar  mHighSurrogate;

  NS_IMETHOD GetMaxLength(const PRUnichar * aSrc, PRInt32 aSrcLength, 
                          PRInt32 * aDestLength);

  
  

  NS_IMETHOD Reset() {mBOM = 0; mHighSurrogate = 0; return NS_OK;}
  NS_IMETHOD FillInfo(PRUint32* aInfo);
  NS_IMETHOD SetOutputErrorBehavior(PRInt32 aBehavior, 
                                    nsIUnicharEncoder * aEncoder, 
                                    PRUnichar aChar) 
                                    {return NS_OK;}

protected:
  PRUnichar mBOM;
};











class nsUnicodeToUTF32BE : public nsUnicodeToUTF32Base
{
public:

  
  

  NS_IMETHOD Convert(const PRUnichar * aSrc, PRInt32 * aSrcLength, 
                     char * aDest, PRInt32 * aDestLength);
  NS_IMETHOD Finish(char * aDest, PRInt32 * aDestLength);


};











class nsUnicodeToUTF32LE : public nsUnicodeToUTF32Base
{
public:

  
  
  NS_IMETHOD Convert(const PRUnichar * aSrc, PRInt32 * aSrcLength, 
                     char * aDest, PRInt32 * aDestLength);
  NS_IMETHOD Finish(char * aDest, PRInt32 * aDestLength);

};










#ifdef IS_LITTLE_ENDIAN
class nsUnicodeToUTF32 : public nsUnicodeToUTF32LE
#elif defined(IS_BIG_ENDIAN)
class nsUnicodeToUTF32 : public nsUnicodeToUTF32BE
#else
#error "Unknown endianness"
#endif
{
public:
  nsUnicodeToUTF32() {mBOM = 0xFEFF; mHighSurrogate = 0;};

  
  
  NS_IMETHOD Reset() {mBOM = 0xFEFF; mHighSurrogate = 0; return NS_OK;};

};

#endif 


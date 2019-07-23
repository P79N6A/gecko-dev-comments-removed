







































#ifndef nsUnicodeToUTF32_h___
#define nsUnicodeToUTF32_h___











class nsUnicodeToUTF32 : public nsIUnicodeEncoder
{
   NS_DECL_ISUPPORTS

public:

 


  nsUnicodeToUTF32() {mHighSurrogate = 0;}
  virtual ~nsUnicodeToUTF32() {}

protected:
  PRUnichar  mHighSurrogate;

  NS_IMETHOD GetMaxLength(const PRUnichar * aSrc, PRInt32 aSrcLength, 
                          PRInt32 * aDestLength);

  
  

  NS_IMETHOD Reset() {mHighSurrogate = 0; return NS_OK;}
  NS_IMETHOD FillInfo(PRUint32* aInfo);
  NS_IMETHOD SetOutputErrorBehavior(PRInt32 aBehavior, 
                                    nsIUnicharEncoder * aEncoder, 
                                    PRUnichar aChar) 
                                    {return NS_OK;}

};











class nsUnicodeToUTF32BE : public nsUnicodeToUTF32
{
public:

  
  

  NS_IMETHOD Convert(const PRUnichar * aSrc, PRInt32 * aSrcLength, 
                     char * aDest, PRInt32 * aDestLength);
  NS_IMETHOD Finish(char * aDest, PRInt32 * aDestLength);


};











class nsUnicodeToUTF32LE : public nsUnicodeToUTF32
{
public:

  
  
  NS_IMETHOD Convert(const PRUnichar * aSrc, PRInt32 * aSrcLength, 
                     char * aDest, PRInt32 * aDestLength);
  NS_IMETHOD Finish(char * aDest, PRInt32 * aDestLength);

};

#endif 


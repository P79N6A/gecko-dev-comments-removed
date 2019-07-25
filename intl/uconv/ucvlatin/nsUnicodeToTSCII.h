






#ifndef nsUnicodeToTSCII_h___
#define nsUnicodeToTSCII_h___

#include "nsCOMPtr.h"
#include "nsISupports.h"
#include "nsIUnicodeEncoder.h"




class nsUnicodeToTSCII : public nsIUnicodeEncoder
{

NS_DECL_ISUPPORTS

public:
  nsUnicodeToTSCII() { mBuffer = 0; }
  virtual ~nsUnicodeToTSCII() {}

  NS_IMETHOD Convert(const PRUnichar * aSrc, int32_t * aSrcLength,
                     char * aDest, int32_t * aDestLength);

  NS_IMETHOD Finish(char * aDest, int32_t * aDestLength);

  NS_IMETHOD GetMaxLength(const PRUnichar * aSrc, int32_t aSrcLength,
                          int32_t * aDestLength);

  NS_IMETHOD Reset();

  NS_IMETHOD SetOutputErrorBehavior(int32_t aBehavior,
                                    nsIUnicharEncoder * aEncoder, 
                                    PRUnichar aChar);

private:
  uint32_t mBuffer; 
                    
                    
};

#define CHAR_BUFFER_SIZE 2048




class nsUnicodeToTamilTTF : public nsUnicodeToTSCII
{
  NS_DECL_ISUPPORTS_INHERITED

public:
  nsUnicodeToTamilTTF() : nsUnicodeToTSCII() {}
  virtual ~nsUnicodeToTamilTTF() {}

  NS_IMETHOD Convert      (const PRUnichar * aSrc, int32_t * aSrcLength,
                           char * aDest, int32_t * aDestLength);
  NS_IMETHOD GetMaxLength (const PRUnichar * aSrc, int32_t  aSrcLength,
                           int32_t * aDestLength);

  NS_IMETHOD SetOutputErrorBehavior (int32_t aBehavior, 
                                     nsIUnicharEncoder *aEncoder, 
                                     PRUnichar aChar);

private:
  char mStaticBuffer[CHAR_BUFFER_SIZE];
  int32_t mErrBehavior;
  PRUnichar mErrChar;
  nsCOMPtr<nsIUnicharEncoder> mErrEncoder;
};

#endif 

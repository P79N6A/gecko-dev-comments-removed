




#ifndef nsUnicodeToUCS2BE_h___
#define nsUnicodeToUCS2BE_h___

#include "nsUCSupport.h"

class nsUnicodeToUTF16BE: public nsBasicEncoder
{
public:
  nsUnicodeToUTF16BE() { mBOM = 0;}

  
  

  NS_IMETHOD Convert(const PRUnichar * aSrc, int32_t * aSrcLength, 
      char * aDest, int32_t * aDestLength);
  NS_IMETHOD GetMaxLength(const PRUnichar * aSrc, int32_t aSrcLength, 
      int32_t * aDestLength);
  NS_IMETHOD Finish(char * aDest, int32_t * aDestLength);
  NS_IMETHOD Reset();
  NS_IMETHOD SetOutputErrorBehavior(int32_t aBehavior, 
      nsIUnicharEncoder * aEncoder, PRUnichar aChar);

protected:
  PRUnichar mBOM;
  NS_IMETHOD CopyData(char* aDest, const PRUnichar* aSrc, int32_t aLen  );
};

class nsUnicodeToUTF16LE: public nsUnicodeToUTF16BE
{
public:
  nsUnicodeToUTF16LE() { mBOM = 0;}

protected:
  NS_IMETHOD CopyData(char* aDest, const PRUnichar* aSrc, int32_t aLen  );
};



#ifdef IS_LITTLE_ENDIAN
class nsUnicodeToUTF16: public nsUnicodeToUTF16LE
#elif defined(IS_BIG_ENDIAN)
class nsUnicodeToUTF16: public nsUnicodeToUTF16BE
#else
#error "Unknown endianness"
#endif
{
public:
  nsUnicodeToUTF16() { mBOM = 0xFEFF;}
};

#endif 

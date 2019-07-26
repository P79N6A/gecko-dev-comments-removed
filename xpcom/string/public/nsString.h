





#ifndef nsString_h___
#define nsString_h___

#include "mozilla/Attributes.h"

#ifndef nsSubstring_h___
#include "nsSubstring.h"
#endif

#ifndef nsDependentSubstring_h___
#include "nsDependentSubstring.h"
#endif

#ifndef nsReadableUtils_h___
#include "nsReadableUtils.h"
#endif


#include "prtypes.h"

#include NEW_H

  
#ifndef MOZ_STRING_WITH_OBSOLETE_API
#define MOZ_STRING_WITH_OBSOLETE_API 1
#endif

#if MOZ_STRING_WITH_OBSOLETE_API
  
#define kRadix10        (10)
#define kRadix16        (16)
#define kAutoDetect     (100)
#define kRadixUnknown   (kAutoDetect+1)
#define IGNORE_CASE     (true)
#endif


  
#include "string-template-def-unichar.h"
#include "nsTString.h"
#include "string-template-undef.h"

  
#include "string-template-def-char.h"
#include "nsTString.h"
#include "string-template-undef.h"

PR_STATIC_ASSERT(sizeof(PRUnichar) == 2);
PR_STATIC_ASSERT(sizeof(nsString::char_type) == 2);
PR_STATIC_ASSERT(sizeof(nsCString::char_type) == 1);

  


class NS_LossyConvertUTF16toASCII : public nsAutoCString
  {
    public:
      explicit
      NS_LossyConvertUTF16toASCII( const PRUnichar* aString )
        {
          LossyAppendUTF16toASCII(aString, *this);
        }

      NS_LossyConvertUTF16toASCII( const PRUnichar* aString, uint32_t aLength )
        {
          LossyAppendUTF16toASCII(Substring(aString, aLength), *this);
        }

      explicit
      NS_LossyConvertUTF16toASCII( const nsAString& aString )
        {
          LossyAppendUTF16toASCII(aString, *this);
        }

    private:
        
      NS_LossyConvertUTF16toASCII( char );
  };


class NS_ConvertASCIItoUTF16 : public nsAutoString
  {
    public:
      explicit
      NS_ConvertASCIItoUTF16( const char* aCString )
        {
          AppendASCIItoUTF16(aCString, *this);
        }

      NS_ConvertASCIItoUTF16( const char* aCString, uint32_t aLength )
        {
          AppendASCIItoUTF16(Substring(aCString, aLength), *this);
        }

      explicit
      NS_ConvertASCIItoUTF16( const nsACString& aCString )
        {
          AppendASCIItoUTF16(aCString, *this);
        }

    private:
        
      NS_ConvertASCIItoUTF16( PRUnichar );
  };


  


class NS_ConvertUTF16toUTF8 : public nsAutoCString
  {
    public:
      explicit
      NS_ConvertUTF16toUTF8( const PRUnichar* aString )
        {
          AppendUTF16toUTF8(aString, *this);
        }

      NS_ConvertUTF16toUTF8( const PRUnichar* aString, uint32_t aLength )
        {
          AppendUTF16toUTF8(Substring(aString, aLength), *this);
        }

      explicit
      NS_ConvertUTF16toUTF8( const nsAString& aString )
        {
          AppendUTF16toUTF8(aString, *this);
        }

    private:
        
      NS_ConvertUTF16toUTF8( char );
  };


class NS_ConvertUTF8toUTF16 : public nsAutoString
  {
    public:
      explicit
      NS_ConvertUTF8toUTF16( const char* aCString )
        {
          AppendUTF8toUTF16(aCString, *this);
        }

      NS_ConvertUTF8toUTF16( const char* aCString, uint32_t aLength )
        {
          AppendUTF8toUTF16(Substring(aCString, aLength), *this);
        }

      explicit
      NS_ConvertUTF8toUTF16( const nsACString& aCString )
        {
          AppendUTF8toUTF16(aCString, *this);
        }

    private:
        
      NS_ConvertUTF8toUTF16( PRUnichar );
  };



typedef nsAutoString nsVoidableString;

#ifndef nsDependentString_h___
#include "nsDependentString.h"
#endif

#ifndef nsLiteralString_h___
#include "nsLiteralString.h"
#endif

#ifndef nsPromiseFlatString_h___
#include "nsPromiseFlatString.h"
#endif


#include "nsMemory.h"
#include <string.h>
#include <stdio.h>
#include "plhash.h"

inline int32_t MinInt(int32_t x, int32_t y)
  {
    return XPCOM_MIN(x, y);
  }

inline int32_t MaxInt(int32_t x, int32_t y)
  {
    return XPCOM_MAX(x, y);
  }






inline void Recycle( char* aBuffer) { nsMemory::Free(aBuffer); }
inline void Recycle( PRUnichar* aBuffer) { nsMemory::Free(aBuffer); }

#endif 

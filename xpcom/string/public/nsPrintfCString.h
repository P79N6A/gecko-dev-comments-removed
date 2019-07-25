




































#ifndef nsPrintfCString_h___
#define nsPrintfCString_h___
 
#ifndef nsString_h___
#include "nsString.h"
#endif








class nsPrintfCString : public nsFixedCString
  {
    typedef nsCString string_type;

    public:
      explicit nsPrintfCString( const char_type* format, ... )
        : nsFixedCString(mLocalBuffer, kLocalBufferSize, 0)
        {
          va_list ap;
          va_start(ap, format);
          AppendPrintf(format, ap);
          va_end(ap);
        }

      
      
      nsPrintfCString( size_type n, const char_type* format, ...)
        : nsFixedCString(mLocalBuffer, kLocalBufferSize, 0)
        {
          va_list ap;
          va_start(ap, format);
          AppendPrintf(format, ap);
          va_end(ap);
        }

    private:
      enum { kLocalBufferSize=15 };
      
      

      char_type  mLocalBuffer[ kLocalBufferSize ];
  };

#endif 






































#ifndef nsPrintfCString_h___
#define nsPrintfCString_h___
 
#ifndef nsString_h___
#include "nsString.h"
#endif


  























class NS_COM nsPrintfCString : public nsCString
  {
    typedef nsCString string_type;

    enum { kLocalBufferSize=15 };
      
      

    public:
      
      explicit nsPrintfCString( const char_type* format, ... );
      nsPrintfCString( size_type n, const char_type* format, ...);

    private:
      char_type  mLocalBuffer[ kLocalBufferSize + 1 ];
  };

#endif 

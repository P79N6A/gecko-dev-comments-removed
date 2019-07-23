





































#include "nsPrintfCString.h"
#include <stdarg.h>
#include "prprf.h"









nsPrintfCString::nsPrintfCString( const char_type* format, ... )
  : string_type(mLocalBuffer, 0, F_TERMINATED)
  {
    va_list ap;

    size_type logical_capacity = kLocalBufferSize;
    size_type physical_capacity = logical_capacity + 1;

    va_start(ap, format);
    mLength = PR_vsnprintf(mData, physical_capacity, format, ap);
    va_end(ap);
  }

nsPrintfCString::nsPrintfCString( size_type n, const char_type* format, ... )
  : string_type(mLocalBuffer, 0, F_TERMINATED)
  {
    va_list ap;

      
    size_type logical_capacity = kLocalBufferSize;
    if ( n > logical_capacity )
      {
        if (!SetCapacity(n))
          return; 
        logical_capacity = n;
      }
    size_type physical_capacity = logical_capacity + 1;

    va_start(ap, format);
    mLength = PR_vsnprintf(mData, physical_capacity, format, ap);
    va_end(ap);
  }

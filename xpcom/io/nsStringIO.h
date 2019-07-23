





































#ifndef nsStringIO_h___
#define nsStringIO_h___

#include "nsAString.h"
#include <stdio.h>


template <class CharT>
class nsFileCharSink
  {
    public:
      typedef CharT value_type;

    public:
      nsFileCharSink( FILE* aOutputFile ) : mOutputFile(aOutputFile) { }

      PRUint32
      write( const value_type* s, PRUint32 n )
        {
          return fwrite(s, sizeof(CharT), n, mOutputFile);
        }

    private:
      FILE* mOutputFile;
  };


template <class CharT>
inline
void
fprint_string( FILE* aFile, const basic_nsAString<CharT>& aString )
  {
    nsReadingIterator<CharT> fromBegin, fromEnd;
    nsFileCharSink<CharT> toBegin(aFile);
    copy_string(aString.BeginReading(fromBegin), aString.EndReading(fromEnd), toBegin);
  }


template <class CharT>
inline
void
print_string( const basic_nsAString<CharT>& aString )
  {
    fprint_string(stdout, aString);
  }


#endif 

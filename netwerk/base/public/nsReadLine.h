







































#ifndef nsReadLine_h__
#define nsReadLine_h__

#include "prmem.h"
#include "nsIInputStream.h"





















#define kLineBufferSize 4096







template<typename CharT>
class nsLineBuffer {
  public:
  CharT buf[kLineBufferSize+1];
  CharT* start;
  CharT* end;
};






















template<typename CharT>
nsresult
NS_InitLineBuffer (nsLineBuffer<CharT> ** aBufferPtr) {
  *aBufferPtr = PR_NEW(nsLineBuffer<CharT>);
  if (!(*aBufferPtr))
    return NS_ERROR_OUT_OF_MEMORY;

  (*aBufferPtr)->start = (*aBufferPtr)->end = (*aBufferPtr)->buf;
  return NS_OK;
}
























template<typename CharT, class StreamType, class StringType>
nsresult
NS_ReadLine (StreamType* aStream, nsLineBuffer<CharT> * aBuffer,
             StringType & aLine, bool *more)
{
  CharT eolchar = 0; 

  aLine.Truncate();

  while (1) { 
    if (aBuffer->start == aBuffer->end) { 
      PRUint32 bytesRead;
      nsresult rv = aStream->Read(aBuffer->buf, kLineBufferSize, &bytesRead);
      if (NS_FAILED(rv) || NS_UNLIKELY(bytesRead == 0)) {
        *more = PR_FALSE;
        return rv;
      }
      aBuffer->start = aBuffer->buf;
      aBuffer->end = aBuffer->buf + bytesRead;
      *(aBuffer->end) = '\0';
    }

    









    CharT* current = aBuffer->start;
    if (NS_LIKELY(eolchar == 0)) {
      for ( ; current < aBuffer->end; ++current) {
        if (*current == '\n' || *current == '\r') {
          eolchar = *current;
          *current++ = '\0';
          aLine.Append(aBuffer->start);
          break;
        }
      }
    }
    if (NS_LIKELY(eolchar != 0)) {
      for ( ; current < aBuffer->end; ++current) {
        if ((eolchar == '\r' && *current == '\n') ||
            (eolchar == '\n' && *current == '\r')) {
          eolchar = 1;
          continue;
        }
        aBuffer->start = current;
        *more = PR_TRUE;
        return NS_OK;
      }
    }

    if (eolchar == 0)
      aLine.Append(aBuffer->start);
    aBuffer->start = aBuffer->end; 
  }
}

#endif 

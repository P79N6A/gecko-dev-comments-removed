





#ifndef nsReadLine_h__
#define nsReadLine_h__

#include "nsIInputStream.h"
#include "mozilla/Likely.h"




















#define kLineBufferSize 4096







template<typename CharT>
class nsLineBuffer {
  public:
    nsLineBuffer() : start(buf), end(buf) { }

  CharT buf[kLineBufferSize+1];
  CharT* start;
  CharT* end;
};























template<typename CharT, class StreamType, class StringType>
nsresult
NS_ReadLine (StreamType* aStream, nsLineBuffer<CharT> * aBuffer,
             StringType & aLine, bool *more)
{
  CharT eolchar = 0; 

  aLine.Truncate();

  while (1) { 
    if (aBuffer->start == aBuffer->end) { 
      uint32_t bytesRead;
      nsresult rv = aStream->Read(aBuffer->buf, kLineBufferSize, &bytesRead);
      if (NS_FAILED(rv) || MOZ_UNLIKELY(bytesRead == 0)) {
        *more = false;
        return rv;
      }
      aBuffer->start = aBuffer->buf;
      aBuffer->end = aBuffer->buf + bytesRead;
      *(aBuffer->end) = '\0';
    }

    









    CharT* current = aBuffer->start;
    if (MOZ_LIKELY(eolchar == 0)) {
      for ( ; current < aBuffer->end; ++current) {
        if (*current == '\n' || *current == '\r') {
          eolchar = *current;
          *current++ = '\0';
          aLine.Append(aBuffer->start);
          break;
        }
      }
    }
    if (MOZ_LIKELY(eolchar != 0)) {
      for ( ; current < aBuffer->end; ++current) {
        if ((eolchar == '\r' && *current == '\n') ||
            (eolchar == '\n' && *current == '\r')) {
          eolchar = 1;
          continue;
        }
        aBuffer->start = current;
        *more = true;
        return NS_OK;
      }
    }

    if (eolchar == 0)
      aLine.Append(aBuffer->start);
    aBuffer->start = aBuffer->end; 
  }
}

#endif 

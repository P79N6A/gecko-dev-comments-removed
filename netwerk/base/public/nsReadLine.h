






































#ifndef nsReadLine_h__
#define nsReadLine_h__

#include "prmem.h"
#include "nsIInputStream.h"



















#define kLineBufferSize 1024





template<typename CharT>
class nsLineBuffer {
  public:
  CharT buf[kLineBufferSize+1];
  CharT* start;
  CharT* current;
  CharT* end;
  PRBool empty;
};






















template<typename CharT>
nsresult
NS_InitLineBuffer (nsLineBuffer<CharT> ** aBufferPtr) {
  *aBufferPtr = PR_NEW(nsLineBuffer<CharT>);
  if (!(*aBufferPtr))
    return NS_ERROR_OUT_OF_MEMORY;

  (*aBufferPtr)->start = (*aBufferPtr)->current = (*aBufferPtr)->end = (*aBufferPtr)->buf;
  (*aBufferPtr)->empty = PR_TRUE;
  return NS_OK;
}
























template<typename CharT, class StreamType, class StringType>
nsresult
NS_ReadLine (StreamType* aStream, nsLineBuffer<CharT> * aBuffer,
             StringType & aLine, PRBool *more) {
  nsresult rv = NS_OK;
  PRUint32 bytesRead;
  *more = PR_TRUE;
  PRBool eolStarted = PR_FALSE;
  CharT eolchar = '\0';
  aLine.Truncate();
  while (1) { 
    if (aBuffer->empty) { 
      rv = aStream->Read(aBuffer->buf, kLineBufferSize, &bytesRead);
      if (NS_FAILED(rv)) 
        return rv;
      if (bytesRead == 0) { 
        *more = PR_FALSE;
        return NS_OK;
      }
      aBuffer->end = aBuffer->buf + bytesRead;
      aBuffer->empty = PR_FALSE;
      *(aBuffer->end) = '\0'; 
    }
    
    while (aBuffer->current < aBuffer->end) {
      if (eolStarted) {
          if ((eolchar == '\n' && *(aBuffer->current) == '\r') ||
              (eolchar == '\r' && *(aBuffer->current) == '\n')) { 
            (aBuffer->current)++;
            aBuffer->start = aBuffer->current;
          }
          eolStarted = PR_FALSE;
          return NS_OK;
      } else if (*(aBuffer->current) == '\n' ||
                 *(aBuffer->current) == '\r') { 
        eolStarted = PR_TRUE;
        eolchar = *(aBuffer->current);
        *(aBuffer->current) = '\0';
        aLine.Append(aBuffer->start);
        (aBuffer->current)++;
        aBuffer->start = aBuffer->current;
      } else {
        eolStarted = PR_FALSE;
        (aBuffer->current)++;
      }
    }

    
    aLine.Append(aBuffer->start);

    
    aBuffer->current = aBuffer->start = aBuffer->buf;
    aBuffer->empty = PR_TRUE;
    
    if (eolStarted) {  
      rv = aStream->Read(aBuffer->buf, 1, &bytesRead);
      if (NS_FAILED(rv)) 
        return rv;
      if (bytesRead == 0) { 
        *more = PR_FALSE;
        return NS_OK;
      }
      if ((eolchar == '\n' && *(aBuffer->buf) == '\r') ||
          (eolchar == '\r' && *(aBuffer->buf) == '\n')) {
        
        return NS_OK;
      } else {
        
        aBuffer->empty = PR_FALSE;
        aBuffer->end = aBuffer->buf + 1;
        *(aBuffer->end) = '\0';
      }
    }
  }
}

#endif 







































#include "Base64.h"

#include "nsIInputStream.h"

namespace {


const unsigned char *base = (unsigned char *)"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

template <typename T>
static void
Encode3to4(const unsigned char *src, T *dest)
{
    PRUint32 b32 = (PRUint32)0;
    PRIntn i, j = 18;

    for( i = 0; i < 3; i++ )
    {
        b32 <<= 8;
        b32 |= (PRUint32)src[i];
    }

    for( i = 0; i < 4; i++ )
    {
        dest[i] = base[ (PRUint32)((b32>>j) & 0x3F) ];
        j -= 6;
    }
}

template <typename T>
static void
Encode2to4(const unsigned char *src, T *dest)
{
    dest[0] = base[ (PRUint32)((src[0]>>2) & 0x3F) ];
    dest[1] = base[ (PRUint32)(((src[0] & 0x03) << 4) | ((src[1] >> 4) & 0x0F)) ];
    dest[2] = base[ (PRUint32)((src[1] & 0x0F) << 2) ];
    dest[3] = (unsigned char)'=';
}

template <typename T>
static void
Encode1to4(const unsigned char *src, T *dest)
{
    dest[0] = base[ (PRUint32)((src[0]>>2) & 0x3F) ];
    dest[1] = base[ (PRUint32)((src[0] & 0x03) << 4) ];
    dest[2] = (unsigned char)'=';
    dest[3] = (unsigned char)'=';
}

template <typename T>
static void
Encode(const unsigned char *src, PRUint32 srclen, T *dest)
{
    while( srclen >= 3 )
    {
        Encode3to4(src, dest);
        src += 3;
        dest += 4;
        srclen -= 3;
    }

    switch( srclen )
    {
        case 2:
            Encode2to4(src, dest);
            break;
        case 1:
            Encode1to4(src, dest);
            break;
        case 0:
            break;
        default:
            NS_NOTREACHED("coding error");
    }
}



template <typename T>
struct EncodeInputStream_State {
  unsigned char c[3];
  PRUint8 charsOnStack;
  typename T::char_type* buffer;
};

template <typename T>
NS_METHOD
EncodeInputStream_Encoder(nsIInputStream *aStream,
                          void *aClosure,
                          const char *aFromSegment,
                          PRUint32 aToOffset,
                          PRUint32 aCount,
                          PRUint32 *aWriteCount)
{
  NS_ASSERTION(aCount > 0, "Er, what?");

  EncodeInputStream_State<T>* state =
    static_cast<EncodeInputStream_State<T>*>(aClosure);

  
  PRUint32 countRemaining = aCount;
  const unsigned char *src = (const unsigned char*)aFromSegment;
  if (state->charsOnStack) {
    unsigned char firstSet[4];
    if (state->charsOnStack == 1) {
      firstSet[0] = state->c[0];
      firstSet[1] = src[0];
      firstSet[2] = (countRemaining > 1) ? src[1] : '\0';
      firstSet[3] = '\0';
    } else  {
      firstSet[0] = state->c[0];
      firstSet[1] = state->c[1];
      firstSet[2] = src[0];
      firstSet[3] = '\0';
    }
    Encode(firstSet, 3, state->buffer);
    state->buffer += 4;
    countRemaining -= (3 - state->charsOnStack);
    src += (3 - state->charsOnStack);
    state->charsOnStack = 0;
  }

  
  PRUint32 encodeLength = countRemaining - countRemaining % 3;
  NS_ABORT_IF_FALSE(encodeLength % 3 == 0,
                    "Should have an exact number of triplets!");
  Encode(src, encodeLength, state->buffer);
  state->buffer += (encodeLength / 3) * 4;
  src += encodeLength;
  countRemaining -= encodeLength;

  
  *aWriteCount = aCount;

  if (countRemaining) {
    
    NS_ABORT_IF_FALSE(countRemaining < 3, "We should have encoded more!");
    state->c[0] = src[0];
    state->c[1] = (countRemaining == 2) ? src[1] : '\0';
    state->charsOnStack = countRemaining;
  }

  return NS_OK;
}

template <typename T>
nsresult
EncodeInputStream(nsIInputStream *aInputStream, 
                  T &aDest,
                  PRUint32 aCount,
                  PRUint32 aOffset)
{
  nsresult rv;

  if (!aCount) {
    rv = aInputStream->Available(&aCount);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRUint32 count = (aCount + 2) / 3 * 4; 
  aDest.SetLength(count + aOffset);
  if (aDest.Length() != count + aOffset)
    return NS_ERROR_OUT_OF_MEMORY;

  EncodeInputStream_State<T> state;
  state.charsOnStack = 0;
  state.c[2] = '\0';
  state.buffer = aOffset + aDest.BeginWriting();

  while (1) {
    PRUint32 read = 0;

    rv = aInputStream->ReadSegments(&EncodeInputStream_Encoder<T>,
                                    (void*)&state,
                                    aCount,
                                    &read);
    if (NS_FAILED(rv)) {
      if (rv == NS_BASE_STREAM_WOULD_BLOCK)
        NS_RUNTIMEABORT("Not implemented for async streams!");
      if (rv == NS_ERROR_NOT_IMPLEMENTED)
        NS_RUNTIMEABORT("Requires a stream that implements ReadSegments!");
      return rv;
    }

    if (!read)
      break;
  }

  
  if (state.charsOnStack)
    Encode(state.c, state.charsOnStack, state.buffer);

  *aDest.EndWriting() = '\0';

  return NS_OK;
}

} 

namespace mozilla {

nsresult
Base64EncodeInputStream(nsIInputStream *aInputStream, 
                        nsACString &aDest,
                        PRUint32 aCount,
                        PRUint32 aOffset)
{
  return EncodeInputStream<nsACString>(aInputStream, aDest, aCount, aOffset);
}

nsresult
Base64EncodeInputStream(nsIInputStream *aInputStream, 
                        nsAString &aDest,
                        PRUint32 aCount,
                        PRUint32 aOffset)
{
  return EncodeInputStream<nsAString>(aInputStream, aDest, aCount, aOffset);
}

} 

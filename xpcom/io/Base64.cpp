




#include "Base64.h"

#include "nsIInputStream.h"
#include "nsStringGlue.h"

#include "plbase64.h"

namespace {


const unsigned char *base = (unsigned char *)"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

template <typename T>
static void
Encode3to4(const unsigned char *src, T *dest)
{
    uint32_t b32 = (uint32_t)0;
    int i, j = 18;

    for( i = 0; i < 3; i++ )
    {
        b32 <<= 8;
        b32 |= (uint32_t)src[i];
    }

    for( i = 0; i < 4; i++ )
    {
        dest[i] = base[ (uint32_t)((b32>>j) & 0x3F) ];
        j -= 6;
    }
}

template <typename T>
static void
Encode2to4(const unsigned char *src, T *dest)
{
    dest[0] = base[ (uint32_t)((src[0]>>2) & 0x3F) ];
    dest[1] = base[ (uint32_t)(((src[0] & 0x03) << 4) | ((src[1] >> 4) & 0x0F)) ];
    dest[2] = base[ (uint32_t)((src[1] & 0x0F) << 2) ];
    dest[3] = (unsigned char)'=';
}

template <typename T>
static void
Encode1to4(const unsigned char *src, T *dest)
{
    dest[0] = base[ (uint32_t)((src[0]>>2) & 0x3F) ];
    dest[1] = base[ (uint32_t)((src[0] & 0x03) << 4) ];
    dest[2] = (unsigned char)'=';
    dest[3] = (unsigned char)'=';
}

template <typename T>
static void
Encode(const unsigned char *src, uint32_t srclen, T *dest)
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
  uint8_t charsOnStack;
  typename T::char_type* buffer;
};

template <typename T>
NS_METHOD
EncodeInputStream_Encoder(nsIInputStream *aStream,
                          void *aClosure,
                          const char *aFromSegment,
                          uint32_t aToOffset,
                          uint32_t aCount,
                          uint32_t *aWriteCount)
{
  NS_ASSERTION(aCount > 0, "Er, what?");

  EncodeInputStream_State<T>* state =
    static_cast<EncodeInputStream_State<T>*>(aClosure);

  
  uint32_t countRemaining = aCount;
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

  
  uint32_t encodeLength = countRemaining - countRemaining % 3;
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
                  uint32_t aCount,
                  uint32_t aOffset)
{
  nsresult rv;
  uint64_t count64 = aCount;

  if (!aCount) {
    rv = aInputStream->Available(&count64);
    NS_ENSURE_SUCCESS(rv, rv);
    
    
    aCount = (uint32_t)count64;
  }

  uint64_t countlong =
    (count64 + 2) / 3 * 4; 
  if (countlong + aOffset > PR_UINT32_MAX)
    return NS_ERROR_OUT_OF_MEMORY;

  uint32_t count = uint32_t(countlong);

  aDest.SetLength(count + aOffset);
  if (aDest.Length() != count + aOffset)
    return NS_ERROR_OUT_OF_MEMORY;

  EncodeInputStream_State<T> state;
  state.charsOnStack = 0;
  state.c[2] = '\0';
  state.buffer = aOffset + aDest.BeginWriting();

  while (1) {
    uint32_t read = 0;

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
                        uint32_t aCount,
                        uint32_t aOffset)
{
  return EncodeInputStream<nsACString>(aInputStream, aDest, aCount, aOffset);
}

nsresult
Base64EncodeInputStream(nsIInputStream *aInputStream, 
                        nsAString &aDest,
                        uint32_t aCount,
                        uint32_t aOffset)
{
  return EncodeInputStream<nsAString>(aInputStream, aDest, aCount, aOffset);
}

nsresult
Base64Encode(const nsACString &aBinaryData, nsACString &aString)
{
  
  if (aBinaryData.Length() > (PR_UINT32_MAX / 4) * 3) {
    return NS_ERROR_FAILURE;
  }

  uint32_t stringLen = ((aBinaryData.Length() + 2) / 3) * 4;

  char *buffer;

  
  if (aString.SetCapacity(stringLen + 1, fallible_t()) &&
    (buffer = aString.BeginWriting()) &&
    PL_Base64Encode(aBinaryData.BeginReading(), aBinaryData.Length(), buffer)) {
    
    
    buffer[stringLen] = '\0';

    aString.SetLength(stringLen);
    return NS_OK;
  }

  aString.Truncate();
  return NS_ERROR_INVALID_ARG;
}

nsresult
Base64Encode(const nsAString &aString, nsAString &aBinaryData)
{
  NS_LossyConvertUTF16toASCII string(aString);
  nsCAutoString binaryData;

  nsresult rv = Base64Encode(string, binaryData);
  if (NS_SUCCEEDED(rv)) {
    CopyASCIItoUTF16(binaryData, aBinaryData);
  } else {
    aBinaryData.Truncate();
  }

  return rv;
}

nsresult
Base64Decode(const nsACString &aString, nsACString &aBinaryData)
{
  
  if (aString.Length() > PR_UINT32_MAX / 3) {
    return NS_ERROR_FAILURE;
  }

  uint32_t binaryDataLen = ((aString.Length() * 3) / 4);

  char *buffer;

  
  if (aBinaryData.SetCapacity(binaryDataLen + 1, fallible_t()) &&
    (buffer = aBinaryData.BeginWriting()) &&
    PL_Base64Decode(aString.BeginReading(), aString.Length(), buffer)) {
    
    
    
    if (!aString.IsEmpty() && aString[aString.Length() - 1] == '=') {
      if (aString.Length() > 1 && aString[aString.Length() - 2] == '=') {
        binaryDataLen -= 2;
      } else {
        binaryDataLen -= 1;
      }
    }
    buffer[binaryDataLen] = '\0';

    aBinaryData.SetLength(binaryDataLen);
    return NS_OK;
  }

  aBinaryData.Truncate();
  return NS_ERROR_INVALID_ARG;
}

nsresult
Base64Decode(const nsAString &aBinaryData, nsAString &aString)
{
  NS_LossyConvertUTF16toASCII binaryData(aBinaryData);
  nsCAutoString string;

  nsresult rv = Base64Decode(binaryData, string);
  if (NS_SUCCEEDED(rv)) {
    CopyASCIItoUTF16(string, aString);
  } else {
    aString.Truncate();
  }

  return rv;
}

} 

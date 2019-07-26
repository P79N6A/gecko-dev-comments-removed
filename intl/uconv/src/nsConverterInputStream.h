




#ifndef nsConverterInputStream_h
#define nsConverterInputStream_h

#include "nsIInputStream.h"
#include "nsIConverterInputStream.h"
#include "nsIUnicharLineInputStream.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsIUnicodeDecoder.h"
#include "nsReadLine.h"

#define NS_CONVERTERINPUTSTREAM_CONTRACTID "@mozilla.org/intl/converter-input-stream;1"


#define NS_CONVERTERINPUTSTREAM_CID \
  { 0x2bc2ad62, 0xad5d, 0x4b7b, \
   { 0xa9, 0xdb, 0xf7, 0x4a, 0xe2, 0x3, 0xc5, 0x27 } }



class nsConverterInputStream : public nsIConverterInputStream,
                               public nsIUnicharLineInputStream {

 public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIUNICHARINPUTSTREAM
    NS_DECL_NSIUNICHARLINEINPUTSTREAM
    NS_DECL_NSICONVERTERINPUTSTREAM

    nsConverterInputStream() :
        mLastErrorCode(NS_OK),
        mLeftOverBytes(0),
        mUnicharDataOffset(0),
        mUnicharDataLength(0),
        mReplacementChar(DEFAULT_REPLACEMENT_CHARACTER),
        mLineBuffer(nullptr) { }

 private:
    virtual ~nsConverterInputStream() { Close(); }

    uint32_t Fill(nsresult *aErrorCode);
    
    nsCOMPtr<nsIUnicodeDecoder> mConverter;
    FallibleTArray<char> mByteData;
    FallibleTArray<char16_t> mUnicharData;
    nsCOMPtr<nsIInputStream> mInput;

    nsresult  mLastErrorCode;
    uint32_t  mLeftOverBytes;
    uint32_t  mUnicharDataOffset;
    uint32_t  mUnicharDataLength;
    char16_t mReplacementChar;

    nsAutoPtr<nsLineBuffer<char16_t> > mLineBuffer;
};

#endif


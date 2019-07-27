





#ifndef mozilla_SnappyCompressOutputStream_h__
#define mozilla_SnappyCompressOutputStream_h__

#include "mozilla/Attributes.h"
#include "mozilla/UniquePtr.h"
#include "nsCOMPtr.h"
#include "nsIOutputStream.h"
#include "nsISupportsImpl.h"
#include "SnappyFrameUtils.h"

namespace mozilla {

class SnappyCompressOutputStream MOZ_FINAL : public nsIOutputStream
                                           , protected detail::SnappyFrameUtils
{
public:
  
  static const size_t kMaxBlockSize;

  
  
  
  
  explicit SnappyCompressOutputStream(nsIOutputStream* aBaseStream,
                                      size_t aBlockSize = kMaxBlockSize);

  
  
  size_t BlockSize() const;

private:
  virtual ~SnappyCompressOutputStream();

  nsresult FlushToBaseStream();
  nsresult MaybeFlushStreamIdentifier();
  nsresult WriteAll(const char* aBuf, uint32_t aCount,
                    uint32_t* aBytesWrittenOut);

  nsCOMPtr<nsIOutputStream> mBaseStream;
  const size_t mBlockSize;

  
  
  mozilla::UniquePtr<char[]> mBuffer;

  
  size_t mNextByte;

  
  mozilla::UniquePtr<char[]> mCompressedBuffer;
  size_t mCompressedBufferLength;

  
  bool mStreamIdentifierWritten;

public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIOUTPUTSTREAM
};

} 

#endif

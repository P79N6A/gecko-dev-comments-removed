





#ifndef mozilla_SnappyUncompressInputStream_h__
#define mozilla_SnappyUncompressInputStream_h__

#include "mozilla/Attributes.h"
#include "mozilla/UniquePtr.h"
#include "nsCOMPtr.h"
#include "nsIInputStream.h"
#include "nsISupportsImpl.h"
#include "SnappyFrameUtils.h"

namespace mozilla {

class SnappyUncompressInputStream MOZ_FINAL : public nsIInputStream
                                            , protected detail::SnappyFrameUtils
{
public:
  
  
  
  SnappyUncompressInputStream(nsIInputStream* aBaseStream);

private:
  virtual ~SnappyUncompressInputStream();

  
  
  
  nsresult ParseNextChunk(uint32_t* aBytesReadOut);

  
  
  
  
  
  
  
  
  
  
  
  nsresult ReadAll(char* aBuf, uint32_t aCount, uint32_t aMinValidCount,
                   uint32_t* aBytesReadOut);

  
  
  size_t UncompressedLength() const;

  nsCOMPtr<nsIInputStream> mBaseStream;

  
  
  
  
  mozilla::UniquePtr<char[]> mCompressedBuffer;

  
  
  mozilla::UniquePtr<char[]> mUncompressedBuffer;

  
  size_t mUncompressedBytes;

  
  
  size_t mNextByte;

  
  ChunkType mNextChunkType;

  
  size_t mNextChunkDataLength;

  
  
  bool mNeedFirstStreamIdentifier;

public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIINPUTSTREAM
};

} 

#endif

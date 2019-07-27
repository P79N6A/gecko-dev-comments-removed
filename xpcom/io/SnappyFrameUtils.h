





#ifndef mozilla_SnappyFrameUtils_h__
#define mozilla_SnappyFrameUtils_h__

#include "mozilla/Attributes.h"
#include "nsError.h"

namespace mozilla {
namespace detail {











class SnappyFrameUtils
{
public:
  enum ChunkType
  {
    Unknown,
    StreamIdentifier,
    CompressedData,
    UncompressedData,
    Padding,
    Reserved,
    ChunkTypeCount
  };

  static const size_t kChunkTypeLength = 1;
  static const size_t kChunkLengthLength = 3;
  static const size_t kHeaderLength = kChunkTypeLength + kChunkLengthLength;
  static const size_t kStreamIdentifierDataLength = 6;
  static const size_t kCRCLength = 4;

  static nsresult
  WriteStreamIdentifier(char* aDest, size_t aDestLength,
                        size_t* aBytesWrittenOut);

  static nsresult
  WriteCompressedData(char* aDest, size_t aDestLength,
                      const char* aData, size_t aDataLength,
                      size_t* aBytesWrittenOut);

  static nsresult
  ParseHeader(const char* aSource, size_t aSourceLength, ChunkType* aTypeOut,
              size_t* aDataLengthOut);

  static nsresult
  ParseData(char* aDest, size_t aDestLength,
            ChunkType aType, const char* aData, size_t aDataLength,
            size_t* aBytesWrittenOut, size_t* aBytesReadOut);

  static nsresult
  ParseStreamIdentifier(char* aDest, size_t aDestLength,
                        const char* aData, size_t aDataLength,
                        size_t* aBytesWrittenOut, size_t* aBytesReadOut);

  static nsresult
  ParseCompressedData(char* aDest, size_t aDestLength,
                      const char* aData, size_t aDataLength,
                      size_t* aBytesWrittenOut, size_t* aBytesReadOut);

  static size_t
  MaxCompressedBufferLength(size_t aSourceLength);

protected:
  SnappyFrameUtils() { }
  virtual ~SnappyFrameUtils() { }
};

} 
} 

#endif 

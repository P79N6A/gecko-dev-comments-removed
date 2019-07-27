



#ifndef BYTE_READER_H_
#define BYTE_READER_H_

#include "mozilla/Endian.h"
#include "mozilla/Vector.h"
#include "nsTArray.h"

namespace mp4_demuxer {

class ByteReader
{
public:
  ByteReader() : mPtr(nullptr), mRemaining(0) {}
  explicit ByteReader(const mozilla::Vector<uint8_t>& aData)
    : mPtr(&aData[0]), mRemaining(aData.length())
  {
  }
  ByteReader(const uint8_t* aData, size_t aSize)
    : mPtr(aData), mRemaining(aSize)
  {
  }
  template<size_t S>
  ByteReader(const nsAutoTArray<uint8_t, S>& aData)
    : mPtr(&aData[0]), mRemaining(aData.Length())
  {
  }

  void SetData(const nsTArray<uint8_t>& aData)
  {
    MOZ_ASSERT(!mPtr && !mRemaining);
    mPtr = &aData[0];
    mRemaining = aData.Length();
  }

  ~ByteReader()
  {
    MOZ_ASSERT(!mRemaining);
  }

  
  void DiscardRemaining() {
    mRemaining = 0;
  }

  size_t Remaining() const { return mRemaining; }

  bool CanRead8() { return mRemaining >= 1; }

  uint8_t ReadU8()
  {
    auto ptr = Read(1);
    if (!ptr) {
      MOZ_ASSERT(false);
      return 0;
    }
    return *ptr;
  }

  bool CanRead16() { return mRemaining >= 2; }

  uint16_t ReadU16()
  {
    auto ptr = Read(2);
    if (!ptr) {
      MOZ_ASSERT(false);
      return 0;
    }
    return mozilla::BigEndian::readUint16(ptr);
  }

  uint32_t ReadU32()
  {
    auto ptr = Read(4);
    if (!ptr) {
      MOZ_ASSERT(false);
      return 0;
    }
    return mozilla::BigEndian::readUint32(ptr);
  }

  int64_t Read32()
  {
    auto ptr = Read(4);
    if (!ptr) {
      MOZ_ASSERT(false);
      return 0;
    }
    return mozilla::BigEndian::readInt32(ptr);
  }

  uint64_t ReadU64()
  {
    auto ptr = Read(8);
    if (!ptr) {
      MOZ_ASSERT(false);
      return 0;
    }
    return mozilla::BigEndian::readUint64(ptr);
  }

  int64_t Read64()
  {
    auto ptr = Read(8);
    if (!ptr) {
      MOZ_ASSERT(false);
      return 0;
    }
    return mozilla::BigEndian::readInt64(ptr);
  }

  const uint8_t* Read(size_t aCount)
  {
    if (aCount > mRemaining) {
      mRemaining = 0;
      return nullptr;
    }
    mRemaining -= aCount;

    const uint8_t* result = mPtr;
    mPtr += aCount;

    return result;
  }

  template <typename T> bool CanReadType() { return mRemaining >= sizeof(T); }

  template <typename T> T ReadType()
  {
    auto ptr = Read(sizeof(T));
    if (!ptr) {
      MOZ_ASSERT(false);
      return 0;
    }
    return *reinterpret_cast<const T*>(ptr);
  }

  template <typename T>
  bool ReadArray(nsTArray<T>& aDest, size_t aLength)
  {
    auto ptr = Read(aLength * sizeof(T));
    if (!ptr) {
      return false;
    }

    aDest.Clear();
    aDest.AppendElements(reinterpret_cast<const T*>(ptr), aLength);
    return true;
  }

private:
  const uint8_t* mPtr;
  size_t mRemaining;
};
}

#endif

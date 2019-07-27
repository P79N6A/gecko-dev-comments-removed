




#ifndef ChunkSet_h__
#define ChunkSet_h__

#include "Entries.h"
#include "nsString.h"
#include "nsTArray.h"

namespace mozilla {
namespace safebrowsing {







class ChunkSet {
public:
  ChunkSet() {}
  ~ChunkSet() {}

  nsresult Serialize(nsACString& aStr);
  nsresult Set(uint32_t aChunk);
  bool Has(uint32_t chunk) const;
  nsresult Merge(const ChunkSet& aOther);
  uint32_t Length() const { return mChunks.Length(); }
  nsresult Remove(const ChunkSet& aOther);
  void Clear();

  nsresult Write(nsIOutputStream* aOut) {
    return WriteTArray(aOut, mChunks);
  }
  nsresult Read(nsIInputStream* aIn, uint32_t aNumElements) {
    return ReadTArray(aIn, &mChunks, aNumElements);
  }

private:
  FallibleTArray<uint32_t> mChunks;
};

}
}

#endif

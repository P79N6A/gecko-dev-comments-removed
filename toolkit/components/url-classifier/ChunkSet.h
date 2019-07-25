






































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
  nsresult Set(PRUint32 aChunk);
  nsresult Unset(PRUint32 aChunk);
  void Clear();
  nsresult Merge(const ChunkSet& aOther);
  nsresult Remove(const ChunkSet& aOther);

  bool Has(PRUint32 chunk) const;

  uint32 Length() const { return mChunks.Length(); }

  nsresult Write(nsIOutputStream* aOut) {
    return WriteTArray(aOut, mChunks);
  }

  nsresult Read(nsIInputStream* aIn, PRUint32 aNumElements) {
    return ReadTArray(aIn, &mChunks, aNumElements);
  }

  uint32 *Begin() { return mChunks.Elements(); }
  uint32 *End() { return mChunks.Elements() + mChunks.Length(); }

private:
  nsTArray<uint32> mChunks;
};

}
}

#endif

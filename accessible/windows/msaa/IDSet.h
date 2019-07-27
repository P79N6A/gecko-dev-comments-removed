









#ifndef MOZILLA_A11Y_IDSet_h_
#define MOZILLA_A11Y_IDSet_h_

#include "mozilla/Attributes.h"
#include "mozilla/MathAlgorithms.h"
#include "mozilla/SplayTree.h"

namespace mozilla {
namespace a11y {











class IDSet
{
public:
  MOZ_CONSTEXPR IDSet() : mBitSet(), mIdx(0) {}

  


  uint32_t GetID()
  {
    uint32_t idx = mIdx;
    while (true) {
      BitSetElt* elt = mBitSet.findOrInsert(BitSetElt(idx));
      if (elt->mBitvec[0] != UINT64_MAX) {
        uint32_t i = CountTrailingZeroes64(~elt->mBitvec[0]);

        elt->mBitvec[0] |= (1ull << i);
        mIdx = idx;
        return ~(elt->mIdx * bitsPerElt + i);
      }

      if (elt->mBitvec[1] != UINT64_MAX) {
        uint32_t i = CountTrailingZeroes64(~elt->mBitvec[1]);

        elt->mBitvec[1] |= (1ull << i);
        mIdx = idx;
        return ~(elt->mIdx * bitsPerElt + bitsPerWord + i);
      }

      idx++;
      if (idx > sMaxIdx) {
        idx = 0;
      }

      if (idx == mIdx) {
        MOZ_CRASH("used up all the available ids");
      }
    }
  }

  


  void ReleaseID(uint32_t aID)
  {
    aID = ~aID;
    MOZ_ASSERT(aID < static_cast<uint32_t>(INT32_MAX));

    uint32_t idx = aID / bitsPerElt;
    mIdx = idx;
    BitSetElt* elt = mBitSet.find(BitSetElt(idx));
    MOZ_ASSERT(elt);

    uint32_t vecIdx = (aID % bitsPerElt) / bitsPerWord;
    elt->mBitvec[vecIdx] &= ~(1ull << (aID % bitsPerWord));
    if (elt->mBitvec[0] == 0 && elt->mBitvec[1] == 0) {
      delete mBitSet.remove(*elt);
    }
  }

private:
  static const unsigned int wordsPerElt = 2;
  static const unsigned int bitsPerWord = 64;
  static const unsigned int bitsPerElt = wordsPerElt * bitsPerWord;
  static const uint32_t sMaxIdx = INT32_MAX / bitsPerElt;

  struct BitSetElt : mozilla::SplayTreeNode<BitSetElt>
  {
    explicit BitSetElt(uint32_t aIdx) :
      mIdx(aIdx)
    { mBitvec[0] = mBitvec[1] = 0; }

    uint64_t mBitvec[wordsPerElt];
    uint32_t mIdx;

    static int compare(const BitSetElt& a, const BitSetElt& b)
    {
      if (a.mIdx == b.mIdx) {
        return 0;
      }

      if (a.mIdx < b.mIdx) {
        return -1;
      }
      return 1;
    }
  };

  SplayTree<BitSetElt, BitSetElt> mBitSet;
  uint32_t mIdx;
};

}
}

#endif

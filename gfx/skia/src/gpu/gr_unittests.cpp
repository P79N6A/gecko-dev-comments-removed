







#include "GrBinHashKey.h"
#include "GrDrawTarget.h"
#include "SkMatrix.h"
#include "GrRedBlackTree.h"


void gr_run_unittests();




#if GR_DEBUG
static bool LT(const int& elem, int value) {
    return elem < value;
}
static bool EQ(const int& elem, int value) {
    return elem == value;
}
#include "GrTBSearch.h"

static void test_bsearch() {
    const int array[] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 22, 33, 44, 55, 66, 77, 88, 99
    };

    for (size_t n = 0; n < GR_ARRAY_COUNT(array); n++) {
        for (size_t i = 0; i < n; i++) {
            int index = GrTBSearch<int, int>(array, n, array[i]);
            GrAssert(index == (int) i);
            index = GrTBSearch<int, int>(array, n, -array[i]);
            GrAssert(index < 0);
        }
    }
}
#endif


class BogusEntry {};

static void test_binHashKey()
{
    const char* testStringA_ = "abcdABCD";
    const char* testStringB_ = "abcdBBCD";
    const uint32_t* testStringA = reinterpret_cast<const uint32_t*>(testStringA_);
    const uint32_t* testStringB = reinterpret_cast<const uint32_t*>(testStringB_);
    enum {
        kDataLenUsedForKey = 8
    };

    GrTBinHashKey<BogusEntry, kDataLenUsedForKey> keyA;
    keyA.setKeyData(testStringA);
    
    GrTBinHashKey<BogusEntry, kDataLenUsedForKey> keyA2(keyA);
    GrAssert(keyA.compare(keyA2) == 0);
    GrAssert(keyA.getHash() == keyA2.getHash());
    
    keyA2.setKeyData(testStringA);
    GrAssert(keyA.compare(keyA2) == 0);
    GrAssert(keyA.getHash() == keyA2.getHash());
    
    GrTBinHashKey<BogusEntry, kDataLenUsedForKey> keyB;
    keyB.setKeyData(testStringB);
    GrAssert(keyA.compare(keyB) < 0);
    GrAssert(keyA.getHash() != keyB.getHash());
}


void gr_run_unittests() {
    GR_DEBUGCODE(test_bsearch();)
    test_binHashKey();
    GrRedBlackTree<int>::UnitTest();
}

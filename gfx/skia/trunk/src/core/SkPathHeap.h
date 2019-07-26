






#ifndef SkPathHeap_DEFINED
#define SkPathHeap_DEFINED

#include "SkRefCnt.h"
#include "SkChunkAlloc.h"
#include "SkTDArray.h"

class SkPath;
class SkReadBuffer;
class SkWriteBuffer;

class SkPathHeap : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkPathHeap)

    SkPathHeap();
    SkPathHeap(SkReadBuffer&);
    virtual ~SkPathHeap();

    



    int append(const SkPath&);

    


    int insert(const SkPath&);

    
    int count() const { return fPaths.count(); }
    const SkPath& operator[](int index) const {
        return *fPaths[index];
    }

    void flatten(SkWriteBuffer&) const;

private:
    
    SkChunkAlloc        fHeap;
    
    SkTDArray<SkPath*>  fPaths;

    class LookupEntry {
    public:
        LookupEntry(const SkPath& path);

        int storageSlot() const { return fStorageSlot; }
        void setStorageSlot(int storageSlot) { fStorageSlot = storageSlot; }

        static bool Less(const LookupEntry& a, const LookupEntry& b) {
            return a.fGenerationID < b.fGenerationID;
        }

    private:
        uint32_t fGenerationID;     
        
        int      fStorageSlot;
    };

    SkTDArray<LookupEntry> fLookupTable;

    SkPathHeap::LookupEntry* addIfNotPresent(const SkPath& path);

    typedef SkRefCnt INHERITED;
};

#endif

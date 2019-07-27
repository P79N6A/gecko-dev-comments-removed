






#ifndef SkScaledImageCache_DEFINED
#define SkScaledImageCache_DEFINED

#include "SkBitmap.h"

class SkDiscardableMemory;
class SkMipMap;











class SkScaledImageCache {
public:
    struct ID;

    



    typedef SkDiscardableMemory* (*DiscardableFactory)(size_t bytes);

    




    static ID* FindAndLock(uint32_t pixelGenerationID,
                           int32_t width,
                           int32_t height,
                           SkBitmap* returnedBitmap);

    static ID* FindAndLock(const SkBitmap& original, SkScalar scaleX,
                           SkScalar scaleY, SkBitmap* returnedBitmap);
    static ID* FindAndLockMip(const SkBitmap& original,
                              SkMipMap const** returnedMipMap);


    static ID* AddAndLock(uint32_t pixelGenerationID,
                          int32_t width,
                          int32_t height,
                          const SkBitmap& bitmap);

    static ID* AddAndLock(const SkBitmap& original, SkScalar scaleX,
                          SkScalar scaleY, const SkBitmap& bitmap);
    static ID* AddAndLockMip(const SkBitmap& original, const SkMipMap* mipMap);

    static void Unlock(ID*);

    static size_t GetTotalBytesUsed();
    static size_t GetTotalByteLimit();
    static size_t SetTotalByteLimit(size_t newLimit);

    static size_t SetSingleAllocationByteLimit(size_t);
    static size_t GetSingleAllocationByteLimit();

    static SkBitmap::Allocator* GetAllocator();

    


    static void Dump();

    

    






    SkScaledImageCache(DiscardableFactory);

    





    SkScaledImageCache(size_t byteLimit);

    ~SkScaledImageCache();

    












    ID* findAndLock(uint32_t pixelGenerationID, int32_t width, int32_t height,
                    SkBitmap* returnedBitmap);

    








    ID* findAndLock(const SkBitmap& original, SkScalar scaleX,
                    SkScalar scaleY, SkBitmap* returnedBitmap);
    ID* findAndLockMip(const SkBitmap& original,
                       SkMipMap const** returnedMipMap);

    







    ID* addAndLock(uint32_t pixelGenerationID, int32_t width, int32_t height,
                   const SkBitmap& bitmap);
    ID* addAndLock(const SkBitmap& original, SkScalar scaleX,
                   SkScalar scaleY, const SkBitmap& bitmap);
    ID* addAndLockMip(const SkBitmap& original, const SkMipMap* mipMap);

    





    void unlock(ID*);

    size_t getTotalBytesUsed() const { return fTotalBytesUsed; }
    size_t getTotalByteLimit() const { return fTotalByteLimit; }

    




    size_t setSingleAllocationByteLimit(size_t maximumAllocationSize);
    size_t getSingleAllocationByteLimit() const;
    




    size_t setTotalByteLimit(size_t newLimit);

    SkBitmap::Allocator* allocator() const { return fAllocator; };

    


    void dump() const;

public:
    struct Rec;
    struct Key;
private:
    Rec*    fHead;
    Rec*    fTail;

    class Hash;
    Hash*   fHash;

    DiscardableFactory  fDiscardableFactory;
    
    SkBitmap::Allocator* fAllocator;

    size_t  fTotalBytesUsed;
    size_t  fTotalByteLimit;
    size_t  fSingleAllocationByteLimit;
    int     fCount;

    Rec* findAndLock(uint32_t generationID, SkScalar sx, SkScalar sy,
                     const SkIRect& bounds);
    Rec* findAndLock(const Key& key);
    ID* addAndLock(Rec* rec);

    void purgeRec(Rec*);
    void purgeAsNeeded();

    
    void moveToHead(Rec*);
    void addToHead(Rec*);
    void detach(Rec*);

    void init();    

#ifdef SK_DEBUG
    void validate() const;
#else
    void validate() const {}
#endif
};
#endif

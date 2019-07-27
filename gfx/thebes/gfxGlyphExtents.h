




#ifndef GFX_GLYPHEXTENTS_H
#define GFX_GLYPHEXTENTS_H

#include "gfxFont.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "nsTArray.h"
#include "mozilla/MemoryReporting.h"

class gfxContext;
struct gfxRect;














class gfxGlyphExtents {
public:
    explicit gfxGlyphExtents(int32_t aAppUnitsPerDevUnit) :
        mAppUnitsPerDevUnit(aAppUnitsPerDevUnit) {
        MOZ_COUNT_CTOR(gfxGlyphExtents);
    }
    ~gfxGlyphExtents();

    enum { INVALID_WIDTH = 0xFFFF };

    void NotifyGlyphsChanged() {
        mTightGlyphExtents.Clear();
    }

    
    
    
    
    uint16_t GetContainedGlyphWidthAppUnits(uint32_t aGlyphID) const {
        return mContainedGlyphWidths.Get(aGlyphID);
    }

    bool IsGlyphKnown(uint32_t aGlyphID) const {
        return mContainedGlyphWidths.Get(aGlyphID) != INVALID_WIDTH ||
            mTightGlyphExtents.GetEntry(aGlyphID) != nullptr;
    }

    bool IsGlyphKnownWithTightExtents(uint32_t aGlyphID) const {
        return mTightGlyphExtents.GetEntry(aGlyphID) != nullptr;
    }

    
    
    
    bool GetTightGlyphExtentsAppUnits(gfxFont *aFont,
            gfxContext *aContext, uint32_t aGlyphID, gfxRect *aExtents);

    void SetContainedGlyphWidthAppUnits(uint32_t aGlyphID, uint16_t aWidth) {
        mContainedGlyphWidths.Set(aGlyphID, aWidth);
    }
    void SetTightGlyphExtents(uint32_t aGlyphID, const gfxRect& aExtentsAppUnits);

    int32_t GetAppUnitsPerDevUnit() { return mAppUnitsPerDevUnit; }

    size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;
    size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

private:
    class HashEntry : public nsUint32HashKey {
    public:
        
        
        explicit HashEntry(KeyTypePointer aPtr) : nsUint32HashKey(aPtr) {}
        HashEntry(const HashEntry& toCopy) : nsUint32HashKey(toCopy) {
          x = toCopy.x; y = toCopy.y; width = toCopy.width; height = toCopy.height;
        }

        float x, y, width, height;
    };

    enum { BLOCK_SIZE_BITS = 7, BLOCK_SIZE = 1 << BLOCK_SIZE_BITS }; 

    class GlyphWidths {
    public:
        void Set(uint32_t aIndex, uint16_t aValue);
        uint16_t Get(uint32_t aIndex) const {
            uint32_t block = aIndex >> BLOCK_SIZE_BITS;
            if (block >= mBlocks.Length())
                return INVALID_WIDTH;
            uintptr_t bits = mBlocks[block];
            if (!bits)
                return INVALID_WIDTH;
            uint32_t indexInBlock = aIndex & (BLOCK_SIZE - 1);
            if (bits & 0x1) {
                if (GetGlyphOffset(bits) != indexInBlock)
                    return INVALID_WIDTH;
                return GetWidth(bits);
            }
            uint16_t *widths = reinterpret_cast<uint16_t *>(bits);
            return widths[indexInBlock];
        }

        uint32_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;
        
        ~GlyphWidths();

    private:
        static uint32_t GetGlyphOffset(uintptr_t aBits) {
            NS_ASSERTION(aBits & 0x1, "This is really a pointer...");
            return (aBits >> 1) & ((1 << BLOCK_SIZE_BITS) - 1);
        }
        static uint32_t GetWidth(uintptr_t aBits) {
            NS_ASSERTION(aBits & 0x1, "This is really a pointer...");
            return aBits >> (1 + BLOCK_SIZE_BITS);
        }
        static uintptr_t MakeSingle(uint32_t aGlyphOffset, uint16_t aWidth) {
            return (aWidth << (1 + BLOCK_SIZE_BITS)) + (aGlyphOffset << 1) + 1;
        }

        nsTArray<uintptr_t> mBlocks;
    };

    GlyphWidths             mContainedGlyphWidths;
    nsTHashtable<HashEntry> mTightGlyphExtents;
    int32_t                 mAppUnitsPerDevUnit;

private:
    
    gfxGlyphExtents(const gfxGlyphExtents& aOther) = delete;
    gfxGlyphExtents& operator=(const gfxGlyphExtents& aOther) = delete;
};

#endif

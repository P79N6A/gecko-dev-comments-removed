






































#ifndef GFX_FONT_UTILS_H
#define GFX_FONT_UTILS_H

#include "gfxTypes.h"

#include "prtypes.h"
#include "prcpucfg.h"

#include "nsDataHashtable.h"

#include "nsITimer.h"
#include "nsCOMPtr.h"
#include "nsIRunnable.h"
#include "nsThreadUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "nsIStreamBufferAccess.h"


#ifdef __MINGW32__
#undef min
#undef max
#endif

class gfxSparseBitSet {
private:
    enum { BLOCK_SIZE = 32 };   
    enum { BLOCK_SIZE_BITS = BLOCK_SIZE * 8 };
    enum { BLOCK_INDEX_SHIFT = 8 };

    struct Block {
        Block(const Block& aBlock) { memcpy(mBits, aBlock.mBits, sizeof(mBits)); }
        Block(unsigned char memsetValue = 0) { memset(mBits, memsetValue, BLOCK_SIZE); }
        PRUint8 mBits[BLOCK_SIZE];
    };

public:
    gfxSparseBitSet() { }
    gfxSparseBitSet(const gfxSparseBitSet& aBitset) {
        PRUint32 len = aBitset.mBlocks.Length();
        mBlocks.AppendElements(len);
        for (PRUint32 i = 0; i < len; ++i) {
            Block *block = aBitset.mBlocks[i];
            if (block)
                mBlocks[i] = new Block(*block);
        }
    }
    PRBool test(PRUint32 aIndex) {
        NS_ASSERTION(mBlocks.DebugGetHeader(), "mHdr is null, this is bad");
        PRUint32 blockIndex = aIndex/BLOCK_SIZE_BITS;
        if (blockIndex >= mBlocks.Length())
            return PR_FALSE;
        Block *block = mBlocks[blockIndex];
        if (!block)
            return PR_FALSE;
        return ((block->mBits[(aIndex>>3) & (BLOCK_SIZE - 1)]) & (1 << (aIndex & 0x7))) != 0;
    }

    PRBool TestRange(PRUint32 aStart, PRUint32 aEnd) {
        PRUint32 startBlock, endBlock, blockLen;
        
        
        startBlock = aStart >> BLOCK_INDEX_SHIFT;
        blockLen = mBlocks.Length();
        if (startBlock >= blockLen) return PR_FALSE;
        
        
        PRUint32 blockIndex;
        PRBool hasBlocksInRange = PR_FALSE;

        endBlock = aEnd >> BLOCK_INDEX_SHIFT;
        blockIndex = startBlock;
        for (blockIndex = startBlock; blockIndex <= endBlock; blockIndex++) {
            if (blockIndex < blockLen && mBlocks[blockIndex])
                hasBlocksInRange = PR_TRUE;
        }
        if (!hasBlocksInRange) return PR_FALSE;

        Block *block;
        PRUint32 i, start, end;
        
        
        if ((block = mBlocks[startBlock])) {
            start = aStart;
            end = PR_MIN(aEnd, ((startBlock+1) << BLOCK_INDEX_SHIFT) - 1);
            for (i = start; i <= end; i++) {
                if ((block->mBits[(i>>3) & (BLOCK_SIZE - 1)]) & (1 << (i & 0x7)))
                    return PR_TRUE;
            }
        }
        if (endBlock == startBlock) return PR_FALSE;

        
        for (blockIndex = startBlock + 1; blockIndex < endBlock; blockIndex++) {
            PRUint32 index;
            
            if (blockIndex >= blockLen || !(block = mBlocks[blockIndex])) continue;
            for (index = 0; index < BLOCK_SIZE; index++) {
                if (block->mBits[index]) 
                    return PR_TRUE;
            }
        }
        
        
        if (endBlock < blockLen && (block = mBlocks[endBlock])) {
            start = endBlock << BLOCK_INDEX_SHIFT;
            end = aEnd;
            for (i = start; i <= end; i++) {
                if ((block->mBits[(i>>3) & (BLOCK_SIZE - 1)]) & (1 << (i & 0x7)))
                    return PR_TRUE;
            }
        }
        
        return PR_FALSE;
    }
    
    void set(PRUint32 aIndex) {
        PRUint32 blockIndex = aIndex/BLOCK_SIZE_BITS;
        if (blockIndex >= mBlocks.Length()) {
            nsAutoPtr<Block> *blocks = mBlocks.AppendElements(blockIndex + 1 - mBlocks.Length());
            if (NS_UNLIKELY(!blocks)) 
                return;
        }
        Block *block = mBlocks[blockIndex];
        if (!block) {
            block = new Block;
            if (NS_UNLIKELY(!block)) 
                return;
            mBlocks[blockIndex] = block;
        }
        block->mBits[(aIndex>>3) & (BLOCK_SIZE - 1)] |= 1 << (aIndex & 0x7);
    }

    void SetRange(PRUint32 aStart, PRUint32 aEnd) {
        const PRUint32 startIndex = aStart/BLOCK_SIZE_BITS;
        const PRUint32 endIndex = aEnd/BLOCK_SIZE_BITS;

        if (endIndex >= mBlocks.Length()) {
            PRUint32 numNewBlocks = endIndex + 1 - mBlocks.Length();
            nsAutoPtr<Block> *blocks = mBlocks.AppendElements(numNewBlocks);
            if (NS_UNLIKELY(!blocks)) 
                return;
        }

        for (PRUint32 i = startIndex; i <= endIndex; ++i) {
            const PRUint32 blockFirstBit = i * BLOCK_SIZE_BITS;
            const PRUint32 blockLastBit = blockFirstBit + BLOCK_SIZE_BITS - 1;

            Block *block = mBlocks[i];
            if (!block) {
                PRBool fullBlock = PR_FALSE;
                if (aStart <= blockFirstBit && aEnd >= blockLastBit)
                    fullBlock = PR_TRUE;

                block = new Block(fullBlock ? 0xFF : 0);

                if (NS_UNLIKELY(!block)) 
                    return;
                mBlocks[i] = block;

                if (fullBlock)
                    continue;
            }

            const PRUint32 start = aStart > blockFirstBit ? aStart - blockFirstBit : 0;
            const PRUint32 end = PR_MIN(aEnd - blockFirstBit, BLOCK_SIZE_BITS - 1);

            for (PRUint32 bit = start; bit <= end; ++bit) {
                block->mBits[bit>>3] |= 1 << (bit & 0x7);
            }
        }
    }

    void clear(PRUint32 aIndex) {
        PRUint32 blockIndex = aIndex/BLOCK_SIZE_BITS;
        if (blockIndex >= mBlocks.Length()) {
            nsAutoPtr<Block> *blocks = mBlocks.AppendElements(blockIndex + 1 - mBlocks.Length());
            if (NS_UNLIKELY(!blocks)) 
                return;
        }
        Block *block = mBlocks[blockIndex];
        if (!block) {
            block = new Block;
            if (NS_UNLIKELY(!block)) 
                return;
            mBlocks[blockIndex] = block;
        }
        block->mBits[(aIndex>>3) & (BLOCK_SIZE - 1)] &= ~(1 << (aIndex & 0x7));
    }

    void ClearRange(PRUint32 aStart, PRUint32 aEnd) {
        const PRUint32 startIndex = aStart/BLOCK_SIZE_BITS;
        const PRUint32 endIndex = aEnd/BLOCK_SIZE_BITS;

        if (endIndex >= mBlocks.Length()) {
            PRUint32 numNewBlocks = endIndex + 1 - mBlocks.Length();
            nsAutoPtr<Block> *blocks = mBlocks.AppendElements(numNewBlocks);
            if (NS_UNLIKELY(!blocks)) 
                return;
        }

        for (PRUint32 i = startIndex; i <= endIndex; ++i) {
            const PRUint32 blockFirstBit = i * BLOCK_SIZE_BITS;
            const PRUint32 blockLastBit = blockFirstBit + BLOCK_SIZE_BITS - 1;

            Block *block = mBlocks[i];
            if (!block) {
                PRBool fullBlock = PR_FALSE;
                if (aStart <= blockFirstBit && aEnd >= blockLastBit)
                    fullBlock = PR_TRUE;

                block = new Block(fullBlock ? 0xFF : 0);

                if (NS_UNLIKELY(!block)) 
                    return;
                mBlocks[i] = block;

                if (fullBlock)
                    continue;
            }

            const PRUint32 start = aStart > blockFirstBit ? aStart - blockFirstBit : 0;
            const PRUint32 end = PR_MIN(aEnd - blockFirstBit, BLOCK_SIZE_BITS - 1);

            for (PRUint32 bit = start; bit <= end; ++bit) {
                block->mBits[bit>>3] &= ~(1 << (bit & 0x7));
            }
        }
    }

    PRUint32 GetSize() {
        PRUint32 size = 0;
        for (PRUint32 i = 0; i < mBlocks.Length(); i++) {
            if (mBlocks[i])
                size += sizeof(Block);
            size += sizeof(nsAutoPtr<Block>);
        }
        return size;
    }

    
    void reset() {
        PRUint32 i;
        for (i = 0; i < mBlocks.Length(); i++)
            mBlocks[i] = nsnull;    
    }
    
    nsTArray< nsAutoPtr<Block> > mBlocks;
};

#define TRUETYPE_TAG(a, b, c, d) ((a) << 24 | (b) << 16 | (c) << 8 | (d))

namespace mozilla {



#pragma pack(1)

struct AutoSwap_PRUint16 {
#ifdef __SUNPRO_CC
    AutoSwap_PRUint16& operator = (const PRUint16 aValue)
      { this->value = NS_SWAP16(aValue); return *this; }
#else
    AutoSwap_PRUint16(PRUint16 aValue) { value = NS_SWAP16(aValue); }
#endif
    operator PRUint16() const { return NS_SWAP16(value); }
    operator PRUint32() const { return NS_SWAP16(value); }
    operator PRUint64() const { return NS_SWAP16(value); }
    PRUint16 value;
};

struct AutoSwap_PRInt16 {
#ifdef __SUNPRO_CC
    AutoSwap_PRInt16& operator = (const PRInt16 aValue)
      { this->value = NS_SWAP16(aValue); return *this; }
#else
    AutoSwap_PRInt16(PRInt16 aValue) { value = NS_SWAP16(aValue); }
#endif
    operator PRInt16() const { return NS_SWAP16(value); }
    operator PRUint32() const { return NS_SWAP16(value); }
    PRInt16  value;
};

struct AutoSwap_PRUint32 {
#ifdef __SUNPRO_CC
    AutoSwap_PRUint32& operator = (const PRUint32 aValue)
      { this->value = NS_SWAP32(aValue); return *this; }
#else
    AutoSwap_PRUint32(PRUint32 aValue) { value = NS_SWAP32(aValue); }
#endif
    operator PRUint32() const { return NS_SWAP32(value); }
    PRUint32  value;
};

struct AutoSwap_PRUint64 {
#ifdef __SUNPRO_CC
    AutoSwap_PRUint64& operator = (const PRUint64 aValue)
      { this->value = NS_SWAP64(aValue); return *this; }
#else
    AutoSwap_PRUint64(PRUint64 aValue) { value = NS_SWAP64(aValue); }
#endif
    operator PRUint64() const { return NS_SWAP64(value); }
    PRUint64  value;
};

#pragma pack()

} 


struct FontDataOverlay {
    
    PRUint32  overlaySrc;    
    PRUint32  overlaySrcLen; 
    PRUint32  overlayDest;   
};
    
enum gfxUserFontType {
    GFX_USERFONT_UNKNOWN = 0,
    GFX_USERFONT_OPENTYPE = 1,
    GFX_USERFONT_SVG = 2,
    GFX_USERFONT_WOFF = 3
};

class THEBES_API gfxFontUtils {

public:
    
    enum {
        NAME_ID_FAMILY = 1,
        NAME_ID_STYLE = 2,
        NAME_ID_UNIQUE = 3,
        NAME_ID_FULL = 4,
        NAME_ID_VERSION = 5,
        NAME_ID_POSTSCRIPT = 6,
        NAME_ID_PREFERRED_FAMILY = 16,
        NAME_ID_PREFERRED_STYLE = 17,

        PLATFORM_ALL = -1,
        PLATFORM_ID_UNICODE = 0,           
        PLATFORM_ID_MAC = 1,
        PLATFORM_ID_ISO = 2,
        PLATFORM_ID_MICROSOFT = 3,

        ENCODING_ID_MAC_ROMAN = 0,         
        ENCODING_ID_MAC_JAPANESE = 1,      
        ENCODING_ID_MAC_TRAD_CHINESE = 2,  
        ENCODING_ID_MAC_KOREAN = 3,        
        ENCODING_ID_MAC_ARABIC = 4,
        ENCODING_ID_MAC_HEBREW = 5,
        ENCODING_ID_MAC_GREEK = 6,
        ENCODING_ID_MAC_CYRILLIC = 7,
        ENCODING_ID_MAC_DEVANAGARI = 9,
        ENCODING_ID_MAC_GURMUKHI = 10,
        ENCODING_ID_MAC_GUJARATI = 11,
        ENCODING_ID_MAC_SIMP_CHINESE = 25,

        ENCODING_ID_MICROSOFT_SYMBOL = 0,  
        ENCODING_ID_MICROSOFT_UNICODEBMP = 1,
        ENCODING_ID_MICROSOFT_SHIFTJIS = 2,
        ENCODING_ID_MICROSOFT_PRC = 3,
        ENCODING_ID_MICROSOFT_BIG5 = 4,
        ENCODING_ID_MICROSOFT_WANSUNG = 5,
        ENCODING_ID_MICROSOFT_JOHAB  = 6,
        ENCODING_ID_MICROSOFT_UNICODEFULL = 10,

        LANG_ALL = -1,
        LANG_ID_MAC_ENGLISH = 0,      
        LANG_ID_MAC_HEBREW = 10,      
        LANG_ID_MAC_JAPANESE = 11,    
        LANG_ID_MAC_ARABIC = 12,
        LANG_ID_MAC_ICELANDIC = 15,
        LANG_ID_MAC_TURKISH = 17,
        LANG_ID_MAC_TRAD_CHINESE = 19,
        LANG_ID_MAC_URDU = 20,
        LANG_ID_MAC_KOREAN = 23,
        LANG_ID_MAC_POLISH = 25,
        LANG_ID_MAC_FARSI = 31,
        LANG_ID_MAC_SIMP_CHINESE = 33,
        LANG_ID_MAC_ROMANIAN = 37,
        LANG_ID_MAC_CZECH = 38,
        LANG_ID_MAC_SLOVAK = 39,

        LANG_ID_MICROSOFT_EN_US = 0x0409,        
        
        CMAP_MAX_CODEPOINT = 0x10ffff     
                                          
    };

    
    struct NameHeader {
        mozilla::AutoSwap_PRUint16    format;       
        mozilla::AutoSwap_PRUint16    count;        
        mozilla::AutoSwap_PRUint16    stringOffset; 
                                                    
    };

    struct NameRecord {
        mozilla::AutoSwap_PRUint16    platformID;   
        mozilla::AutoSwap_PRUint16    encodingID;   
        mozilla::AutoSwap_PRUint16    languageID;   
        mozilla::AutoSwap_PRUint16    nameID;       
        mozilla::AutoSwap_PRUint16    length;       
        mozilla::AutoSwap_PRUint16    offset;       
                                                    
    };

    
    
    static inline PRUint16
    ReadShortAt(const PRUint8 *aBuf, PRUint32 aIndex)
    {
        return (aBuf[aIndex] << 8) | aBuf[aIndex + 1];
    }
    
    static inline PRUint16
    ReadShortAt16(const PRUint16 *aBuf, PRUint32 aIndex)
    {
        const PRUint8 *buf = reinterpret_cast<const PRUint8*>(aBuf);
        PRUint32 index = aIndex << 1;
        return (buf[index] << 8) | buf[index+1];
    }
    
    static inline PRUint32
    ReadLongAt(const PRUint8 *aBuf, PRUint32 aIndex)
    {
        return ((aBuf[aIndex] << 24) | (aBuf[aIndex + 1] << 16) | 
                (aBuf[aIndex + 2] << 8) | (aBuf[aIndex + 3]));
    }
    
    static nsresult
    ReadCMAPTableFormat12(PRUint8 *aBuf, PRUint32 aLength, 
                          gfxSparseBitSet& aCharacterMap);
    
    static nsresult 
    ReadCMAPTableFormat4(PRUint8 *aBuf, PRUint32 aLength, 
                         gfxSparseBitSet& aCharacterMap);

    static nsresult
    ReadCMAP(PRUint8 *aBuf, PRUint32 aBufLength, gfxSparseBitSet& aCharacterMap,
             PRPackedBool& aUnicodeFont, PRPackedBool& aSymbolFont);

#ifdef XP_WIN

    
    
    
    
    
    static nsresult
    MakeEOTHeader(const PRUint8 *aFontData, PRUint32 aFontDataLength,
                  nsTArray<PRUint8> *aHeader, FontDataOverlay *aOverlay);

    
    
    static PRBool
    IsCffFont(const PRUint8* aFontData);

#endif

    
    static gfxUserFontType
    DetermineFontDataType(const PRUint8 *aFontData, PRUint32 aFontDataLength);

    
    
    static PRBool
    ValidateSFNTHeaders(const PRUint8 *aFontData, PRUint32 aFontDataLength);
    
    
    
    static nsresult
    RenameFont(const nsAString& aName, const PRUint8 *aFontData, 
               PRUint32 aFontDataLength, nsTArray<PRUint8> *aNewFont);
    
    
    static nsresult
    ReadNames(nsTArray<PRUint8>& aNameTable, PRUint32 aNameID, 
              PRInt32 aPlatformID, nsTArray<nsString>& aNames);
      
    
    
    static nsresult
    ReadCanonicalName(nsTArray<PRUint8>& aNameTable, PRUint32 aNameID, 
                      nsString& aName);
      
    
    
    
    static PRBool
    DecodeFontName(const PRUint8 *aBuf, PRInt32 aLength, 
                   PRUint32 aPlatformCode, PRUint32 aScriptCode,
                   PRUint32 aLangCode, nsAString& dest);

    static inline bool IsJoiner(PRUint32 ch) {
        return (ch == 0x200C ||
                ch == 0x200D ||
                ch == 0x2060);
    }

    static inline bool IsInvalid(PRUint32 ch) {
        return (ch == 0xFFFD);
    }

    static PRUint8 CharRangeBit(PRUint32 ch);
    
    
    static void GetPrefsFontList(const char *aPrefName, 
                                 nsTArray<nsString>& aFontList);

    
    static nsresult MakeUniqueUserFontName(nsAString& aName);

protected:
    static nsresult
    ReadNames(nsTArray<PRUint8>& aNameTable, PRUint32 aNameID, 
              PRInt32 aLangID, PRInt32 aPlatformID, nsTArray<nsString>& aNames);

    
    
    static const char*
    GetCharsetForFontName(PRUint16 aPlatform, PRUint16 aScript, PRUint16 aLanguage);

    struct MacFontNameCharsetMapping {
        PRUint16    mEncoding;
        PRUint16    mLanguage;
        const char *mCharsetName;

        bool operator<(const MacFontNameCharsetMapping& rhs) const {
            return (mEncoding < rhs.mEncoding) ||
                   ((mEncoding == rhs.mEncoding) && (mLanguage < rhs.mLanguage));
        }
    };
    static const MacFontNameCharsetMapping gMacFontNameCharsets[];
    static const char* gISOFontNameCharsets[];
    static const char* gMSFontNameCharsets[];
};



class gfxFontInfoLoader {
public:

    
    
    
    
    
    
    
    
    typedef enum {
        stateInitial,
        stateTimerOnDelay,
        stateTimerOnInterval,
        stateTimerOff
    } TimerState;

    gfxFontInfoLoader() :
        mInterval(0), mState(stateInitial)
    {
    }

    virtual ~gfxFontInfoLoader() {}

    
    void StartLoader(PRUint32 aDelay, PRUint32 aInterval) {
        mInterval = aInterval;

        
        if (mState != stateInitial && mState != stateTimerOff)
            CancelLoader();

        
        if (!mTimer) {
            mTimer = do_CreateInstance("@mozilla.org/timer;1");
            if (!mTimer) {
                NS_WARNING("Failure to create font info loader timer");
                return;
            }
        }

        
        PRUint32 timerInterval;

        if (aDelay) {
            mState = stateTimerOnDelay;
            timerInterval = aDelay;
        } else {
            mState = stateTimerOnInterval;
            timerInterval = mInterval;
        }

        InitLoader();

        
        mTimer->InitWithFuncCallback(LoaderTimerCallback, this, aDelay, 
                                     nsITimer::TYPE_REPEATING_SLACK);
    }

    
    void CancelLoader() {
        if (mState == stateInitial)
            return;
        mState = stateTimerOff;
        if (mTimer) {
            mTimer->Cancel();
        }
        FinishLoader();
    }

protected:

    
    virtual void InitLoader() = 0;

    
    virtual PRBool RunLoader() = 0;

    
    virtual void FinishLoader() = 0;

    static void LoaderTimerCallback(nsITimer *aTimer, void *aThis) {
        gfxFontInfoLoader *loader = static_cast<gfxFontInfoLoader*>(aThis);
        loader->LoaderTimerFire();
    }

    
    void LoaderTimerFire() {
        if (mState == stateTimerOnDelay) {
            mState = stateTimerOnInterval;
            mTimer->SetDelay(mInterval);
        }

        PRBool done = RunLoader();
        if (done) {
            CancelLoader();
        }
    }

    nsCOMPtr<nsITimer> mTimer;
    PRUint32 mInterval;
    TimerState mState;
};

#endif

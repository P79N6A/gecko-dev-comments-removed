




































#include "gfxTextRunWordCache.h"





class TextRunWordCache {
public:
    TextRunWordCache() {
        mCache.Init(100);
    }
    ~TextRunWordCache() {
        NS_WARN_IF_FALSE(mCache.Count() == 0, "Textrun cache not empty!");
    }

    








    gfxTextRun *MakeTextRun(const PRUnichar *aText, PRUint32 aLength,
                            gfxFontGroup *aFontGroup,
                            const gfxFontGroup::Parameters *aParams,
                            PRUint32 aFlags);
    








    gfxTextRun *MakeTextRun(const PRUint8 *aText, PRUint32 aLength,
                            gfxFontGroup *aFontGroup,
                            const gfxFontGroup::Parameters *aParams,
                            PRUint32 aFlags);

    



    void RemoveTextRun(gfxTextRun *aTextRun);

protected:
    struct CacheHashKey {
        void        *mFontOrGroup;
        const void  *mString;
        PRUint32     mLength;
        PRUint32     mAppUnitsPerDevUnit;
        PRUint32     mStringHash;
        PRPackedBool mIsDoubleByteText;
        PRPackedBool mIsRTL;
        PRPackedBool mEnabledOptionalLigatures;
        
        CacheHashKey(gfxTextRun *aBaseTextRun, void *aFontOrGroup,
                     PRUint32 aStart, PRUint32 aLength, PRUint32 aHash)
            : mFontOrGroup(aFontOrGroup), mString(aBaseTextRun->GetTextAt(aStart)),
              mLength(aLength),
              mAppUnitsPerDevUnit(aBaseTextRun->GetAppUnitsPerDevUnit()),
              mStringHash(aHash),
              mIsDoubleByteText((aBaseTextRun->GetFlags() & gfxTextRunFactory::TEXT_IS_8BIT) == 0),
              mIsRTL(aBaseTextRun->IsRightToLeft()),
              mEnabledOptionalLigatures((aBaseTextRun->GetFlags() & gfxTextRunFactory::TEXT_DISABLE_OPTIONAL_LIGATURES) == 0)
        {
        }
    };

    class CacheHashEntry : public PLDHashEntryHdr {
    public:
        typedef const CacheHashKey &KeyType;
        typedef const CacheHashKey *KeyTypePointer;

        
        
        CacheHashEntry(KeyTypePointer aKey) : mTextRun(nsnull), mWordOffset(0),
            mHashedByFont(PR_FALSE) { }
        CacheHashEntry(const CacheHashEntry& toCopy) { NS_ERROR("Should not be called"); }
        ~CacheHashEntry() { }

        PRBool KeyEquals(const KeyTypePointer aKey) const;
        static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
        static PLDHashNumber HashKey(const KeyTypePointer aKey);
        enum { ALLOW_MEMMOVE = PR_TRUE };

        gfxTextRun *mTextRun;
        
        
        
        PRUint32    mWordOffset:31;
        
        
        
        PRUint32    mHashedByFont:1;
    };
    
    
    
    struct DeferredWord {
        gfxTextRun *mSourceTextRun;
        PRUint32    mSourceOffset;
        PRUint32    mDestOffset;
        PRUint32    mLength;
        PRUint32    mHash;
    };
    
    PRBool LookupWord(gfxTextRun *aTextRun, gfxFont *aFirstFont,
                      PRUint32 aStart, PRUint32 aEnd, PRUint32 aHash,
                      nsTArray<DeferredWord>* aDeferredWords);
    void FinishTextRun(gfxTextRun *aTextRun, gfxTextRun *aNewRun,
                       gfxContext *aContext,
                       const nsTArray<DeferredWord>& aDeferredWords,
                       PRBool aSuccessful);
    void RemoveWord(gfxTextRun *aTextRun, PRUint32 aStart,
                    PRUint32 aEnd, PRUint32 aHash);    

    nsTHashtable<CacheHashEntry> mCache;
};

static PRLogModuleInfo *gWordCacheLog = PR_NewLogModule("wordCache");

static inline PRUint32
HashMix(PRUint32 aHash, PRUnichar aCh)
{
    return (aHash >> 28) ^ (aHash << 4) ^ aCh;
}




static void *GetWordFontOrGroup(gfxTextRun *aTextRun, PRUint32 aOffset,
                                PRUint32 aLength)
{
    PRUint32 glyphRunCount;
    const gfxTextRun::GlyphRun *glyphRuns = aTextRun->GetGlyphRuns(&glyphRunCount);
    PRUint32 glyphRunIndex = aTextRun->FindFirstGlyphRunContaining(aOffset);
    gfxFontGroup *fontGroup = aTextRun->GetFontGroup();
    gfxFont *firstFont = fontGroup->GetFontAt(0);
    if (glyphRuns[glyphRunIndex].mFont != firstFont)
        return fontGroup;

    PRUint32 glyphRunEnd = glyphRunIndex == glyphRunCount - 1
        ? aTextRun->GetLength() : glyphRuns[glyphRunIndex + 1].mCharacterOffset;
    if (aOffset + aLength <= glyphRunEnd)
        return firstFont;
    return fontGroup;
}

#define UNICODE_NBSP 0x00A0



static PRBool
IsBoundarySpace(PRUnichar aChar)
{
    return aChar == ' ' || aChar == UNICODE_NBSP;
}

static PRBool
IsWordBoundary(PRUnichar aChar)
{
    return IsBoundarySpace(aChar) || gfxFontGroup::IsInvalidChar(aChar);
}






















PRBool
TextRunWordCache::LookupWord(gfxTextRun *aTextRun, gfxFont *aFirstFont,
                             PRUint32 aStart, PRUint32 aEnd, PRUint32 aHash,
                             nsTArray<DeferredWord>* aDeferredWords)
{
    if (aEnd <= aStart)
        return PR_TRUE;

    CacheHashKey key(aTextRun, aFirstFont, aStart, aEnd - aStart, aHash);
    CacheHashEntry *fontEntry = mCache.PutEntry(key);
    if (!fontEntry)
        return PR_FALSE;
    CacheHashEntry *existingEntry = nsnull;

    if (fontEntry->mTextRun) {
        existingEntry = fontEntry;
    } else {
        PR_LOG(gWordCacheLog, PR_LOG_DEBUG, ("%p(%d-%d,%d): added using font", aTextRun, aStart, aEnd - aStart, aHash));
        key.mFontOrGroup = aTextRun->GetFontGroup();
        CacheHashEntry *groupEntry = mCache.GetEntry(key);
        if (groupEntry) {
            existingEntry = groupEntry;
            mCache.RawRemoveEntry(fontEntry);
            PR_LOG(gWordCacheLog, PR_LOG_DEBUG, ("%p(%d-%d,%d): removed using font", aTextRun, aStart, aEnd - aStart, aHash));
            fontEntry = nsnull;
        }
    }
    
    
    
    

    if (existingEntry) {
        if (aDeferredWords) {
            DeferredWord word = { existingEntry->mTextRun,
                  existingEntry->mWordOffset, aStart, aEnd - aStart, aHash };
            aDeferredWords->AppendElement(word);
        } else {
            aTextRun->CopyGlyphDataFrom(existingEntry->mTextRun,
                existingEntry->mWordOffset, aEnd - aStart, aStart, PR_FALSE);
        }
        return PR_TRUE;
    }

    
    
    fontEntry->mTextRun = aTextRun;
    fontEntry->mWordOffset = aStart;
    fontEntry->mHashedByFont = PR_TRUE;
    return PR_FALSE;
}













void
TextRunWordCache::FinishTextRun(gfxTextRun *aTextRun, gfxTextRun *aNewRun,
                                gfxContext *aContext,
                                const nsTArray<DeferredWord>& aDeferredWords,
                                PRBool aSuccessful)
{
    aTextRun->SetFlagBits(gfxTextRunWordCache::TEXT_IN_CACHE);

    PRUint32 i;
    gfxFontGroup *fontGroup = aTextRun->GetFontGroup();
    gfxFont *font = fontGroup->GetFontAt(0);
    
    for (i = 0; i < aDeferredWords.Length(); ++i) {
        const DeferredWord *word = &aDeferredWords[i];
        gfxTextRun *source = word->mSourceTextRun;
        if (!source) {
            source = aNewRun;
            
            
            
            
            if (!aSuccessful ||
                GetWordFontOrGroup(aNewRun, word->mSourceOffset, word->mLength) != font) {
                CacheHashKey key(aTextRun, font, word->mDestOffset, word->mLength,
                                 word->mHash);
                NS_ASSERTION(mCache.GetEntry(key),
                             "This entry should have been added previously!");
                mCache.RemoveEntry(key);
                PR_LOG(gWordCacheLog, PR_LOG_DEBUG, ("%p(%d-%d,%d): removed using font", aTextRun, word->mDestOffset, word->mLength, word->mHash));
                
                if (aSuccessful) {
                    key.mFontOrGroup = fontGroup;
                    CacheHashEntry *groupEntry = mCache.PutEntry(key);
                    if (groupEntry) {
                        PR_LOG(gWordCacheLog, PR_LOG_DEBUG, ("%p(%d-%d,%d): added using fontgroup", aTextRun, word->mDestOffset, word->mLength, word->mHash));
                        groupEntry->mTextRun = aTextRun;
                        groupEntry->mWordOffset = word->mDestOffset;
                        groupEntry->mHashedByFont = PR_FALSE;
                        NS_ASSERTION(mCache.GetEntry(key),
                                     "We should find the thing we just added!");
                    }
                }
            }
        }
        if (aSuccessful) {
            
            
            
            aTextRun->CopyGlyphDataFrom(source,
                word->mSourceOffset, word->mLength, word->mDestOffset,
                source == aNewRun);
            
            PRUint32 endCharIndex;
            if (i + 1 < aDeferredWords.Length()) {
                endCharIndex = aDeferredWords[i + 1].mDestOffset;
            } else {
                endCharIndex = aTextRun->GetLength();
            }
            PRUint32 charIndex;
            for (charIndex = word->mDestOffset + word->mLength;
                 charIndex < endCharIndex; ++charIndex) {
                if (IsBoundarySpace(aTextRun->GetChar(charIndex))) {
                    aTextRun->SetSpaceGlyph(font, aContext, charIndex);
                }
            }
        }
    }
}

gfxTextRun *
TextRunWordCache::MakeTextRun(const PRUnichar *aText, PRUint32 aLength,
                              gfxFontGroup *aFontGroup,
                              const gfxFontGroup::Parameters *aParams,
                              PRUint32 aFlags)
{
    nsAutoPtr<gfxTextRun> textRun;
    textRun = new gfxTextRun(aParams, aText, aLength, aFontGroup, aFlags);
    if (!textRun || !textRun->GetCharacterGlyphs())
        return nsnull;   

    gfxFont *font = aFontGroup->GetFontAt(0);
    nsresult rv = textRun->AddGlyphRun(font, 0);
    NS_ENSURE_SUCCESS(rv, nsnull);

    nsAutoTArray<PRUnichar,200> tempString;
    nsAutoTArray<DeferredWord,50> deferredWords;
    PRUint32 i;
    PRUint32 wordStart = 0;
    PRUint32 hash = 0;
    for (i = 0; i <= aLength; ++i) {
        PRUnichar ch = i < aLength ? aText[i] : ' ';
        if (IsWordBoundary(ch)) {
            PRBool hit = LookupWord(textRun, font, wordStart, i, hash,
                                    deferredWords.Length() == 0 ? nsnull : &deferredWords);
            if (!hit) {
                if (tempString.Length() > 0) {
                    tempString.AppendElement(' ');
                }
                PRUint32 offset = tempString.Length();
                PRUint32 length = i - wordStart;
                PRUnichar *chars = tempString.AppendElements(length);
                if (!chars) {
                    FinishTextRun(textRun, nsnull, nsnull, deferredWords, PR_FALSE);
                    return nsnull;
                }
                memcpy(chars, aText + wordStart, length*sizeof(PRUnichar));
                DeferredWord word = { nsnull, offset, wordStart, length, hash };
                deferredWords.AppendElement(word);
            }
            
            if (deferredWords.Length() == 0) {
                if (IsBoundarySpace(ch) && i < aLength) {
                    textRun->SetSpaceGlyph(font, aParams->mContext, i);
                } 
                  
            }
            hash = 0;
            wordStart = i + 1;
        } else {
            hash = HashMix(hash, ch);
        }
    }

    if (deferredWords.Length() == 0) {
        
        
        
        return textRun.forget();
    }

    
    gfxTextRunFactory::Parameters params =
        { aParams->mContext, nsnull, nsnull, nsnull, 0, aParams->mAppUnitsPerDevUnit };
    nsAutoPtr<gfxTextRun> newRun;
    newRun = aFontGroup->MakeTextRun(tempString.Elements(), tempString.Length(),
                                     &params, aFlags | gfxTextRunFactory::TEXT_IS_PERSISTENT);

    FinishTextRun(textRun, newRun, aParams->mContext, deferredWords, newRun != nsnull);
    return textRun.forget();
}

gfxTextRun *
TextRunWordCache::MakeTextRun(const PRUint8 *aText, PRUint32 aLength,
                              gfxFontGroup *aFontGroup,
                              const gfxFontGroup::Parameters *aParams,
                              PRUint32 aFlags)
{
    aFlags |= gfxTextRunFactory::TEXT_IS_8BIT;
    nsAutoPtr<gfxTextRun> textRun;
    textRun = new gfxTextRun(aParams, aText, aLength, aFontGroup, aFlags);
    if (!textRun || !textRun->GetCharacterGlyphs())
        return nsnull;

    gfxFont *font = aFontGroup->GetFontAt(0);
    nsresult rv = textRun->AddGlyphRun(font, 0);
    NS_ENSURE_SUCCESS(rv, nsnull);

    nsAutoTArray<PRUint8,200> tempString;
    nsAutoTArray<DeferredWord,50> deferredWords;
    PRUint32 i;
    PRUint32 wordStart = 0;
    PRUint32 hash = 0;
    for (i = 0; i <= aLength; ++i) {
        PRUint8 ch = i < aLength ? aText[i] : ' ';
        if (IsWordBoundary(ch)) {
            PRBool hit = LookupWord(textRun, font, wordStart, i, hash,
                                    deferredWords.Length() == 0 ? nsnull : &deferredWords);
            if (!hit) {
                if (tempString.Length() > 0) {
                    tempString.AppendElement(' ');
                }
                PRUint32 offset = tempString.Length();
                PRUint32 length = i - wordStart;
                PRUint8 *chars = tempString.AppendElements(length);
                if (!chars) {
                    FinishTextRun(textRun, nsnull, nsnull, deferredWords, PR_FALSE);
                    return nsnull;
                }
                memcpy(chars, aText + wordStart, length*sizeof(PRUint8));
                DeferredWord word = { nsnull, offset, wordStart, length, hash };
                deferredWords.AppendElement(word);
            }
            
            if (deferredWords.Length() == 0) {
                if (IsBoundarySpace(ch) && i < aLength) {
                    textRun->SetSpaceGlyph(font, aParams->mContext, i);
                } 
                  
            }
            hash = 0;
            wordStart = i + 1;
        } else {
            hash = HashMix(hash, ch);
        }
    }

    if (deferredWords.Length() == 0) {
        
        
        
        return textRun.forget();
    }

    
    gfxTextRunFactory::Parameters params =
        { aParams->mContext, nsnull, nsnull, nsnull, 0, aParams->mAppUnitsPerDevUnit };
    nsAutoPtr<gfxTextRun> newRun;
    newRun = aFontGroup->MakeTextRun(tempString.Elements(), tempString.Length(),
                                     &params, aFlags | gfxTextRunFactory::TEXT_IS_PERSISTENT);

    FinishTextRun(textRun, newRun, aParams->mContext, deferredWords, newRun != nsnull);
    return textRun.forget();
}

void
TextRunWordCache::RemoveWord(gfxTextRun *aTextRun, PRUint32 aStart,
                             PRUint32 aEnd, PRUint32 aHash)
{
    if (aEnd <= aStart)
        return;

    PRUint32 length = aEnd - aStart;
    CacheHashKey key(aTextRun, GetWordFontOrGroup(aTextRun, aStart, length),
                     aStart, length, aHash);
    CacheHashEntry *entry = mCache.GetEntry(key);
    if (entry && entry->mTextRun == aTextRun) {
        
        
        mCache.RemoveEntry(key);
        PR_LOG(gWordCacheLog, PR_LOG_DEBUG, ("%p(%d-%d,%d): removed using %s",
            aTextRun, aStart, length, aHash,
            key.mFontOrGroup == aTextRun->GetFontGroup() ? "fontgroup" : "font"));
    }
}


void
TextRunWordCache::RemoveTextRun(gfxTextRun *aTextRun)
{
    PRUint32 i;
    PRUint32 wordStart = 0;
    PRUint32 hash = 0;
    for (i = 0; i < aTextRun->GetLength(); ++i) {
        PRUnichar ch = aTextRun->GetChar(i);
        if (IsWordBoundary(ch)) {
            RemoveWord(aTextRun, wordStart, i, hash);
            hash = 0;
            wordStart = i + 1;
        } else {
            hash = HashMix(hash, ch);
        }
    }
    RemoveWord(aTextRun, wordStart, i, hash);
}

static PRBool
CompareDifferentWidthStrings(const PRUint8 *aStr1, const PRUnichar *aStr2,
                             PRUint32 aLength)
{
    PRUint32 i;
    for (i = 0; i < aLength; ++i) {
        if (aStr1[i] != aStr2[i])
            return PR_FALSE;
    }
    return PR_TRUE;
}

static PRBool
IsWordEnd(gfxTextRun *aTextRun, PRUint32 aOffset)
{
    PRUint32 runLength = aTextRun->GetLength();
    if (aOffset == runLength)
        return PR_TRUE;
    if (aOffset > runLength)
        return PR_FALSE;
    return IsWordBoundary(aTextRun->GetChar(aOffset));
}

static void *
GetFontOrGroup(gfxFontGroup *aFontGroup, PRBool aUseFont)
{
    return aUseFont
        ? static_cast<void *>(aFontGroup->GetFontAt(0))
        : static_cast<void *>(aFontGroup);
}

PRBool
TextRunWordCache::CacheHashEntry::KeyEquals(const KeyTypePointer aKey) const
{
    if (!mTextRun)
        return PR_FALSE;

    PRUint32 length = aKey->mLength;
    gfxFontGroup *fontGroup = mTextRun->GetFontGroup();
    if (!IsWordEnd(mTextRun, mWordOffset + length) ||
        GetFontOrGroup(fontGroup, mHashedByFont) != aKey->mFontOrGroup ||
        aKey->mAppUnitsPerDevUnit != mTextRun->GetAppUnitsPerDevUnit() ||
        aKey->mIsRTL != mTextRun->IsRightToLeft() ||
        aKey->mEnabledOptionalLigatures != ((mTextRun->GetFlags() & gfxTextRunFactory::TEXT_DISABLE_OPTIONAL_LIGATURES) == 0))
        return PR_FALSE;

    if (mTextRun->GetFlags() & gfxFontGroup::TEXT_IS_8BIT) {
        const PRUint8 *text = mTextRun->GetText8Bit() + mWordOffset;
        if (!aKey->mIsDoubleByteText)
            return memcmp(text, aKey->mString, length) == 0;
        return CompareDifferentWidthStrings(text,
                                            static_cast<const PRUnichar *>(aKey->mString), length);
    } else {
        const PRUnichar *text = mTextRun->GetTextUnicode() + mWordOffset;
        if (aKey->mIsDoubleByteText)
            return memcmp(text, aKey->mString, length*sizeof(PRUnichar)) == 0;
        return CompareDifferentWidthStrings(static_cast<const PRUint8 *>(aKey->mString),
                                            text, length);
    }
}

PLDHashNumber
TextRunWordCache::CacheHashEntry::HashKey(const KeyTypePointer aKey)
{
    return aKey->mStringHash + (long)aKey->mFontOrGroup + aKey->mAppUnitsPerDevUnit +
        aKey->mIsDoubleByteText + aKey->mIsRTL*2 + aKey->mEnabledOptionalLigatures*4;
}

static TextRunWordCache *gTextRunWordCache = nsnull;

nsresult
gfxTextRunWordCache::Init()
{
    gTextRunWordCache = new TextRunWordCache();
    return gTextRunWordCache ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

void
gfxTextRunWordCache::Shutdown()
{
    delete gTextRunWordCache;
    gTextRunWordCache = nsnull;
}

gfxTextRun *
gfxTextRunWordCache::MakeTextRun(const PRUnichar *aText, PRUint32 aLength,
                                 gfxFontGroup *aFontGroup,
                                 const gfxFontGroup::Parameters *aParams,
                                 PRUint32 aFlags)
{
    if (!gTextRunWordCache)
        return nsnull;
    return gTextRunWordCache->MakeTextRun(aText, aLength, aFontGroup, aParams, aFlags);
}

gfxTextRun *
gfxTextRunWordCache::MakeTextRun(const PRUint8 *aText, PRUint32 aLength,
                                 gfxFontGroup *aFontGroup,
                                 const gfxFontGroup::Parameters *aParams,
                                 PRUint32 aFlags)
{
    if (!gTextRunWordCache)
        return nsnull;
    return gTextRunWordCache->MakeTextRun(aText, aLength, aFontGroup, aParams, aFlags);
}

void
gfxTextRunWordCache::RemoveTextRun(gfxTextRun *aTextRun)
{
    if (!gTextRunWordCache)
        return;
    gTextRunWordCache->RemoveTextRun(aTextRun);
}

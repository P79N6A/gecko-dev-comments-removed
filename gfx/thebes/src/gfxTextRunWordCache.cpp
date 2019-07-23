




































#include "gfxTextRunWordCache.h"

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
    return IsBoundarySpace(aChar) || gfxFontGroup::IsInvisibleChar(aChar);
}






















PRBool
gfxTextRunWordCache::LookupWord(gfxTextRun *aTextRun, gfxFont *aFirstFont,
                                PRUint32 aStart, PRUint32 aEnd, PRUint32 aHash,
                                nsTArray<DeferredWord>* aDeferredWords)
{
    if (aEnd <= aStart)
        return PR_TRUE;

    PRUint32 length = aEnd - aStart;
    CacheHashKey key =
        { aFirstFont, aTextRun->GetTextAt(aStart),
          length, aTextRun->GetAppUnitsPerDevUnit(), aHash,
          (aTextRun->GetFlags() & gfxTextRunFactory::TEXT_IS_8BIT) == 0 };
    CacheHashEntry *fontEntry = mCache.PutEntry(key);
    if (!fontEntry)
        return PR_FALSE;
    CacheHashEntry *existingEntry = nsnull;

    if (fontEntry->mTextRun) {
        existingEntry = fontEntry;
    } else {
        key.mFontOrGroup = aTextRun->GetFontGroup();
        CacheHashEntry *groupEntry = mCache.GetEntry(key);
        if (groupEntry) {
            existingEntry = groupEntry;
            mCache.RawRemoveEntry(fontEntry);
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
gfxTextRunWordCache::FinishTextRun(gfxTextRun *aTextRun, gfxTextRun *aNewRun,
                                   const nsTArray<DeferredWord>& aDeferredWords,
                                   PRBool aSuccessful)
{
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
                CacheHashKey key =
                    { font, aTextRun->GetTextAt(word->mDestOffset),
                      word->mLength, aTextRun->GetAppUnitsPerDevUnit(), word->mHash,
                      (aTextRun->GetFlags() & gfxTextRunFactory::TEXT_IS_8BIT) == 0 };
                NS_ASSERTION(mCache.GetEntry(key),
                             "This entry should have been added previously!");
                mCache.RemoveEntry(key);
                
                if (aSuccessful) {
                    key.mFontOrGroup = fontGroup;
                    CacheHashEntry *groupEntry = mCache.PutEntry(key);
                    if (groupEntry) {
                        groupEntry->mTextRun = aTextRun;
                        groupEntry->mWordOffset = word->mDestOffset;
                        groupEntry->mHashedByFont = PR_FALSE;
                    }
                }
            }
        }
        if (aSuccessful) {
            
            
            
            aTextRun->CopyGlyphDataFrom(source,
                word->mSourceOffset, word->mLength, word->mDestOffset,
                source == aNewRun);
        }
    }
}

gfxTextRun *
gfxTextRunWordCache::MakeTextRun(const PRUnichar *aText, PRUint32 aLength,
                                 gfxFontGroup *aFontGroup,
                                 const gfxFontGroup::Parameters *aParams,
                                 PRUint32 aFlags, PRBool *aIsInCache)
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
                    FinishTextRun(textRun, nsnull, deferredWords, PR_FALSE);
                    return nsnull;
                }
                memcpy(chars, aText + wordStart, length*sizeof(PRUnichar));
                DeferredWord word = { nsnull, offset, wordStart, length, hash };
                deferredWords.AppendElement(word);
            }
            
            if (IsBoundarySpace(ch) && i < aLength) {
                textRun->SetSpaceGlyph(font, aParams->mContext, i);
            } 
              
            hash = 0;
            wordStart = i + 1;
        } else {
            hash = HashMix(hash, ch);
        }
    }

    if (deferredWords.Length() == 0) {
        
        
        
        *aIsInCache = PR_FALSE;
        return textRun.forget();
    }
    *aIsInCache = PR_TRUE;

    
    gfxTextRunFactory::Parameters params =
        { aParams->mContext, nsnull, nsnull, nsnull, 0, aParams->mAppUnitsPerDevUnit };
    nsAutoPtr<gfxTextRun> newRun;
    newRun = aFontGroup->MakeTextRun(tempString.Elements(), tempString.Length(),
                                     &params, aFlags | gfxTextRunFactory::TEXT_IS_PERSISTENT);

    FinishTextRun(textRun, newRun, deferredWords, newRun != nsnull);
    return textRun.forget();
}

gfxTextRun *
gfxTextRunWordCache::MakeTextRun(const PRUint8 *aText, PRUint32 aLength,
                                 gfxFontGroup *aFontGroup,
                                 const gfxFontGroup::Parameters *aParams,
                                 PRUint32 aFlags, PRBool *aIsInCache)
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
                    FinishTextRun(textRun, nsnull, deferredWords, PR_FALSE);
                    return nsnull;
                }
                memcpy(chars, aText + wordStart, length*sizeof(PRUint8));
                DeferredWord word = { nsnull, offset, wordStart, length, hash };
                deferredWords.AppendElement(word);
            }
            
            if (IsBoundarySpace(ch) && i < aLength) {
                textRun->SetSpaceGlyph(font, aParams->mContext, i);
            } 
              
            hash = 0;
            wordStart = i + 1;
        } else {
            hash = HashMix(hash, ch);
        }
    }

    if (deferredWords.Length() == 0) {
        
        
        
        *aIsInCache = PR_FALSE;
        return textRun.forget();
    }
    *aIsInCache = PR_TRUE;

    
    gfxTextRunFactory::Parameters params =
        { aParams->mContext, nsnull, nsnull, nsnull, 0, aParams->mAppUnitsPerDevUnit };
    nsAutoPtr<gfxTextRun> newRun;
    newRun = aFontGroup->MakeTextRun(tempString.Elements(), tempString.Length(),
                                     &params, aFlags | gfxTextRunFactory::TEXT_IS_PERSISTENT);

    FinishTextRun(textRun, newRun, deferredWords, newRun != nsnull);
    return textRun.forget();
}

void
gfxTextRunWordCache::RemoveWord(gfxTextRun *aTextRun, PRUint32 aStart,
                                PRUint32 aEnd, PRUint32 aHash)
{
    if (aEnd <= aStart)
        return;

    PRUint32 length = aEnd - aStart;
    CacheHashKey key =
        { GetWordFontOrGroup(aTextRun, aStart, length), aTextRun->GetTextAt(aStart),
          length, aTextRun->GetAppUnitsPerDevUnit(), aHash,
          (aTextRun->GetFlags() & gfxTextRunFactory::TEXT_IS_8BIT) == 0 };
    CacheHashEntry *entry = mCache.GetEntry(key);
    if (entry && entry->mTextRun == aTextRun) {
        
        
        mCache.RemoveEntry(key);
    }
}


void
gfxTextRunWordCache::RemoveTextRun(gfxTextRun *aTextRun)
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
        ? NS_STATIC_CAST(void *, aFontGroup->GetFontAt(0))
        : NS_STATIC_CAST(void *, aFontGroup);
}

PRBool
gfxTextRunWordCache::CacheHashEntry::KeyEquals(const KeyTypePointer aKey) const
{
    if (!mTextRun)
        return PR_FALSE;

    PRUint32 length = aKey->mLength;
    gfxFontGroup *fontGroup = mTextRun->GetFontGroup();
    if (!IsWordEnd(mTextRun, mWordOffset + length) ||
        GetFontOrGroup(fontGroup, mHashedByFont) != aKey->mFontOrGroup ||
        aKey->mAppUnitsPerDevUnit != mTextRun->GetAppUnitsPerDevUnit())
        return PR_FALSE;

    if (mTextRun->GetFlags() & gfxFontGroup::TEXT_IS_8BIT) {
        const PRUint8 *text = mTextRun->GetText8Bit() + mWordOffset;
        if (!aKey->mIsDoubleByteText)
            return memcmp(text, aKey->mString, length) == 0;
        return CompareDifferentWidthStrings(text,
                                            NS_STATIC_CAST(const PRUnichar *, aKey->mString), length);
    } else {
        const PRUnichar *text = mTextRun->GetTextUnicode() + mWordOffset;
        if (aKey->mIsDoubleByteText)
            return memcmp(text, aKey->mString, length*sizeof(PRUnichar)) == 0;
        return CompareDifferentWidthStrings(NS_STATIC_CAST(const PRUint8 *, aKey->mString),
                                            text, length);
    }
}

PLDHashNumber
gfxTextRunWordCache::CacheHashEntry::HashKey(const KeyTypePointer aKey)
{
    return aKey->mStringHash + (long)aKey->mFontOrGroup + aKey->mAppUnitsPerDevUnit +
        aKey->mIsDoubleByteText;
}

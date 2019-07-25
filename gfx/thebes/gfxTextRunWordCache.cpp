






































#include "gfxTextRunWordCache.h"

#include "nsWeakReference.h"
#include "nsCRT.h"
#include "nsServiceManagerUtils.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranch2.h"
#include "nsIPrefService.h"
#include "nsIObserver.h"

#include "nsBidiUtils.h"

#if defined(XP_UNIX)
#include <stdint.h>
#endif

#ifdef DEBUG
#include <stdio.h>
#endif




















class TextRunWordCache :
    public nsIObserver,
    public nsSupportsWeakReference {
public:
    TextRunWordCache() :
        mBidiNumeral(0) {
        mCache.Init(100);
    }
    ~TextRunWordCache() {
        Uninit();
        NS_WARN_IF_FALSE(mCache.Count() == 0, "Textrun cache not empty!");
    }
    void Init();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    








    gfxTextRun *MakeTextRun(const PRUnichar *aText, PRUint32 aLength,
                            gfxFontGroup *aFontGroup,
                            const gfxFontGroup::Parameters *aParams,
                            PRUint32 aFlags);
    








    gfxTextRun *MakeTextRun(const PRUint8 *aText, PRUint32 aLength,
                            gfxFontGroup *aFontGroup,
                            const gfxFontGroup::Parameters *aParams,
                            PRUint32 aFlags);

    



    void RemoveTextRun(gfxTextRun *aTextRun);

    


    void Flush() {
        mCache.Clear(); 
#ifdef DEBUG
        mGeneration++;
#endif
    }

#ifdef DEBUG
    PRUint32 mGeneration;
    void Dump();
#endif

protected:
    struct CacheHashKey {
        void        *mFontOrGroup;
        const void  *mString;
        PRUint32     mLength;
        PRUint32     mAppUnitsPerDevUnit;
        PRUint32     mStringHash;
        PRUint64     mUserFontSetGeneration;
        PRPackedBool mIsDoubleByteText;
        PRPackedBool mIsRTL;
        PRPackedBool mEnabledOptionalLigatures;
        PRPackedBool mOptimizeSpeed;
        
        CacheHashKey(gfxTextRun *aBaseTextRun, void *aFontOrGroup,
                     PRUint32 aStart, PRUint32 aLength, PRUint32 aHash)
            : mFontOrGroup(aFontOrGroup), mString(aBaseTextRun->GetTextAt(aStart)),
              mLength(aLength),
              mAppUnitsPerDevUnit(aBaseTextRun->GetAppUnitsPerDevUnit()),
              mStringHash(aHash),
              mUserFontSetGeneration(aBaseTextRun->GetUserFontSetGeneration()),
              mIsDoubleByteText((aBaseTextRun->GetFlags() & gfxTextRunFactory::TEXT_IS_8BIT) == 0),
              mIsRTL(aBaseTextRun->IsRightToLeft()),
              mEnabledOptionalLigatures((aBaseTextRun->GetFlags() & gfxTextRunFactory::TEXT_DISABLE_OPTIONAL_LIGATURES) == 0),
              mOptimizeSpeed((aBaseTextRun->GetFlags() & gfxTextRunFactory::TEXT_OPTIMIZE_SPEED) != 0)
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
                       const gfxFontGroup::Parameters *aParams,
                       const nsTArray<DeferredWord>& aDeferredWords,
                       PRBool aSuccessful);
    void RemoveWord(gfxTextRun *aTextRun, PRUint32 aStart,
                    PRUint32 aEnd, PRUint32 aHash);
    void Uninit();

    nsTHashtable<CacheHashEntry> mCache;

    PRInt32 mBidiNumeral;
    nsCOMPtr<nsIPrefBranch2> mPrefBranch;
    nsCOMPtr<nsIPrefBranch2> mFontPrefBranch;

#ifdef DEBUG
    static PLDHashOperator CacheDumpEntry(CacheHashEntry* aEntry, void* userArg);
#endif
};

NS_IMPL_ISUPPORTS2(TextRunWordCache, nsIObserver, nsISupportsWeakReference)

static TextRunWordCache *gTextRunWordCache = nsnull;

static PRLogModuleInfo *gWordCacheLog = PR_NewLogModule("wordCache");

void
TextRunWordCache::Init()
{
#ifdef DEBUG
    mGeneration = 0;
#endif

    nsCOMPtr<nsIPrefService> prefService = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (!prefService)
        return;

    nsCOMPtr<nsIPrefBranch> branch;
    prefService->GetBranch("bidi.", getter_AddRefs(branch));
    mPrefBranch = do_QueryInterface(branch);
    if (!mPrefBranch)
        return;

    mPrefBranch->AddObserver("", this, PR_TRUE);
    mPrefBranch->GetIntPref("numeral", &mBidiNumeral);

    nsCOMPtr<nsIPrefBranch> fontBranch;
    prefService->GetBranch("font.", getter_AddRefs(fontBranch));
    mFontPrefBranch = do_QueryInterface(fontBranch);
    if (mFontPrefBranch)
      mFontPrefBranch->AddObserver("", this, PR_TRUE);
}

void
TextRunWordCache::Uninit()
{
    if (mPrefBranch)
        mPrefBranch->RemoveObserver("", this);
    if (mFontPrefBranch)
        mFontPrefBranch->RemoveObserver("", this);
}

NS_IMETHODIMP
TextRunWordCache::Observe(nsISupports     *aSubject,
                          const char      *aTopic,
                          const PRUnichar *aData)
{
    if (!nsCRT::strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
        if (!nsCRT::strcmp(aData, NS_LITERAL_STRING("numeral").get())) {
          mPrefBranch->GetIntPref("numeral", &mBidiNumeral);
        }
        mCache.Clear();
        PR_LOG(gWordCacheLog, PR_LOG_DEBUG, ("flushing the textrun cache"));
#ifdef DEBUG
        mGeneration++;
#endif
    }

    return NS_OK;
}

static inline PRUint32
HashMix(PRUint32 aHash, PRUnichar aCh)
{
    return (aHash >> 28) ^ (aHash << 4) ^ aCh;
}




static void *GetWordFontOrGroup(gfxTextRun *aTextRun, PRUint32 aOffset,
                                PRUint32 aLength)
{
    gfxFontGroup *fontGroup = aTextRun->GetFontGroup();
    if (fontGroup->GetUserFontSet() != nsnull)
        return fontGroup;
        
    PRUint32 glyphRunCount;
    const gfxTextRun::GlyphRun *glyphRuns = aTextRun->GetGlyphRuns(&glyphRunCount);
    PRUint32 glyphRunIndex = aTextRun->FindFirstGlyphRunContaining(aOffset);
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
        
    gfxFontGroup *fontGroup = aTextRun->GetFontGroup();

    PRBool useFontGroup = (fontGroup->GetUserFontSet() != nsnull);
    CacheHashKey key(aTextRun, (useFontGroup ? (void*)fontGroup : (void*)aFirstFont), aStart, aEnd - aStart, aHash);
    CacheHashEntry *fontEntry = mCache.PutEntry(key);
    if (!fontEntry)
        return PR_FALSE;
    CacheHashEntry *existingEntry = nsnull;

    if (fontEntry->mTextRun) {
        existingEntry = fontEntry;
    } else if (useFontGroup) {
        PR_LOG(gWordCacheLog, PR_LOG_DEBUG, ("%p(%d-%d,%d): added using font group", aTextRun, aStart, aEnd - aStart, aHash));
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
                existingEntry->mWordOffset, aEnd - aStart, aStart);
        }
        return PR_TRUE;
    }

#ifdef DEBUG
    ++aTextRun->mCachedWords;
#endif
    
    
    fontEntry->mTextRun = aTextRun;
    fontEntry->mWordOffset = aStart;
    if (!useFontGroup)
        fontEntry->mHashedByFont = PR_TRUE;
    return PR_FALSE;
}













void
TextRunWordCache::FinishTextRun(gfxTextRun *aTextRun, gfxTextRun *aNewRun,
                                const gfxFontGroup::Parameters *aParams,
                                const nsTArray<DeferredWord>& aDeferredWords,
                                PRBool aSuccessful)
{
    aTextRun->SetFlagBits(gfxTextRunWordCache::TEXT_IN_CACHE);

    PRUint32 i;
    gfxFontGroup *fontGroup = aTextRun->GetFontGroup();
    gfxFont *font = fontGroup->GetFontAt(0);
    
    
    
    PRBool useFontGroup = (fontGroup->GetUserFontSet() != nsnull);

    
    for (i = 0; i < aDeferredWords.Length(); ++i) {
        const DeferredWord *word = &aDeferredWords[i];
        gfxTextRun *source = word->mSourceTextRun;
        if (!source) {
            source = aNewRun;
        }
        
        
        PRBool wordStartsInsideCluster;
        PRBool wordStartsInsideLigature;
        if (aSuccessful) {
            wordStartsInsideCluster =
                !source->IsClusterStart(word->mSourceOffset);
            wordStartsInsideLigature =
                !source->IsLigatureGroupStart(word->mSourceOffset);
        }
        if (source == aNewRun) {
            
            
            
            
            PRBool removeFontKey = !aSuccessful ||
                wordStartsInsideCluster || wordStartsInsideLigature ||
                (!useFontGroup && font != GetWordFontOrGroup(aNewRun,
                                                             word->mSourceOffset,
                                                             word->mLength));
            if (removeFontKey) {
                
                CacheHashKey key(aTextRun,
                                 (useFontGroup ? (void*)fontGroup : (void*)font),
                                 word->mDestOffset, word->mLength, word->mHash);
                NS_ASSERTION(mCache.GetEntry(key),
                             "This entry should have been added previously!");
                mCache.RemoveEntry(key);
#ifdef DEBUG
                --aTextRun->mCachedWords;
#endif
                PR_LOG(gWordCacheLog, PR_LOG_DEBUG, ("%p(%d-%d,%d): removed using font", aTextRun, word->mDestOffset, word->mLength, word->mHash));
                
                if (aSuccessful && !wordStartsInsideCluster && !wordStartsInsideLigature) {
                    key.mFontOrGroup = fontGroup;
                    CacheHashEntry *groupEntry = mCache.PutEntry(key);
                    if (groupEntry) {
#ifdef DEBUG
                        ++aTextRun->mCachedWords;
#endif
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
            
            PRUint32 sourceOffset = word->mSourceOffset;
            PRUint32 destOffset = word->mDestOffset;
            PRUint32 length = word->mLength;
            nsAutoPtr<gfxTextRun> tmpTextRun;
            if (wordStartsInsideCluster || wordStartsInsideLigature) {
                NS_ASSERTION(sourceOffset > 0, "How can the first character be inside a cluster?");
                if (wordStartsInsideCluster && destOffset > 0 &&
                    IsBoundarySpace(aTextRun->GetChar(destOffset - 1))) {
                    
                    
                    
                    
                    
                    --sourceOffset;
                    --destOffset;
                    ++length;
                } else {
                    
                    
                    
                    
                    
                    
                    
                    if (source->GetFlags() & gfxFontGroup::TEXT_IS_8BIT) {
                        tmpTextRun = fontGroup->
                            MakeTextRun(source->GetText8Bit() + sourceOffset,
                                        length, aParams, source->GetFlags());
                    } else {
                        tmpTextRun = fontGroup->
                            MakeTextRun(source->GetTextUnicode() + sourceOffset,
                                        length, aParams, source->GetFlags());
                    }
                    if (tmpTextRun) {
                        source = tmpTextRun;
                        sourceOffset = 0;
                    } else {
                        
                        
                        
                        
                        
                        
                        
                        continue;
                    }
                }
            }
            aTextRun->CopyGlyphDataFrom(source, sourceOffset, length,
                                        destOffset);
            
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
                    aTextRun->SetSpaceGlyph(font, aParams->mContext, charIndex);
                }
            }
        }
    }
}

static gfxTextRun *
MakeBlankTextRun(const void* aText, PRUint32 aLength,
                         gfxFontGroup *aFontGroup,
                         const gfxFontGroup::Parameters *aParams,
                         PRUint32 aFlags)
{
    nsAutoPtr<gfxTextRun> textRun;
    textRun = gfxTextRun::Create(aParams, aText, aLength, aFontGroup, aFlags);
    if (!textRun || !textRun->GetCharacterGlyphs())
        return nsnull;
    gfxFont *font = aFontGroup->GetFontAt(0);
    textRun->AddGlyphRun(font, 0);
#ifdef DEBUG
    textRun->mCachedWords = 0;
    textRun->mCacheGeneration = gTextRunWordCache ? gTextRunWordCache->mGeneration : 0;
#endif
    return textRun.forget();
}

gfxTextRun *
TextRunWordCache::MakeTextRun(const PRUnichar *aText, PRUint32 aLength,
                              gfxFontGroup *aFontGroup,
                              const gfxFontGroup::Parameters *aParams,
                              PRUint32 aFlags)
{
    
    aFontGroup->UpdateFontList();

    if (aFontGroup->GetStyle()->size == 0) {
        
        
        
        return MakeBlankTextRun(aText, aLength, aFontGroup, aParams, aFlags);
    }

    nsAutoPtr<gfxTextRun> textRun;
    textRun = gfxTextRun::Create(aParams, aText, aLength, aFontGroup, aFlags);
    if (!textRun || !textRun->GetCharacterGlyphs())
        return nsnull;
#ifdef DEBUG
    textRun->mCachedWords = 0;
    textRun->mCacheGeneration = mGeneration;
#endif

    gfxFont *font = aFontGroup->GetFontAt(0);
    nsresult rv = textRun->AddGlyphRun(font, 0);
    NS_ENSURE_SUCCESS(rv, nsnull);

    nsAutoTArray<PRUnichar,200> tempString;
    nsAutoTArray<DeferredWord,50> deferredWords;
    nsAutoTArray<nsAutoPtr<gfxTextRun>,10> transientRuns;
    PRUint32 i;
    PRUint32 wordStart = 0;
    PRUint32 hash = 0;
    PRBool seenDigitToModify = PR_FALSE;
    PRBool needsNumeralProcessing =
        mBidiNumeral != IBMBIDI_NUMERAL_NOMINAL;
    for (i = 0; i <= aLength; ++i) {
        PRUnichar ch = i < aLength ? aText[i] : ' ';
        if (!seenDigitToModify && needsNumeralProcessing) {
            
            if (HandleNumberInChar(ch, !!(i > 0 ?
                                       IS_ARABIC_CHAR(aText[i-1]) :
                                       (aFlags & gfxTextRunWordCache::TEXT_INCOMING_ARABICCHAR)),
                                   mBidiNumeral) != ch)
                seenDigitToModify = PR_TRUE;
        }
        if (IsWordBoundary(ch)) {
            if (seenDigitToModify) {
                
                
                
                PRUint32 length = i - wordStart;
                nsAutoArrayPtr<PRUnichar> numString;
                numString = new PRUnichar[length];
                for (PRUint32 j = 0; j < length; ++j) {
                    numString[j] = HandleNumberInChar(aText[wordStart+j],
                                                      !!(j > 0 ?
                                                          IS_ARABIC_CHAR(numString[j-1]) :
                                                          (aFlags & gfxTextRunWordCache::TEXT_INCOMING_ARABICCHAR)),
                                                      mBidiNumeral);
                }
                
                gfxTextRun *numRun;
                numRun =
                    aFontGroup->MakeTextRun(numString.get(), length, aParams,
                                            aFlags & ~(gfxTextRunFactory::TEXT_IS_PERSISTENT |
                                                       gfxTextRunFactory::TEXT_IS_8BIT));
                
                
                
                
                
                
                if (numRun) {
                    DeferredWord word = { numRun, 0, wordStart, length, hash };
                    deferredWords.AppendElement(word);
                    transientRuns.AppendElement(numRun);
                } else {
                    seenDigitToModify = PR_FALSE;
                }
            }

            if (!seenDigitToModify) {
                
                PRBool hit = LookupWord(textRun, font, wordStart, i, hash,
                                        deferredWords.Length() == 0 ? nsnull : &deferredWords);
                if (!hit) {
                    
                    
                    tempString.AppendElement(' ');
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
            } else {
                seenDigitToModify = PR_FALSE;
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
    if (tempString.Length() == 0) {
        newRun = aFontGroup->MakeEmptyTextRun(&params, aFlags);
    } else {
        newRun = aFontGroup->MakeTextRun(tempString.Elements(), tempString.Length(),
                                         &params, aFlags | gfxTextRunFactory::TEXT_IS_PERSISTENT);
    }
    FinishTextRun(textRun, newRun, aParams, deferredWords, newRun != nsnull);
    return textRun.forget();
}

gfxTextRun *
TextRunWordCache::MakeTextRun(const PRUint8 *aText, PRUint32 aLength,
                              gfxFontGroup *aFontGroup,
                              const gfxFontGroup::Parameters *aParams,
                              PRUint32 aFlags)
{
    
    aFontGroup->UpdateFontList();

    if (aFontGroup->GetStyle()->size == 0) {
        
        
        
        return MakeBlankTextRun(aText, aLength, aFontGroup, aParams, aFlags);
    }

    aFlags |= gfxTextRunFactory::TEXT_IS_8BIT;
    nsAutoPtr<gfxTextRun> textRun;
    textRun = gfxTextRun::Create(aParams, aText, aLength, aFontGroup, aFlags);
    if (!textRun || !textRun->GetCharacterGlyphs())
        return nsnull;
#ifdef DEBUG
    textRun->mCachedWords = 0;
    textRun->mCacheGeneration = mGeneration;
#endif

    gfxFont *font = aFontGroup->GetFontAt(0);
    nsresult rv = textRun->AddGlyphRun(font, 0);
    NS_ENSURE_SUCCESS(rv, nsnull);

    nsAutoTArray<PRUint8,200> tempString;
    nsAutoTArray<DeferredWord,50> deferredWords;
    nsAutoTArray<nsAutoPtr<gfxTextRun>,10> transientRuns;
    PRUint32 i;
    PRUint32 wordStart = 0;
    PRUint32 hash = 0;
    PRBool seenDigitToModify = PR_FALSE;
    PRBool needsNumeralProcessing =
        mBidiNumeral != IBMBIDI_NUMERAL_NOMINAL;
    for (i = 0; i <= aLength; ++i) {
        PRUint8 ch = i < aLength ? aText[i] : ' ';
        if (!seenDigitToModify && needsNumeralProcessing) {
            
            if (HandleNumberInChar(ch, i == 0 && (aFlags & gfxTextRunWordCache::TEXT_INCOMING_ARABICCHAR),
                                   mBidiNumeral) != ch)
                seenDigitToModify = PR_TRUE;
        }
        if (IsWordBoundary(ch)) {
            if (seenDigitToModify) {
                
                PRUint32 length = i - wordStart;
                nsAutoArrayPtr<PRUnichar> numString;
                numString = new PRUnichar[length];
                for (PRUint32 j = 0; j < length; ++j) {
                    numString[j] = HandleNumberInChar(aText[wordStart+j],
                                                      !!(j > 0 ?
                                                          IS_ARABIC_CHAR(numString[j-1]) :
                                                          (aFlags & gfxTextRunWordCache::TEXT_INCOMING_ARABICCHAR)),
                                                      mBidiNumeral);
                }
                
                gfxTextRun *numRun;
                numRun =
                    aFontGroup->MakeTextRun(numString.get(), length, aParams,
                                            aFlags & ~(gfxTextRunFactory::TEXT_IS_PERSISTENT |
                                                       gfxTextRunFactory::TEXT_IS_8BIT));
                if (numRun) {
                    DeferredWord word = { numRun, 0, wordStart, length, hash };
                    deferredWords.AppendElement(word);
                    transientRuns.AppendElement(numRun);
                } else {
                    seenDigitToModify = PR_FALSE;
                }
            }

            if (!seenDigitToModify) {
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
            } else {
                seenDigitToModify = PR_FALSE;
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
    if (tempString.Length() == 0) {
        newRun = aFontGroup->MakeEmptyTextRun(&params, aFlags);
    } else {
        newRun = aFontGroup->MakeTextRun(tempString.Elements(), tempString.Length(),
                                         &params, aFlags | gfxTextRunFactory::TEXT_IS_PERSISTENT);
    }
    FinishTextRun(textRun, newRun, aParams, deferredWords, newRun != nsnull);
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
#ifdef DEBUG
        --aTextRun->mCachedWords;
#endif
        PR_LOG(gWordCacheLog, PR_LOG_DEBUG, ("%p(%d-%d,%d): removed using %s",
            aTextRun, aStart, length, aHash,
            key.mFontOrGroup == aTextRun->GetFontGroup() ? "fontgroup" : "font"));
    }
}


void
TextRunWordCache::RemoveTextRun(gfxTextRun *aTextRun)
{
#ifdef DEBUG
    if (aTextRun->mCacheGeneration != mGeneration) {
        PR_LOG(gWordCacheLog, PR_LOG_DEBUG, ("cache generation changed (aTextRun %p)", aTextRun));
        return;
    }
#endif
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
#ifdef DEBUG
    NS_ASSERTION(aTextRun->mCachedWords == 0,
                 "Textrun was not completely removed from the cache!");
#endif
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
        aKey->mEnabledOptionalLigatures != ((mTextRun->GetFlags() & gfxTextRunFactory::TEXT_DISABLE_OPTIONAL_LIGATURES) == 0) ||
        aKey->mOptimizeSpeed != ((mTextRun->GetFlags() & gfxTextRunFactory::TEXT_OPTIMIZE_SPEED) != 0) ||
        aKey->mUserFontSetGeneration != (mTextRun->GetUserFontSetGeneration()))
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
    
    
    PRUint32 fontSetGen;
    LL_L2UI(fontSetGen, aKey->mUserFontSetGeneration);

    return aKey->mStringHash + fontSetGen + (PRUint32)(intptr_t)aKey->mFontOrGroup + aKey->mAppUnitsPerDevUnit +
        aKey->mIsDoubleByteText + aKey->mIsRTL*2 + aKey->mEnabledOptionalLigatures*4 +
        aKey->mOptimizeSpeed*8;
}

#ifdef DEBUG
PLDHashOperator
TextRunWordCache::CacheDumpEntry(CacheHashEntry* aEntry, void* userArg)
{
    FILE* output = static_cast<FILE*>(userArg);
    if (!aEntry->mTextRun) {
        fprintf(output, "<EMPTY>\n");
        return PL_DHASH_NEXT;
    }
    fprintf(output, "Word at %p:%d => ", static_cast<void*>(aEntry->mTextRun), aEntry->mWordOffset);
    aEntry->mTextRun->Dump(output);
    fprintf(output, " (hashed by %s)\n", aEntry->mHashedByFont ? "font" : "fontgroup");
    return PL_DHASH_NEXT;
}

void
TextRunWordCache::Dump()
{
    mCache.EnumerateEntries(CacheDumpEntry, stdout);
}
#endif

nsresult
gfxTextRunWordCache::Init()
{
    gTextRunWordCache = new TextRunWordCache();
    if (gTextRunWordCache) {
        
        
        NS_ADDREF(gTextRunWordCache);
        gTextRunWordCache->Init();
    }
    return gTextRunWordCache ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

void
gfxTextRunWordCache::Shutdown()
{
    NS_IF_RELEASE(gTextRunWordCache);
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

void
gfxTextRunWordCache::Flush()
{
    if (!gTextRunWordCache)
        return;
    gTextRunWordCache->Flush();
}

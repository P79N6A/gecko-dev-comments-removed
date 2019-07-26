











#include "utypeinfo.h"  

#include "unicode/utypes.h"

#if !UCONFIG_NO_BREAK_ITERATION

#include "unicode/rbbi.h"
#include "unicode/schriter.h"
#include "unicode/uchriter.h"
#include "unicode/udata.h"
#include "unicode/uclean.h"
#include "rbbidata.h"
#include "rbbirb.h"
#include "cmemory.h"
#include "cstring.h"
#include "umutex.h"
#include "ucln_cmn.h"
#include "brkeng.h"

#include "uassert.h"
#include "uvector.h"


#if U_LOCAL_SERVICE_HOOK
#include "localsvc.h"
#endif

#ifdef RBBI_DEBUG
static UBool fTrace = FALSE;
#endif

U_NAMESPACE_BEGIN


#define START_STATE 1


#define STOP_STATE  0


UOBJECT_DEFINE_RTTI_IMPLEMENTATION(RuleBasedBreakIterator)










RuleBasedBreakIterator::RuleBasedBreakIterator(RBBIDataHeader* data, UErrorCode &status)
{
    init();
    fData = new RBBIDataWrapper(data, status); 
    if (U_FAILURE(status)) {return;}
    if(fData == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
}




RuleBasedBreakIterator::RuleBasedBreakIterator(const RBBIDataHeader* data, enum EDontAdopt, UErrorCode &status)
{
    init();
    fData = new RBBIDataWrapper(data, RBBIDataWrapper::kDontAdopt, status); 
    if (U_FAILURE(status)) {return;}
    if(fData == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
}






RuleBasedBreakIterator::RuleBasedBreakIterator(const uint8_t *compiledRules,
                       uint32_t       ruleLength,
                       UErrorCode     &status) {
    init();
    if (U_FAILURE(status)) {
        return;
    }
    if (compiledRules == NULL || ruleLength < sizeof(RBBIDataHeader)) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    const RBBIDataHeader *data = (const RBBIDataHeader *)compiledRules;
    if (data->fLength > ruleLength) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    fData = new RBBIDataWrapper(data, RBBIDataWrapper::kDontAdopt, status); 
    if (U_FAILURE(status)) {return;}
    if(fData == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
}    








RuleBasedBreakIterator::RuleBasedBreakIterator(UDataMemory* udm, UErrorCode &status)
{
    init();
    fData = new RBBIDataWrapper(udm, status); 
    if (U_FAILURE(status)) {return;}
    if(fData == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
}








RuleBasedBreakIterator::RuleBasedBreakIterator( const UnicodeString  &rules,
                                                UParseError          &parseError,
                                                UErrorCode           &status)
{
    init();
    if (U_FAILURE(status)) {return;}
    RuleBasedBreakIterator *bi = (RuleBasedBreakIterator *)
        RBBIRuleBuilder::createRuleBasedBreakIterator(rules, &parseError, status);
    
    
    
    
    if (U_SUCCESS(status)) {
        *this = *bi;
        delete bi;
    }
}








RuleBasedBreakIterator::RuleBasedBreakIterator() {
    init();
}








RuleBasedBreakIterator::RuleBasedBreakIterator(const RuleBasedBreakIterator& other)
: BreakIterator(other)
{
    this->init();
    *this = other;
}





RuleBasedBreakIterator::~RuleBasedBreakIterator() {
    if (fCharIter!=fSCharIter && fCharIter!=fDCharIter) {
        
        delete fCharIter;
    }
    fCharIter = NULL;
    delete fSCharIter;
    fCharIter = NULL;
    delete fDCharIter;
    fDCharIter = NULL;
    
    utext_close(fText);

    if (fData != NULL) {
        fData->removeReference();
        fData = NULL;
    }
    if (fCachedBreakPositions) {
        uprv_free(fCachedBreakPositions);
        fCachedBreakPositions = NULL;
    }
    if (fLanguageBreakEngines) {
        delete fLanguageBreakEngines;
        fLanguageBreakEngines = NULL;
    }
    if (fUnhandledBreakEngine) {
        delete fUnhandledBreakEngine;
        fUnhandledBreakEngine = NULL;
    }
}





RuleBasedBreakIterator&
RuleBasedBreakIterator::operator=(const RuleBasedBreakIterator& that) {
    if (this == &that) {
        return *this;
    }
    reset();    
    fBreakType = that.fBreakType;
    if (fLanguageBreakEngines != NULL) {
        delete fLanguageBreakEngines;
        fLanguageBreakEngines = NULL;   
    }
    
    UErrorCode status = U_ZERO_ERROR;
    fText = utext_clone(fText, that.fText, FALSE, TRUE, &status);

    if (fCharIter!=fSCharIter && fCharIter!=fDCharIter) {
        delete fCharIter;
    }
    fCharIter = NULL;

    if (that.fCharIter != NULL ) {
        
        
        
        fCharIter = that.fCharIter->clone();
    }

    if (fData != NULL) {
        fData->removeReference();
        fData = NULL;
    }
    if (that.fData != NULL) {
        fData = that.fData->addReference();
    }

    return *this;
}









void RuleBasedBreakIterator::init() {
    UErrorCode  status    = U_ZERO_ERROR;
    fBufferClone          = FALSE;
    fText                 = utext_openUChars(NULL, NULL, 0, &status);
    fCharIter             = NULL;
    fSCharIter            = NULL;
    fDCharIter            = NULL;
    fData                 = NULL;
    fLastRuleStatusIndex  = 0;
    fLastStatusIndexValid = TRUE;
    fDictionaryCharCount  = 0;
    fBreakType            = UBRK_WORD;  
                                        
                                        
                                        

    fCachedBreakPositions    = NULL;
    fLanguageBreakEngines    = NULL;
    fUnhandledBreakEngine    = NULL;
    fNumCachedBreakPositions = 0;
    fPositionInCache         = 0;

#ifdef RBBI_DEBUG
    static UBool debugInitDone = FALSE;
    if (debugInitDone == FALSE) {
        char *debugEnv = getenv("U_RBBIDEBUG");
        if (debugEnv && uprv_strstr(debugEnv, "trace")) {
            fTrace = TRUE;
        }
        debugInitDone = TRUE;
    }
#endif
}










BreakIterator*
RuleBasedBreakIterator::clone(void) const {
    return new RuleBasedBreakIterator(*this);
}





UBool
RuleBasedBreakIterator::operator==(const BreakIterator& that) const {
    if (typeid(*this) != typeid(that)) {
        return FALSE;
    }

    const RuleBasedBreakIterator& that2 = (const RuleBasedBreakIterator&) that;

    if (!utext_equals(fText, that2.fText)) {
        
        
        return FALSE;
    };

    

    if (that2.fData == fData ||
        (fData != NULL && that2.fData != NULL && *that2.fData == *fData)) {
            
            return TRUE;
        }
    return FALSE;
}





int32_t
RuleBasedBreakIterator::hashCode(void) const {
    int32_t   hash = 0;
    if (fData != NULL) {
        hash = fData->hashCode();
    }
    return hash;
}


void RuleBasedBreakIterator::setText(UText *ut, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    reset();
    fText = utext_clone(fText, ut, FALSE, TRUE, &status);

    
    
    
    
    
    
    if (fDCharIter == NULL) {
        static const UChar c = 0;
        fDCharIter = new UCharCharacterIterator(&c, 0);
        if (fDCharIter == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
    }

    if (fCharIter!=fSCharIter && fCharIter!=fDCharIter) {
        
        delete fCharIter;
    }
    fCharIter = fDCharIter;

    this->first();
}


UText *RuleBasedBreakIterator::getUText(UText *fillIn, UErrorCode &status) const {
    UText *result = utext_clone(fillIn, fText, FALSE, TRUE, &status);  
    return result;
}






const UnicodeString&
RuleBasedBreakIterator::getRules() const {
    if (fData != NULL) {
        return fData->getRuleSourceString();
    } else {
        static const UnicodeString *s;
        if (s == NULL) {
            
            
            
            
            s = new UnicodeString;
        }
        return *s;
    }
}








CharacterIterator&
RuleBasedBreakIterator::getText() const {
    return *fCharIter;
}






void
RuleBasedBreakIterator::adoptText(CharacterIterator* newText) {
    
    
    if (fCharIter!=fSCharIter && fCharIter!=fDCharIter) {
        delete fCharIter;
    }

    fCharIter = newText;
    UErrorCode status = U_ZERO_ERROR;
    reset();
    if (newText==NULL || newText->startIndex() != 0) {   
        
        
        fText = utext_openUChars(fText, NULL, 0, &status);
    } else {
        fText = utext_openCharacterIterator(fText, newText, &status);
    }
    this->first();
}






void
RuleBasedBreakIterator::setText(const UnicodeString& newText) {
    UErrorCode status = U_ZERO_ERROR;
    reset();
    fText = utext_openConstUnicodeString(fText, &newText, &status);

    
    
    
    
    if (fSCharIter == NULL) {
        fSCharIter = new StringCharacterIterator(newText);
    } else {
        fSCharIter->setText(newText);
    }

    if (fCharIter!=fSCharIter && fCharIter!=fDCharIter) {
        
        delete fCharIter;
    }
    fCharIter = fSCharIter;

    this->first();
}








RuleBasedBreakIterator &RuleBasedBreakIterator::refreshInputText(UText *input, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return *this;
    }
    if (input == NULL) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return *this;
    }
    int64_t pos = utext_getNativeIndex(fText);
    
    fText = utext_clone(fText, input, FALSE, TRUE, &status);
    if (U_FAILURE(status)) {
        return *this;
    }
    utext_setNativeIndex(fText, pos);
    if (utext_getNativeIndex(fText) != pos) {
        
        
        
        
        status = U_ILLEGAL_ARGUMENT_ERROR;
    }
    return *this;
}






int32_t RuleBasedBreakIterator::first(void) {
    reset();
    fLastRuleStatusIndex  = 0;
    fLastStatusIndexValid = TRUE;
    
    

    utext_setNativeIndex(fText, 0);
    return 0;
}





int32_t RuleBasedBreakIterator::last(void) {
    reset();
    if (fText == NULL) {
        fLastRuleStatusIndex  = 0;
        fLastStatusIndexValid = TRUE;
        return BreakIterator::DONE;
    }

    fLastStatusIndexValid = FALSE;
    int32_t pos = (int32_t)utext_nativeLength(fText);
    utext_setNativeIndex(fText, pos);
    return pos;
}










int32_t RuleBasedBreakIterator::next(int32_t n) {
    int32_t result = current();
    while (n > 0) {
        result = next();
        --n;
    }
    while (n < 0) {
        result = previous();
        ++n;
    }
    return result;
}





int32_t RuleBasedBreakIterator::next(void) {
    
    
    if (fCachedBreakPositions != NULL) {
        if (fPositionInCache < fNumCachedBreakPositions - 1) {
            ++fPositionInCache;
            int32_t pos = fCachedBreakPositions[fPositionInCache];
            utext_setNativeIndex(fText, pos);
            return pos;
        }
        else {
            reset();
        }
    }

    int32_t startPos = current();
    int32_t result = handleNext(fData->fForwardTable);
    if (fDictionaryCharCount > 0) {
        result = checkDictionary(startPos, result, FALSE);
    }
    return result;
}





int32_t RuleBasedBreakIterator::previous(void) {
    int32_t result;
    int32_t startPos;

    
    
    if (fCachedBreakPositions != NULL) {
        if (fPositionInCache > 0) {
            --fPositionInCache;
            
            
            if (fPositionInCache <= 0) {
                fLastStatusIndexValid = FALSE;
            }
            int32_t pos = fCachedBreakPositions[fPositionInCache];
            utext_setNativeIndex(fText, pos);
            return pos;
        }
        else {
            reset();
        }
    }

    
    if (fText == NULL || (startPos = current()) == 0) {
        fLastRuleStatusIndex  = 0;
        fLastStatusIndexValid = TRUE;
        return BreakIterator::DONE;
    }

    if (fData->fSafeRevTable != NULL || fData->fSafeFwdTable != NULL) {
        result = handlePrevious(fData->fReverseTable);
        if (fDictionaryCharCount > 0) {
            result = checkDictionary(result, startPos, TRUE);
        }
        return result;
    }

    
    
    
    
    

    

    int32_t start = current();

    (void)UTEXT_PREVIOUS32(fText);
    int32_t lastResult    = handlePrevious(fData->fReverseTable);
    if (lastResult == UBRK_DONE) {
        lastResult = 0;
        utext_setNativeIndex(fText, 0);
    }
    result = lastResult;
    int32_t lastTag       = 0;
    UBool   breakTagValid = FALSE;

    
    
    

    for (;;) {
        result         = next();
        if (result == BreakIterator::DONE || result >= start) {
            break;
        }
        lastResult     = result;
        lastTag        = fLastRuleStatusIndex;
        breakTagValid  = TRUE;
    }

    
    
    
    
    
    

    
    
    utext_setNativeIndex(fText, lastResult);
    fLastRuleStatusIndex  = lastTag;       
    fLastStatusIndexValid = breakTagValid;

    
    

    return lastResult;
}







int32_t RuleBasedBreakIterator::following(int32_t offset) {
    
    
    
    
    if (fCachedBreakPositions != NULL) {
        if (offset >= fCachedBreakPositions[0]
                && offset < fCachedBreakPositions[fNumCachedBreakPositions - 1]) {
            fPositionInCache = 0;
            
            while (offset >= fCachedBreakPositions[fPositionInCache]) {
                ++fPositionInCache;
            }
            int32_t pos = fCachedBreakPositions[fPositionInCache];
            utext_setNativeIndex(fText, pos);
            return pos;
        }
        else {
            reset();
        }
    }

    
    
    
    fLastRuleStatusIndex  = 0;
    fLastStatusIndexValid = TRUE;
    if (fText == NULL || offset >= utext_nativeLength(fText)) {
        last();
        return next();
    }
    else if (offset < 0) {
        return first();
    }

    
    
    

    int32_t result = 0;

    if (fData->fSafeRevTable != NULL) {
        
        utext_setNativeIndex(fText, offset);
        
        
        
        (void)UTEXT_NEXT32(fText);
        
        handlePrevious(fData->fSafeRevTable);
        int32_t result = next();
        while (result <= offset) {
            result = next();
        }
        return result;
    }
    if (fData->fSafeFwdTable != NULL) {
        
        utext_setNativeIndex(fText, offset);
        (void)UTEXT_PREVIOUS32(fText);
        
        handleNext(fData->fSafeFwdTable);
        
        
        
        int32_t oldresult = previous();
        while (oldresult > offset) {
            int32_t result = previous();
            if (result <= offset) {
                return oldresult;
            }
            oldresult = result;
        }
        int32_t result = next();
        if (result <= offset) {
            return next();
        }
        return result;
    }
    
    
    
    
    
    
    
    

    utext_setNativeIndex(fText, offset);
    if (offset==0 || 
        (offset==1  && utext_getNativeIndex(fText)==0)) {
        return next();
    }
    result = previous();

    while (result != BreakIterator::DONE && result <= offset) {
        result = next();
    }

    return result;
}







int32_t RuleBasedBreakIterator::preceding(int32_t offset) {
    
    
    if (fCachedBreakPositions != NULL) {
        
        
        if (offset > fCachedBreakPositions[0]
                && offset <= fCachedBreakPositions[fNumCachedBreakPositions - 1]) {
            fPositionInCache = 0;
            while (fPositionInCache < fNumCachedBreakPositions
                   && offset > fCachedBreakPositions[fPositionInCache])
                ++fPositionInCache;
            --fPositionInCache;
            
            
            if (fPositionInCache <= 0) {
                fLastStatusIndexValid = FALSE;
            }
            utext_setNativeIndex(fText, fCachedBreakPositions[fPositionInCache]);
            return fCachedBreakPositions[fPositionInCache];
        }
        else {
            reset();
        }
    }

    
    
    
    if (fText == NULL || offset > utext_nativeLength(fText)) {
        
        return last();
    }
    else if (offset < 0) {
        return first();
    }

    
    
    

    if (fData->fSafeFwdTable != NULL) {
        
        utext_setNativeIndex(fText, offset);
        int32_t newOffset = (int32_t)UTEXT_GETNATIVEINDEX(fText);
        if (newOffset != offset) {
            
            
            
            
            
            (void)UTEXT_NEXT32(fText);
            offset = (int32_t)UTEXT_GETNATIVEINDEX(fText);
        }

        
        
        
        
        
        (void)UTEXT_PREVIOUS32(fText);
        handleNext(fData->fSafeFwdTable);
        int32_t result = (int32_t)UTEXT_GETNATIVEINDEX(fText);
        while (result >= offset) {
            result = previous();
        }
        return result;
    }
    if (fData->fSafeRevTable != NULL) {
        
        
        
        
        
        utext_setNativeIndex(fText, offset);
        (void)UTEXT_NEXT32(fText);
        
        
        handlePrevious(fData->fSafeRevTable);

        
        
        
        int32_t oldresult = next();
        while (oldresult < offset) {
            int32_t result = next();
            if (result >= offset) {
                return oldresult;
            }
            oldresult = result;
        }
        int32_t result = previous();
        if (result >= offset) {
            return previous();
        }
        return result;
    }

    
    utext_setNativeIndex(fText, offset);
    return previous();
}








UBool RuleBasedBreakIterator::isBoundary(int32_t offset) {
    
    if (offset == 0) {
        first();       
        return TRUE;
    }

    if (offset == (int32_t)utext_nativeLength(fText)) {
        last();       
        return TRUE;
    }

    
    if (offset < 0) {
        first();       
        return FALSE;
    }

    if (offset > utext_nativeLength(fText)) {
        last();        
        return FALSE;
    }

    
    
    
    utext_previous32From(fText, offset);
    int32_t backOne = (int32_t)UTEXT_GETNATIVEINDEX(fText);
    UBool    result  = following(backOne) == offset;
    return result;
}





int32_t RuleBasedBreakIterator::current(void) const {
    int32_t  pos = (int32_t)UTEXT_GETNATIVEINDEX(fText);
    return pos;
}
 









enum RBBIRunMode {
    RBBI_START,     
    RBBI_RUN,       
    RBBI_END        
};












int32_t RuleBasedBreakIterator::handleNext(const RBBIStateTable *statetable) {
    int32_t             state;
    uint16_t            category        = 0;
    RBBIRunMode         mode;
    
    RBBIStateTableRow  *row;
    UChar32             c;
    int32_t             lookaheadStatus = 0;
    int32_t             lookaheadTagIdx = 0;
    int32_t             result          = 0;
    int32_t             initialPosition = 0;
    int32_t             lookaheadResult = 0;
    UBool               lookAheadHardBreak = (statetable->fFlags & RBBI_LOOKAHEAD_HARD_BREAK) != 0;
    const char         *tableData       = statetable->fTableData;
    uint32_t            tableRowLen     = statetable->fRowLen;

    #ifdef RBBI_DEBUG
        if (fTrace) {
            RBBIDebugPuts("Handle Next   pos   char  state category");
        }
    #endif

    
    fLastStatusIndexValid = TRUE;
    fLastRuleStatusIndex = 0;

    
    initialPosition = (int32_t)UTEXT_GETNATIVEINDEX(fText); 
    result          = initialPosition;
    c               = UTEXT_NEXT32(fText);
    if (fData == NULL || c==U_SENTINEL) {
        return BreakIterator::DONE;
    }

    
    state = START_STATE;
    row = (RBBIStateTableRow *)
            
            (tableData + tableRowLen * state);
            
    
    mode     = RBBI_RUN;
    if (statetable->fFlags & RBBI_BOF_REQUIRED) {
        category = 2;
        mode     = RBBI_START;
    }


    
    
    for (;;) {
        if (c == U_SENTINEL) {
            
            if (mode == RBBI_END) {
                
                
                
                if (lookaheadResult > result) {
                    
                    
                    
                    result               = lookaheadResult;
                    fLastRuleStatusIndex = lookaheadTagIdx;
                    lookaheadStatus = 0;
                } 
                break;
            }
            
            mode = RBBI_END;
            category = 1;
        }

        
        
        
        
        
        if (mode == RBBI_RUN) {
            
            
            
            
            
            UTRIE_GET16(&fData->fTrie, c, category);

            
            
            
            
            
            if ((category & 0x4000) != 0)  {
                fDictionaryCharCount++;
                
                category &= ~0x4000;
            }
        }

       #ifdef RBBI_DEBUG
            if (fTrace) {
                RBBIDebugPrintf("             %4ld   ", utext_getNativeIndex(fText));
                if (0x20<=c && c<0x7f) {
                    RBBIDebugPrintf("\"%c\"  ", c);
                } else {
                    RBBIDebugPrintf("%5x  ", c);
                }
                RBBIDebugPrintf("%3d  %3d\n", state, category);
            }
        #endif

        
        

        
        
        
        U_ASSERT(category<fData->fHeader->fCatCount);
        state = row->fNextState[category];  
        row = (RBBIStateTableRow *)
            
            (tableData + tableRowLen * state);


        if (row->fAccepting == -1) {
            
            if (mode != RBBI_START) {
                result = (int32_t)UTEXT_GETNATIVEINDEX(fText);
            }
            fLastRuleStatusIndex = row->fTagIdx;   
        }

        if (row->fLookAhead != 0) {
            if (lookaheadStatus != 0
                && row->fAccepting == lookaheadStatus) {
                
                result               = lookaheadResult;
                fLastRuleStatusIndex = lookaheadTagIdx;
                lookaheadStatus      = 0;
                
                if (lookAheadHardBreak) {
                    UTEXT_SETNATIVEINDEX(fText, result);
                    return result;
                }
                
                
                goto continueOn;
            }

            int32_t  r = (int32_t)UTEXT_GETNATIVEINDEX(fText);
            lookaheadResult = r;
            lookaheadStatus = row->fLookAhead;
            lookaheadTagIdx = row->fTagIdx;
            goto continueOn;
        }


        if (row->fAccepting != 0) {
            
            
            lookaheadStatus = 0;           
        }

continueOn:
        if (state == STOP_STATE) {
            
            
            
            break;
        }
        
        
        
        
        
        if (mode == RBBI_RUN) {
            c = UTEXT_NEXT32(fText);
        } else {
            if (mode == RBBI_START) {
                mode = RBBI_RUN;
            }
        }


    }

    

    
    
    
    if (result == initialPosition) {
        UTEXT_SETNATIVEINDEX(fText, initialPosition);
        UTEXT_NEXT32(fText);
        result = (int32_t)UTEXT_GETNATIVEINDEX(fText);
    }

    
    UTEXT_SETNATIVEINDEX(fText, result);
    #ifdef RBBI_DEBUG
        if (fTrace) {
            RBBIDebugPrintf("result = %d\n\n", result);
        }
    #endif
    return result;
}













int32_t RuleBasedBreakIterator::handlePrevious(const RBBIStateTable *statetable) {
    int32_t             state;
    uint16_t            category        = 0;
    RBBIRunMode         mode;
    RBBIStateTableRow  *row;
    UChar32             c;
    int32_t             lookaheadStatus = 0;
    int32_t             result          = 0;
    int32_t             initialPosition = 0;
    int32_t             lookaheadResult = 0;
    UBool               lookAheadHardBreak = (statetable->fFlags & RBBI_LOOKAHEAD_HARD_BREAK) != 0;

    #ifdef RBBI_DEBUG
        if (fTrace) {
            RBBIDebugPuts("Handle Previous   pos   char  state category");
        }
    #endif

    
    
    
    
    fLastStatusIndexValid = FALSE;
    fLastRuleStatusIndex = 0;

    
    if (fText == NULL || fData == NULL || UTEXT_GETNATIVEINDEX(fText)==0) {
        return BreakIterator::DONE;
    }

    
    initialPosition = (int32_t)UTEXT_GETNATIVEINDEX(fText);
    result          = initialPosition;
    c               = UTEXT_PREVIOUS32(fText);

    
    state = START_STATE;
    row = (RBBIStateTableRow *)
            (statetable->fTableData + (statetable->fRowLen * state));
    category = 3;
    mode     = RBBI_RUN;
    if (statetable->fFlags & RBBI_BOF_REQUIRED) {
        category = 2;
        mode     = RBBI_START;
    }


    
    
    for (;;) {
        if (c == U_SENTINEL) {
            
            if (mode == RBBI_END) {
                
                
                
                if (lookaheadResult < result) {
                    
                    
                    
                    result               = lookaheadResult;
                    lookaheadStatus = 0;
                } else if (result == initialPosition) {
                    
                    
                    UTEXT_SETNATIVEINDEX(fText, initialPosition);
                    (void)UTEXT_PREVIOUS32(fText);   
                }
                break;
            }
            
            mode = RBBI_END;
            category = 1;
        }

        
        
        
        
        
        if (mode == RBBI_RUN) {
            
            
            
            
            
            UTRIE_GET16(&fData->fTrie, c, category);

            
            
            
            
            
            if ((category & 0x4000) != 0)  {
                fDictionaryCharCount++;
                
                category &= ~0x4000;
            }
        }

        #ifdef RBBI_DEBUG
            if (fTrace) {
                RBBIDebugPrintf("             %4d   ", (int32_t)utext_getNativeIndex(fText));
                if (0x20<=c && c<0x7f) {
                    RBBIDebugPrintf("\"%c\"  ", c);
                } else {
                    RBBIDebugPrintf("%5x  ", c);
                }
                RBBIDebugPrintf("%3d  %3d\n", state, category);
            }
        #endif

        
        

        
        
        
        U_ASSERT(category<fData->fHeader->fCatCount);
        state = row->fNextState[category];  
        row = (RBBIStateTableRow *)
            (statetable->fTableData + (statetable->fRowLen * state));

        if (row->fAccepting == -1) {
            
            result = (int32_t)UTEXT_GETNATIVEINDEX(fText);
        }

        if (row->fLookAhead != 0) {
            if (lookaheadStatus != 0
                && row->fAccepting == lookaheadStatus) {
                
                result               = lookaheadResult;
                lookaheadStatus      = 0;
                
                if (lookAheadHardBreak) {
                    UTEXT_SETNATIVEINDEX(fText, result);
                    return result;
                }
                
                
                goto continueOn;
            }

            int32_t  r = (int32_t)UTEXT_GETNATIVEINDEX(fText);
            lookaheadResult = r;
            lookaheadStatus = row->fLookAhead;
            goto continueOn;
        }


        if (row->fAccepting != 0) {
            
            
            lookaheadStatus = 0;    
        }

continueOn:
        if (state == STOP_STATE) {
            
            
            
            break;
        }

        
        
        
        
        if (mode == RBBI_RUN) {
            c = UTEXT_PREVIOUS32(fText);
        } else {            
            if (mode == RBBI_START) {
                mode = RBBI_RUN;
            }
        }
    }

    

    
    
    
    if (result == initialPosition) {
        UTEXT_SETNATIVEINDEX(fText, initialPosition);
        UTEXT_PREVIOUS32(fText);
        result = (int32_t)UTEXT_GETNATIVEINDEX(fText);
    }

    
    UTEXT_SETNATIVEINDEX(fText, result);
    #ifdef RBBI_DEBUG
        if (fTrace) {
            RBBIDebugPrintf("result = %d\n\n", result);
        }
    #endif
    return result;
}


void
RuleBasedBreakIterator::reset()
{
    if (fCachedBreakPositions) {
        uprv_free(fCachedBreakPositions);
    }
    fCachedBreakPositions = NULL;
    fNumCachedBreakPositions = 0;
    fDictionaryCharCount = 0;
    fPositionInCache = 0;
}
















void RuleBasedBreakIterator::makeRuleStatusValid() {
    if (fLastStatusIndexValid == FALSE) {
        
        if (fText == NULL || current() == 0) {
            
            fLastRuleStatusIndex = 0;
            fLastStatusIndexValid = TRUE;
        } else {
            
            int32_t pa = current();
            previous();
            if (fNumCachedBreakPositions > 0) {
                reset();                
            }
            int32_t pb = next();
            if (pa != pb) {
                
                
                U_ASSERT(pa == pb);
            }
        }
    }
    U_ASSERT(fLastRuleStatusIndex >= 0  &&  fLastRuleStatusIndex < fData->fStatusMaxIdx);
}


int32_t  RuleBasedBreakIterator::getRuleStatus() const {
    RuleBasedBreakIterator *nonConstThis  = (RuleBasedBreakIterator *)this;
    nonConstThis->makeRuleStatusValid();

    
    
    
    int32_t  idx = fLastRuleStatusIndex + fData->fRuleStatusTable[fLastRuleStatusIndex];
    int32_t  tagVal = fData->fRuleStatusTable[idx];

    return tagVal;
}




int32_t RuleBasedBreakIterator::getRuleStatusVec(
             int32_t *fillInVec, int32_t capacity, UErrorCode &status)
{
    if (U_FAILURE(status)) {
        return 0;
    }

    RuleBasedBreakIterator *nonConstThis  = (RuleBasedBreakIterator *)this;
    nonConstThis->makeRuleStatusValid();
    int32_t  numVals = fData->fRuleStatusTable[fLastRuleStatusIndex];
    int32_t  numValsToCopy = numVals;
    if (numVals > capacity) {
        status = U_BUFFER_OVERFLOW_ERROR;
        numValsToCopy = capacity;
    }
    int i;
    for (i=0; i<numValsToCopy; i++) {
        fillInVec[i] = fData->fRuleStatusTable[fLastRuleStatusIndex + i + 1];
    }
    return numVals;
}










const uint8_t  *RuleBasedBreakIterator::getBinaryRules(uint32_t &length) {
    const uint8_t  *retPtr = NULL;
    length = 0;

    if (fData != NULL) {
        retPtr = (const uint8_t *)fData->fHeader;
        length = fData->fHeader->fLength;
    }
    return retPtr;
}














BreakIterator *  RuleBasedBreakIterator::createBufferClone(void *stackBuffer,
                                   int32_t &bufferSize,
                                   UErrorCode &status)
{
    if (U_FAILURE(status)){
        return NULL;
    }

    
    
    
    
    if (bufferSize == 0) {
        bufferSize = sizeof(RuleBasedBreakIterator) + U_ALIGNMENT_OFFSET_UP(0);
        return NULL;
    }


    
    
    
    
    char    *buf   = (char *)stackBuffer;
    uint32_t s      = bufferSize;

    if (stackBuffer == NULL) {
        s = 0;   
    }
    if (U_ALIGNMENT_OFFSET(stackBuffer) != 0) {
        uint32_t offsetUp = (uint32_t)U_ALIGNMENT_OFFSET_UP(buf);
        s   -= offsetUp;
        buf += offsetUp;
    }
    if (s < sizeof(RuleBasedBreakIterator)) {
        
        
        
        RuleBasedBreakIterator *clonedBI = new RuleBasedBreakIterator(*this);
        if (clonedBI == 0) {
            status = U_MEMORY_ALLOCATION_ERROR;
        } else {
            status = U_SAFECLONE_ALLOCATED_WARNING;
        }
        return clonedBI;
    }

    
    
    
    RuleBasedBreakIterator *clone = new(buf) RuleBasedBreakIterator(*this);
    clone->fBufferClone = TRUE;   

    return clone;
}






























int32_t RuleBasedBreakIterator::checkDictionary(int32_t startPos,
                            int32_t endPos,
                            UBool reverse) {
    
    reset();

    
    
    
    if ((endPos - startPos) <= 1) {
        return (reverse ? startPos : endPos);
    }
    
    
    
    
    
    
    
    
    
    
    
    
    static const void *utext_utf8Funcs;
    if (utext_utf8Funcs == NULL) {
        
        UErrorCode status = U_ZERO_ERROR;
        UText tempUText = UTEXT_INITIALIZER; 
        utext_openUTF8(&tempUText, NULL, 0, &status);
        utext_utf8Funcs = tempUText.pFuncs;
        utext_close(&tempUText);
    }
    if (fText->pFuncs == utext_utf8Funcs) {
        return (reverse ? startPos : endPos);
    }

    
    
    
    utext_setNativeIndex(fText, reverse ? endPos : startPos);
    if (reverse) {
        UTEXT_PREVIOUS32(fText);
    }
    
    int32_t rangeStart = startPos;
    int32_t rangeEnd = endPos;

    uint16_t    category;
    int32_t     current;
    UErrorCode  status = U_ZERO_ERROR;
    UStack      breaks(status);
    int32_t     foundBreakCount = 0;
    UChar32     c = utext_current32(fText);

    UTRIE_GET16(&fData->fTrie, c, category);
    
    
    
    
    
    
    if (category & 0x4000) {
        if (reverse) {
            do {
                utext_next32(fText);          
                c = utext_current32(fText);
                UTRIE_GET16(&fData->fTrie, c, category);
            } while (c != U_SENTINEL && (category & 0x4000));
            
            rangeEnd = (int32_t)UTEXT_GETNATIVEINDEX(fText);
            if (c == U_SENTINEL) {
                
                
                c = UTEXT_PREVIOUS32(fText);
            }
            else {
                c = UTEXT_PREVIOUS32(fText);
            }
        }
        else {
            do {
                c = UTEXT_PREVIOUS32(fText);
                UTRIE_GET16(&fData->fTrie, c, category);
            }
            while (c != U_SENTINEL && (category & 0x4000));
            
            if (c == U_SENTINEL) {
                
                c = utext_current32(fText);
            }
            else {
                utext_next32(fText);
                c = utext_current32(fText);
            }
            rangeStart = (int32_t)UTEXT_GETNATIVEINDEX(fText);;
        }
        UTRIE_GET16(&fData->fTrie, c, category);
    }
    
    
    
    
    
    
    if (reverse) {
        utext_setNativeIndex(fText, rangeStart);
        c = utext_current32(fText);
        UTRIE_GET16(&fData->fTrie, c, category);
    }
    while(U_SUCCESS(status)) {
        while((current = (int32_t)UTEXT_GETNATIVEINDEX(fText)) < rangeEnd && (category & 0x4000) == 0) {
            utext_next32(fText);           
            c = utext_current32(fText);
            UTRIE_GET16(&fData->fTrie, c, category);
        }
        if (current >= rangeEnd) {
            break;
        }
        
        
        
        const LanguageBreakEngine *lbe = getLanguageBreakEngine(c);
        
        
        
        if (lbe != NULL) {
            foundBreakCount += lbe->findBreaks(fText, rangeStart, rangeEnd, FALSE, fBreakType, breaks);
        }
        
        
        c = utext_current32(fText);
        UTRIE_GET16(&fData->fTrie, c, category);
    }
    
    
    
    if (foundBreakCount > 0) {
        int32_t totalBreaks = foundBreakCount;
        if (startPos < breaks.elementAti(0)) {
            totalBreaks += 1;
        }
        if (endPos > breaks.peeki()) {
            totalBreaks += 1;
        }
        fCachedBreakPositions = (int32_t *)uprv_malloc(totalBreaks * sizeof(int32_t));
        if (fCachedBreakPositions != NULL) {
            int32_t out = 0;
            fNumCachedBreakPositions = totalBreaks;
            if (startPos < breaks.elementAti(0)) {
                fCachedBreakPositions[out++] = startPos;
            }
            for (int32_t i = 0; i < foundBreakCount; ++i) {
                fCachedBreakPositions[out++] = breaks.elementAti(i);
            }
            if (endPos > fCachedBreakPositions[out-1]) {
                fCachedBreakPositions[out] = endPos;
            }
            
            
            
            if (reverse) {
                return preceding(endPos);
            }
            else {
                return following(startPos);
            }
        }
        
    }

    
    
    utext_setNativeIndex(fText, reverse ? startPos : endPos);
    return (reverse ? startPos : endPos);
}

U_NAMESPACE_END



static icu::UStack *gLanguageBreakFactories = NULL;




U_CDECL_BEGIN
static UBool U_CALLCONV breakiterator_cleanup_dict(void) {
    if (gLanguageBreakFactories) {
        delete gLanguageBreakFactories;
        gLanguageBreakFactories = NULL;
    }
    return TRUE;
}
U_CDECL_END

U_CDECL_BEGIN
static void U_CALLCONV _deleteFactory(void *obj) {
    delete (icu::LanguageBreakFactory *) obj;
}
U_CDECL_END
U_NAMESPACE_BEGIN

static const LanguageBreakEngine*
getLanguageBreakEngineFromFactory(UChar32 c, int32_t breakType)
{
    UBool       needsInit;
    UErrorCode  status = U_ZERO_ERROR;
    UMTX_CHECK(NULL, (UBool)(gLanguageBreakFactories == NULL), needsInit);
    
    if (needsInit) {
        UStack  *factories = new UStack(_deleteFactory, NULL, status);
        if (factories != NULL && U_SUCCESS(status)) {
            ICULanguageBreakFactory *builtIn = new ICULanguageBreakFactory(status);
            factories->push(builtIn, status);
#ifdef U_LOCAL_SERVICE_HOOK
            LanguageBreakFactory *extra = (LanguageBreakFactory *)uprv_svc_hook("languageBreakFactory", &status);
            if (extra != NULL) {
                factories->push(extra, status);
            }
#endif
        }
        umtx_lock(NULL);
        if (gLanguageBreakFactories == NULL) {
            gLanguageBreakFactories = factories;
            factories = NULL;
            ucln_common_registerCleanup(UCLN_COMMON_BREAKITERATOR_DICT, breakiterator_cleanup_dict);
        }
        umtx_unlock(NULL);
        delete factories;
    }
    
    if (gLanguageBreakFactories == NULL) {
        return NULL;
    }
    
    int32_t i = gLanguageBreakFactories->size();
    const LanguageBreakEngine *lbe = NULL;
    while (--i >= 0) {
        LanguageBreakFactory *factory = (LanguageBreakFactory *)(gLanguageBreakFactories->elementAt(i));
        lbe = factory->getEngineFor(c, breakType);
        if (lbe != NULL) {
            break;
        }
    }
    return lbe;
}








const LanguageBreakEngine *
RuleBasedBreakIterator::getLanguageBreakEngine(UChar32 c) {
    const LanguageBreakEngine *lbe = NULL;
    UErrorCode status = U_ZERO_ERROR;
    
    if (fLanguageBreakEngines == NULL) {
        fLanguageBreakEngines = new UStack(status);
        if (fLanguageBreakEngines == NULL || U_FAILURE(status)) {
            delete fLanguageBreakEngines;
            fLanguageBreakEngines = 0;
            return NULL;
        }
    }
    
    int32_t i = fLanguageBreakEngines->size();
    while (--i >= 0) {
        lbe = (const LanguageBreakEngine *)(fLanguageBreakEngines->elementAt(i));
        if (lbe->handles(c, fBreakType)) {
            return lbe;
        }
    }
    
    
    
    lbe = getLanguageBreakEngineFromFactory(c, fBreakType);
    
    
    if (lbe != NULL) {
        fLanguageBreakEngines->push((void *)lbe, status);
        
        
        return lbe;
    }
    
    
    
    if (fUnhandledBreakEngine == NULL) {
        fUnhandledBreakEngine = new UnhandledEngine(status);
        if (U_SUCCESS(status) && fUnhandledBreakEngine == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
        }
        
        
        fLanguageBreakEngines->insertElementAt(fUnhandledBreakEngine, 0, status);
        
        if (U_FAILURE(status)) {
            delete fUnhandledBreakEngine;
            fUnhandledBreakEngine = 0;
            return NULL;
        }
    }
    
    
    
    fUnhandledBreakEngine->handleCharacter(c, fBreakType);
        
    return fUnhandledBreakEngine;
}







void RuleBasedBreakIterator::setBreakType(int32_t type) {
    fBreakType = type;
    reset();
}

U_NAMESPACE_END

#endif 

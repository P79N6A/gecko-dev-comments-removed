



















#include "unicode/utypes.h"
#include "unicode/uspoof.h"

#if !UCONFIG_NO_NORMALIZATION

#if !UCONFIG_NO_REGULAR_EXPRESSIONS 

#include "unicode/unorm.h"
#include "unicode/uregex.h"
#include "unicode/ustring.h"
#include "cmemory.h"
#include "uspoof_impl.h"
#include "uhash.h"
#include "uvector.h"
#include "uassert.h"
#include "uspoof_wsconf.h"

U_NAMESPACE_USE















static const char *parseExp = 
        "(?m)"                                         
        "^([ \\t]*(?:#.*?)?)$"                         
        "|^(?:"                                        
        "\\s*([0-9A-F]{4,})(?:..([0-9A-F]{4,}))?\\s*;" 
        "\\s*([A-Za-z]+)\\s*;"                         
        "\\s*([A-Za-z]+)\\s*;"                         
        "\\s*(?:(A)|(L))"                              
        "[ \\t]*(?:#.*?)?"                             
        ")$|"                                          
        "^(.*?)$";                                     
                                                       
                                                       
                                                       






static void extractGroup(
    URegularExpression *e, int32_t group, char *destBuf, int32_t destCapacity, UErrorCode &status) {

    UChar ubuf[50];
    ubuf[0] = 0;
    destBuf[0] = 0;
    int32_t len = uregex_group(e, group, ubuf, 50, &status);
    if (U_FAILURE(status) || len == -1 || len >= destCapacity) {
        return;
    }
    UnicodeString s(FALSE, ubuf, len);   
    s.extract(0, len, destBuf, destCapacity, US_INV);
}



U_NAMESPACE_BEGIN









void buildWSConfusableData(SpoofImpl *spImpl, const char * confusablesWS,
          int32_t confusablesWSLen, UParseError *pe, UErrorCode &status) 
{
    if (U_FAILURE(status)) {
        return;
    }
    URegularExpression *parseRegexp = NULL;
    int32_t             inputLen    = 0;
    UChar              *input       = NULL;
    int32_t             lineNum     = 0;
    
    UVector            *scriptSets        = NULL;
    uint32_t            rtScriptSetsCount = 2;

    UTrie2             *anyCaseTrie   = NULL;
    UTrie2             *lowerCaseTrie = NULL;

    anyCaseTrie = utrie2_open(0, 0, &status);
    lowerCaseTrie = utrie2_open(0, 0, &status);

    UnicodeString pattern(parseExp, -1, US_INV);

    
    
    
    
    
    
    
    
    
    
    
    
    scriptSets = new UVector(status);
    if (scriptSets == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        goto cleanup;
    }
    scriptSets->addElement((void *)NULL, status);
    scriptSets->addElement((void *)NULL, status);

    
    u_strFromUTF8(NULL, 0, &inputLen, confusablesWS, confusablesWSLen, &status);
    if (status != U_BUFFER_OVERFLOW_ERROR) {
        goto cleanup;
    }
    status = U_ZERO_ERROR;
    input = static_cast<UChar *>(uprv_malloc((inputLen+1) * sizeof(UChar)));
    if (input == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        goto cleanup;
    }
    u_strFromUTF8(input, inputLen+1, NULL, confusablesWS, confusablesWSLen, &status);

    parseRegexp = uregex_open(pattern.getBuffer(), pattern.length(), 0, NULL, &status);

    
    
    if (*input == 0xfeff) {
        *input = 0x20;
    }

    
    uregex_setText(parseRegexp, input, inputLen, &status);
    while (uregex_findNext(parseRegexp, &status)) {
        lineNum++;
        if (uregex_start(parseRegexp, 1, &status) >= 0) {
            
            continue;
        }
        if (uregex_start(parseRegexp, 8, &status) >= 0) {
            
            status = U_PARSE_ERROR;
            goto cleanup;
        }
        if (U_FAILURE(status)) {
            goto cleanup;
        }

        
        UChar32  startCodePoint = SpoofImpl::ScanHex(
            input, uregex_start(parseRegexp, 2, &status), uregex_end(parseRegexp, 2, &status), status);
        UChar32  endCodePoint = startCodePoint;
        if (uregex_start(parseRegexp, 3, &status) >=0) {
            endCodePoint = SpoofImpl::ScanHex(
                input, uregex_start(parseRegexp, 3, &status), uregex_end(parseRegexp, 3, &status), status);
        }

        
        
        
        char  srcScriptName[20];
        char  targScriptName[20];
        extractGroup(parseRegexp, 4, srcScriptName, sizeof(srcScriptName), status);
        extractGroup(parseRegexp, 5, targScriptName, sizeof(targScriptName), status);
        UScriptCode srcScript  =
            static_cast<UScriptCode>(u_getPropertyValueEnum(UCHAR_SCRIPT, srcScriptName));
        UScriptCode targScript =
            static_cast<UScriptCode>(u_getPropertyValueEnum(UCHAR_SCRIPT, targScriptName));
        if (U_FAILURE(status)) {
            goto cleanup;
        }
        if (srcScript == USCRIPT_INVALID_CODE || targScript == USCRIPT_INVALID_CODE) {
            status = U_INVALID_FORMAT_ERROR;
            goto cleanup;
        }

        
        UTrie2 *table = anyCaseTrie;
        if (uregex_start(parseRegexp, 7, &status) >= 0) {
            table = lowerCaseTrie;
        }

        
        
        
        
        
        
        
        UChar32 cp;
        for (cp=startCodePoint; cp<=endCodePoint; cp++) {
            int32_t setIndex = utrie2_get32(table, cp);
            BuilderScriptSet *bsset = NULL;
            if (setIndex > 0) {
                U_ASSERT(setIndex < scriptSets->size());
                bsset = static_cast<BuilderScriptSet *>(scriptSets->elementAt(setIndex));
            } else {
                bsset = new BuilderScriptSet();
                if (bsset == NULL) {
                    status = U_MEMORY_ALLOCATION_ERROR;
                    goto cleanup;
                }
                bsset->codePoint = cp;
                bsset->trie = table;
                bsset->sset = new ScriptSet();
                setIndex = scriptSets->size();
                bsset->index = setIndex;
                bsset->rindex = 0;
                if (bsset->sset == NULL) {
                    status = U_MEMORY_ALLOCATION_ERROR;
                    goto cleanup;
                }
                scriptSets->addElement(bsset, status);
                utrie2_set32(table, cp, setIndex, &status);
            }
            bsset->sset->Union(targScript);
            bsset->sset->Union(srcScript);

            if (U_FAILURE(status)) {
                goto cleanup;
            }
            UScriptCode cpScript = uscript_getScript(cp, &status);
            if (cpScript != srcScript) {
                status = U_INVALID_FORMAT_ERROR;
                goto cleanup;
            }
        }
    }

    
    
    
    
    
    
    {
        int32_t duplicateCount = 0;
        rtScriptSetsCount = 2;
        for (int32_t outeri=2; outeri<scriptSets->size(); outeri++) {
            BuilderScriptSet *outerSet = static_cast<BuilderScriptSet *>(scriptSets->elementAt(outeri));
            if (outerSet->index != static_cast<uint32_t>(outeri)) {
                
                
                continue;
            }
            outerSet->rindex = rtScriptSetsCount++;
            for (int32_t inneri=outeri+1; inneri<scriptSets->size(); inneri++) {
                BuilderScriptSet *innerSet = static_cast<BuilderScriptSet *>(scriptSets->elementAt(inneri));
                if (*(outerSet->sset) == *(innerSet->sset) && outerSet->sset != innerSet->sset) {
                    delete innerSet->sset;
                    innerSet->scriptSetOwned = FALSE;
                    innerSet->sset = outerSet->sset;
                    innerSet->index = outeri;
                    innerSet->rindex = outerSet->rindex;
                    duplicateCount++;
                }
                
            }
        }
        
    }

    

    
    
    
    {
        for (int32_t i=2; i<scriptSets->size(); i++) {
            BuilderScriptSet *bSet = static_cast<BuilderScriptSet *>(scriptSets->elementAt(i));
            if (bSet->rindex != (uint32_t)i) {
                utrie2_set32(bSet->trie, bSet->codePoint, bSet->rindex, &status);
            }
        }
    }

    
    
    
    
    {
        UnicodeSet ignoreSet;
        ignoreSet.applyIntPropertyValue(UCHAR_SCRIPT, USCRIPT_COMMON, status);
        UnicodeSet inheritedSet;
        inheritedSet.applyIntPropertyValue(UCHAR_SCRIPT, USCRIPT_INHERITED, status);
        ignoreSet.addAll(inheritedSet);
        for (int32_t rn=0; rn<ignoreSet.getRangeCount(); rn++) {
            UChar32 rangeStart = ignoreSet.getRangeStart(rn);
            UChar32 rangeEnd   = ignoreSet.getRangeEnd(rn);
            utrie2_setRange32(anyCaseTrie,   rangeStart, rangeEnd, 1, TRUE, &status);
            utrie2_setRange32(lowerCaseTrie, rangeStart, rangeEnd, 1, TRUE, &status);
        }
    }

    
    {
        utrie2_freeze(anyCaseTrie,   UTRIE2_16_VALUE_BITS, &status);
        int32_t size = utrie2_serialize(anyCaseTrie, NULL, 0, &status);
        
        if (status != U_BUFFER_OVERFLOW_ERROR) {
            goto cleanup;
        }
        status = U_ZERO_ERROR;
        spImpl->fSpoofData->fRawData->fAnyCaseTrie = spImpl->fSpoofData->fMemLimit;
        spImpl->fSpoofData->fRawData->fAnyCaseTrieLength = size;
        spImpl->fSpoofData->fAnyCaseTrie = anyCaseTrie;
        void *where = spImpl->fSpoofData->reserveSpace(size, status);
        utrie2_serialize(anyCaseTrie, where, size, &status);
        
        utrie2_freeze(lowerCaseTrie, UTRIE2_16_VALUE_BITS, &status);
        size = utrie2_serialize(lowerCaseTrie, NULL, 0, &status);
        
        if (status != U_BUFFER_OVERFLOW_ERROR) {
            goto cleanup;
        }
        status = U_ZERO_ERROR;
        spImpl->fSpoofData->fRawData->fLowerCaseTrie = spImpl->fSpoofData->fMemLimit;
        spImpl->fSpoofData->fRawData->fLowerCaseTrieLength = size;
        spImpl->fSpoofData->fLowerCaseTrie = lowerCaseTrie;
        where = spImpl->fSpoofData->reserveSpace(size, status);
        utrie2_serialize(lowerCaseTrie, where, size, &status);

        spImpl->fSpoofData->fRawData->fScriptSets = spImpl->fSpoofData->fMemLimit;
        spImpl->fSpoofData->fRawData->fScriptSetsLength = rtScriptSetsCount;
        ScriptSet *rtScriptSets =  static_cast<ScriptSet *>
            (spImpl->fSpoofData->reserveSpace(rtScriptSetsCount * sizeof(ScriptSet), status));
        uint32_t rindex = 2;
        for (int32_t i=2; i<scriptSets->size(); i++) {
            BuilderScriptSet *bSet = static_cast<BuilderScriptSet *>(scriptSets->elementAt(i));
            if (bSet->rindex < rindex) {
                
                continue;
            }
            U_ASSERT(rindex == bSet->rindex);
            rtScriptSets[rindex] = *bSet->sset;   
            rindex++;
        }
    }

    
    
    
    
    
    
    
    spImpl->fSpoofData->fAnyCaseTrie = utrie2_openFromSerialized(
            UTRIE2_16_VALUE_BITS,
            (const char *)spImpl->fSpoofData->fRawData + spImpl->fSpoofData->fRawData->fAnyCaseTrie,
            spImpl->fSpoofData->fRawData->fAnyCaseTrieLength,
            NULL,
            &status);

    spImpl->fSpoofData->fLowerCaseTrie = utrie2_openFromSerialized(
            UTRIE2_16_VALUE_BITS,
            (const char *)spImpl->fSpoofData->fRawData + spImpl->fSpoofData->fRawData->fLowerCaseTrie,
            spImpl->fSpoofData->fRawData->fAnyCaseTrieLength,
            NULL,
            &status);

    

cleanup:
    if (U_FAILURE(status)) {
        pe->line = lineNum;
    }
    uregex_close(parseRegexp);
    uprv_free(input);

    int32_t i;
    if (scriptSets != NULL) {
        for (i=0; i<scriptSets->size(); i++) {
            BuilderScriptSet *bsset = static_cast<BuilderScriptSet *>(scriptSets->elementAt(i));
            delete bsset;
        }
        delete scriptSets;
    }
    utrie2_close(anyCaseTrie);
    utrie2_close(lowerCaseTrie);
    return;
}

U_NAMESPACE_END



BuilderScriptSet::BuilderScriptSet() {
    codePoint = -1;
    trie = NULL;
    sset = NULL;
    index = 0;
    rindex = 0;
    scriptSetOwned = TRUE;
}

BuilderScriptSet::~BuilderScriptSet() {
    if (scriptSetOwned) {
        delete sset;
    }
}

#endif
#endif 


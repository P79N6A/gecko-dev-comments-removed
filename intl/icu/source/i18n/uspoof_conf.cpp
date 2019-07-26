

















#include "unicode/utypes.h"
#include "unicode/uspoof.h"
#if !UCONFIG_NO_REGULAR_EXPRESSIONS
#if !UCONFIG_NO_NORMALIZATION

#include "unicode/unorm.h"
#include "unicode/uregex.h"
#include "unicode/ustring.h"
#include "cmemory.h"
#include "uspoof_impl.h"
#include "uhash.h"
#include "uvector.h"
#include "uassert.h"
#include "uarrsort.h"
#include "uspoof_conf.h"

U_NAMESPACE_USE


























SPUString::SPUString(UnicodeString *s) {
    fStr = s;
    fStrTableIndex = 0;
}


SPUString::~SPUString() {
    delete fStr;
}


SPUStringPool::SPUStringPool(UErrorCode &status) : fVec(NULL), fHash(NULL) {
    fVec = new UVector(status);
    fHash = uhash_open(uhash_hashUnicodeString,           
                       uhash_compareUnicodeString,        
                       NULL,                              
                       &status);
}


SPUStringPool::~SPUStringPool() {
    int i;
    for (i=fVec->size()-1; i>=0; i--) {
        SPUString *s = static_cast<SPUString *>(fVec->elementAt(i));
        delete s;
    }
    delete fVec;
    uhash_close(fHash);
}


int32_t SPUStringPool::size() {
    return fVec->size();
}

SPUString *SPUStringPool::getByIndex(int32_t index) {
    SPUString *retString = (SPUString *)fVec->elementAt(index);
    return retString;
}







static int8_t U_CALLCONV SPUStringCompare(UHashTok left, UHashTok right) {
	const SPUString *sL = const_cast<const SPUString *>(
        static_cast<SPUString *>(left.pointer));
 	const SPUString *sR = const_cast<const SPUString *>(
 	    static_cast<SPUString *>(right.pointer));
    int32_t lenL = sL->fStr->length();
    int32_t lenR = sR->fStr->length();
    if (lenL < lenR) {
        return -1;
    } else if (lenL > lenR) {
        return 1;
    } else {
        return sL->fStr->compare(*(sR->fStr));
    }
}

void SPUStringPool::sort(UErrorCode &status) {
    fVec->sort(SPUStringCompare, status);
}


SPUString *SPUStringPool::addString(UnicodeString *src, UErrorCode &status) {
    SPUString *hashedString = static_cast<SPUString *>(uhash_get(fHash, src));
    if (hashedString != NULL) {
        delete src;
    } else {
        hashedString = new SPUString(src);
        uhash_put(fHash, src, hashedString, &status);
        fVec->addElement(hashedString, status);
    }
    return hashedString;
}



ConfusabledataBuilder::ConfusabledataBuilder(SpoofImpl *spImpl, UErrorCode &status) :
    fSpoofImpl(spImpl),
    fInput(NULL),
    fSLTable(NULL),
    fSATable(NULL),
    fMLTable(NULL),
    fMATable(NULL),
    fKeySet(NULL),
    fKeyVec(NULL),
    fValueVec(NULL),
    fStringTable(NULL),
    fStringLengthsTable(NULL),
    stringPool(NULL),
    fParseLine(NULL),
    fParseHexNum(NULL),
    fLineNum(0)
{
    if (U_FAILURE(status)) {
        return;
    }
    fSLTable    = uhash_open(uhash_hashLong, uhash_compareLong, NULL, &status);
    fSATable    = uhash_open(uhash_hashLong, uhash_compareLong, NULL, &status);
    fMLTable    = uhash_open(uhash_hashLong, uhash_compareLong, NULL, &status);
    fMATable    = uhash_open(uhash_hashLong, uhash_compareLong, NULL, &status);
    fKeySet     = new UnicodeSet();
    fKeyVec     = new UVector(status);
    fValueVec   = new UVector(status);
    stringPool = new SPUStringPool(status);
}


ConfusabledataBuilder::~ConfusabledataBuilder() {
    uprv_free(fInput);
    uregex_close(fParseLine);
    uregex_close(fParseHexNum);
    uhash_close(fSLTable);
    uhash_close(fSATable);
    uhash_close(fMLTable);
    uhash_close(fMATable);
    delete fKeySet;
    delete fKeyVec;
    delete fStringTable;
    delete fStringLengthsTable;
    delete fValueVec;
    delete stringPool;
}


void ConfusabledataBuilder::buildConfusableData(SpoofImpl * spImpl, const char * confusables,
    int32_t confusablesLen, int32_t *errorType, UParseError *pe, UErrorCode &status) {

    if (U_FAILURE(status)) {
        return;
    }
    ConfusabledataBuilder builder(spImpl, status);
    builder.build(confusables, confusablesLen, status);
    if (U_FAILURE(status) && errorType != NULL) {
        *errorType = USPOOF_SINGLE_SCRIPT_CONFUSABLE;
        pe->line = builder.fLineNum;
    }
}


void ConfusabledataBuilder::build(const char * confusables, int32_t confusablesLen,
               UErrorCode &status) {

    
    int32_t inputLen = 0;
    if (U_FAILURE(status)) {
        return;
    }
    u_strFromUTF8(NULL, 0, &inputLen, confusables, confusablesLen, &status);
    if (status != U_BUFFER_OVERFLOW_ERROR) {
        return;
    }
    status = U_ZERO_ERROR;
    fInput = static_cast<UChar *>(uprv_malloc((inputLen+1) * sizeof(UChar)));
    if (fInput == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    u_strFromUTF8(fInput, inputLen+1, NULL, confusables, confusablesLen, &status);


    
    
    
    
    
    
    
    
    
    UnicodeString pattern(
        "(?m)^[ \\t]*([0-9A-Fa-f]+)[ \\t]+;"      
        "[ \\t]*([0-9A-Fa-f]+"                    
           "(?:[ \\t]+[0-9A-Fa-f]+)*)[ \\t]*;"    
        "\\s*(?:(SL)|(SA)|(ML)|(MA))"             
        "[ \\t]*(?:#.*?)?$"                       
        "|^([ \\t]*(?:#.*?)?)$"       
        "|^(.*?)$", -1, US_INV);      
    
    fParseLine = uregex_open(pattern.getBuffer(), pattern.length(), 0, NULL, &status);

    
    
    pattern = UNICODE_STRING_SIMPLE("\\s*([0-9A-F]+)");
    fParseHexNum = uregex_open(pattern.getBuffer(), pattern.length(), 0, NULL, &status);

    
    
    if (*fInput == 0xfeff) {
        *fInput = 0x20;
    }

    
    uregex_setText(fParseLine, fInput, inputLen, &status);
    while (uregex_findNext(fParseLine, &status)) {
        fLineNum++;
        if (uregex_start(fParseLine, 7, &status) >= 0) {
            
            continue;
        }
        if (uregex_start(fParseLine, 8, &status) >= 0) {
            
            status = U_PARSE_ERROR;
            return;
        }

        
        
        UChar32 keyChar = SpoofImpl::ScanHex(fInput, uregex_start(fParseLine, 1, &status),
                          uregex_end(fParseLine, 1, &status), status);

        int32_t mapStringStart = uregex_start(fParseLine, 2, &status);
        int32_t mapStringLength = uregex_end(fParseLine, 2, &status) - mapStringStart;
        uregex_setText(fParseHexNum, &fInput[mapStringStart], mapStringLength, &status);

        UnicodeString  *mapString = new UnicodeString();
        if (mapString == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        while (uregex_findNext(fParseHexNum, &status)) {
            UChar32 c = SpoofImpl::ScanHex(&fInput[mapStringStart], uregex_start(fParseHexNum, 1, &status),
                                 uregex_end(fParseHexNum, 1, &status), status);
            mapString->append(c);
        }
        U_ASSERT(mapString->length() >= 1);

        
        
        SPUString *smapString = stringPool->addString(mapString, status);

        
        UHashtable *table = uregex_start(fParseLine, 3, &status) >= 0 ? fSLTable :
                            uregex_start(fParseLine, 4, &status) >= 0 ? fSATable :
                            uregex_start(fParseLine, 5, &status) >= 0 ? fMLTable :
                            uregex_start(fParseLine, 6, &status) >= 0 ? fMATable :
                            NULL;
        U_ASSERT(table != NULL);
        uhash_iput(table, keyChar, smapString, &status);
        fKeySet->add(keyChar);
        if (U_FAILURE(status)) {
            return;
        }
    }

    
    
    
    
    
    

    
    
    
    
    
    
    stringPool->sort(status);
    fStringTable = new UnicodeString();
    fStringLengthsTable = new UVector(status);
    int32_t previousStringLength = 0;
    int32_t previousStringIndex  = 0;
    int32_t poolSize = stringPool->size();
    int32_t i;
    for (i=0; i<poolSize; i++) {
        SPUString *s = stringPool->getByIndex(i);
        int32_t strLen = s->fStr->length();
        int32_t strIndex = fStringTable->length();
        U_ASSERT(strLen >= previousStringLength);
        if (strLen == 1) {
            
            
            
            s->fStrTableIndex = s->fStr->charAt(0);
        } else {
            if ((strLen > previousStringLength) && (previousStringLength >= 4)) {
                fStringLengthsTable->addElement(previousStringIndex, status);
                fStringLengthsTable->addElement(previousStringLength, status);
            }
            s->fStrTableIndex = strIndex;
            fStringTable->append(*(s->fStr));
        }
        previousStringLength = strLen;
        previousStringIndex  = strIndex;
    }
    
    
    
    if (previousStringLength >= 4) {
        fStringLengthsTable->addElement(previousStringIndex, status);
        fStringLengthsTable->addElement(previousStringLength, status);
    }

    
    
    
    
    
    
    
    
    
    

    for (int32_t range=0; range<fKeySet->getRangeCount(); range++) {
        
        
        for (UChar32 keyChar=fKeySet->getRangeStart(range);
                keyChar <= fKeySet->getRangeEnd(range); keyChar++) {
            addKeyEntry(keyChar, fSLTable, USPOOF_SL_TABLE_FLAG, status);
            addKeyEntry(keyChar, fSATable, USPOOF_SA_TABLE_FLAG, status);
            addKeyEntry(keyChar, fMLTable, USPOOF_ML_TABLE_FLAG, status);
            addKeyEntry(keyChar, fMATable, USPOOF_MA_TABLE_FLAG, status);
        }
    }

    
    outputData(status);

    
    
    return;
}










void ConfusabledataBuilder::outputData(UErrorCode &status) {

    U_ASSERT(fSpoofImpl->fSpoofData->fDataOwned == TRUE);

    
    
    

    int32_t numKeys = fKeyVec->size();
    int32_t *keys =
        static_cast<int32_t *>(fSpoofImpl->fSpoofData->reserveSpace(numKeys*sizeof(int32_t), status));
    if (U_FAILURE(status)) {
        return;
    }
    int i;
    int32_t previousKey = 0;
    for (i=0; i<numKeys; i++) {
        int32_t key =  fKeyVec->elementAti(i);
        U_ASSERT((key & 0x00ffffff) >= (previousKey & 0x00ffffff));
        U_ASSERT((key & 0xff000000) != 0);
        keys[i] = key;
        previousKey = key;
    }
    SpoofDataHeader *rawData = fSpoofImpl->fSpoofData->fRawData;
    rawData->fCFUKeys = (int32_t)((char *)keys - (char *)rawData);
    rawData->fCFUKeysSize = numKeys;
    fSpoofImpl->fSpoofData->fCFUKeys = keys;


    
    int32_t numValues = fValueVec->size();
    U_ASSERT(numKeys == numValues);
    uint16_t *values =
        static_cast<uint16_t *>(fSpoofImpl->fSpoofData->reserveSpace(numKeys*sizeof(uint16_t), status));
    if (U_FAILURE(status)) {
        return;
    }
    for (i=0; i<numValues; i++) {
        uint32_t value = static_cast<uint32_t>(fValueVec->elementAti(i));
        U_ASSERT(value < 0xffff);
        values[i] = static_cast<uint16_t>(value);
    }
    rawData = fSpoofImpl->fSpoofData->fRawData;
    rawData->fCFUStringIndex = (int32_t)((char *)values - (char *)rawData);
    rawData->fCFUStringIndexSize = numValues;
    fSpoofImpl->fSpoofData->fCFUValues = values;

    

    uint32_t stringsLength = fStringTable->length();
    
    
    UChar *strings =
        static_cast<UChar *>(fSpoofImpl->fSpoofData->reserveSpace(stringsLength*sizeof(UChar)+2, status));
    if (U_FAILURE(status)) {
        return;
    }
    fStringTable->extract(strings, stringsLength+1, status);
    rawData = fSpoofImpl->fSpoofData->fRawData;
    U_ASSERT(rawData->fCFUStringTable == 0);
    rawData->fCFUStringTable = (int32_t)((char *)strings - (char *)rawData);
    rawData->fCFUStringTableLen = stringsLength;
    fSpoofImpl->fSpoofData->fCFUStrings = strings;

    
    
    
    
    
    int32_t lengthTableLength = fStringLengthsTable->size();
    uint16_t *stringLengths =
        static_cast<uint16_t *>(fSpoofImpl->fSpoofData->reserveSpace(lengthTableLength*sizeof(uint16_t), status));
    if (U_FAILURE(status)) {
        return;
    }
    int32_t destIndex = 0;
    uint32_t previousLength = 0;
    for (i=0; i<lengthTableLength; i+=2) {
        uint32_t offset = static_cast<uint32_t>(fStringLengthsTable->elementAti(i));
        uint32_t length = static_cast<uint32_t>(fStringLengthsTable->elementAti(i+1));
        U_ASSERT(offset < stringsLength);
        U_ASSERT(length < 40);
        U_ASSERT(length > previousLength);
        stringLengths[destIndex++] = static_cast<uint16_t>(offset);
        stringLengths[destIndex++] = static_cast<uint16_t>(length);
        previousLength = length;
    }
    rawData = fSpoofImpl->fSpoofData->fRawData;
    rawData->fCFUStringLengths = (int32_t)((char *)stringLengths - (char *)rawData);
    
    
    rawData->fCFUStringLengthsSize = lengthTableLength / 2;
    fSpoofImpl->fSpoofData->fCFUStringLengths =
        reinterpret_cast<SpoofStringLengthsElement *>(stringLengths);
}









void ConfusabledataBuilder::addKeyEntry(
    UChar32     keyChar,     
    UHashtable *table,       
    int32_t     tableFlag,   
    UErrorCode &status) {

    SPUString *targetMapping = static_cast<SPUString *>(uhash_iget(table, keyChar));
    if (targetMapping == NULL) {
        
        
        
        return;
    }

    
    
    

    UBool keyHasMultipleValues = FALSE;
    int32_t i;
    for (i=fKeyVec->size()-1; i>=0 ; i--) {
        int32_t key = fKeyVec->elementAti(i);
        if ((key & 0x0ffffff) != keyChar) {
            
            
            break;
        }
        UnicodeString mapping = getMapping(i);
        if (mapping == *(targetMapping->fStr)) {
            
            
            key |= tableFlag;
            fKeyVec->setElementAt(key, i);
            return;
        }
        keyHasMultipleValues = TRUE;
    }

    
    

    int32_t newKey = keyChar | tableFlag;
    if (keyHasMultipleValues) {
        newKey |= USPOOF_KEY_MULTIPLE_VALUES;
    }
    int32_t adjustedMappingLength = targetMapping->fStr->length() - 1;
    if (adjustedMappingLength>3) {
        adjustedMappingLength = 3;
    }
    newKey |= adjustedMappingLength << USPOOF_KEY_LENGTH_SHIFT;

    int32_t newData = targetMapping->fStrTableIndex;

    fKeyVec->addElement(newKey, status);
    fValueVec->addElement(newData, status);

    
    
    if (keyHasMultipleValues) {
        int32_t previousKeyIndex = fKeyVec->size() - 2;
        int32_t previousKey = fKeyVec->elementAti(previousKeyIndex);
        previousKey |= USPOOF_KEY_MULTIPLE_VALUES;
        fKeyVec->setElementAt(previousKey, previousKeyIndex);
    }
}



UnicodeString ConfusabledataBuilder::getMapping(int32_t index) {
    int32_t key = fKeyVec->elementAti(index);
    int32_t value = fValueVec->elementAti(index);
    int32_t length = USPOOF_KEY_LENGTH_FIELD(key);
    int32_t lastIndexWithLen;
    switch (length) {
      case 0:
        return UnicodeString(static_cast<UChar>(value));
      case 1:
      case 2:
        return UnicodeString(*fStringTable, value, length+1);
      case 3:
        length = 0;
        int32_t i;
        for (i=0; i<fStringLengthsTable->size(); i+=2) {
            lastIndexWithLen = fStringLengthsTable->elementAti(i);
            if (value <= lastIndexWithLen) {
                length = fStringLengthsTable->elementAti(i+1);
                break;
            }
        }
        U_ASSERT(length>=3);
        return UnicodeString(*fStringTable, value, length);
      default:
        U_ASSERT(FALSE);
    }
    return UnicodeString();
}

#endif
#endif 









#include "unicode/utypes.h"
#include "unicode/uspoof.h"
#include "unicode/unorm.h"
#include "unicode/uchar.h"
#include "unicode/uniset.h"
#include "unicode/utf16.h"
#include "utrie2.h"
#include "cmemory.h"
#include "cstring.h"
#include "udatamem.h"
#include "umutex.h"
#include "udataswp.h"
#include "uassert.h"
#include "uspoof_impl.h"

#if !UCONFIG_NO_NORMALIZATION


U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(SpoofImpl)

SpoofImpl::SpoofImpl(SpoofData *data, UErrorCode &status) :
    fMagic(0), fSpoofData(NULL), fAllowedCharsSet(NULL) , fAllowedLocales(uprv_strdup("")) {
    if (U_FAILURE(status)) {
        return;
    }
    fMagic = USPOOF_MAGIC;
    fSpoofData = data;
    fChecks = USPOOF_ALL_CHECKS;
    UnicodeSet *allowedCharsSet = new UnicodeSet(0, 0x10ffff);
    if (allowedCharsSet == NULL || fAllowedLocales == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    allowedCharsSet->freeze();
    fAllowedCharsSet = allowedCharsSet;
}


SpoofImpl::SpoofImpl() {
    fMagic = USPOOF_MAGIC;
    fSpoofData = NULL;
    fChecks = USPOOF_ALL_CHECKS;
    UnicodeSet *allowedCharsSet = new UnicodeSet(0, 0x10ffff);
    allowedCharsSet->freeze();
    fAllowedCharsSet = allowedCharsSet;
    fAllowedLocales  = uprv_strdup("");
}



SpoofImpl::SpoofImpl(const SpoofImpl &src, UErrorCode &status)  :
    fMagic(0), fSpoofData(NULL), fAllowedCharsSet(NULL) {
    if (U_FAILURE(status)) {
        return;
    }
    fMagic = src.fMagic;
    fChecks = src.fChecks;
    if (src.fSpoofData != NULL) {
        fSpoofData = src.fSpoofData->addReference();
    }
    fAllowedCharsSet = static_cast<const UnicodeSet *>(src.fAllowedCharsSet->clone());
    if (fAllowedCharsSet == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    fAllowedLocales = uprv_strdup(src.fAllowedLocales);
}

SpoofImpl::~SpoofImpl() {
    fMagic = 0;                
                               
    if (fSpoofData != NULL) {
        fSpoofData->removeReference();   
    }
    delete fAllowedCharsSet;
    uprv_free((void *)fAllowedLocales);
}





const SpoofImpl *SpoofImpl::validateThis(const USpoofChecker *sc, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return NULL;
    }
    if (sc == NULL) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    };
    SpoofImpl *This = (SpoofImpl *)sc;
    if (This->fMagic != USPOOF_MAGIC ||
        This->fSpoofData == NULL) {
        status = U_INVALID_FORMAT_ERROR;
        return NULL;
    }
    if (!SpoofData::validateDataVersion(This->fSpoofData->fRawData, status)) {
        return NULL;
    }
    return This;
}

SpoofImpl *SpoofImpl::validateThis(USpoofChecker *sc, UErrorCode &status) {
    return const_cast<SpoofImpl *>
        (SpoofImpl::validateThis(const_cast<const USpoofChecker *>(sc), status));
}












int32_t SpoofImpl::confusableLookup(UChar32 inChar, int32_t tableMask, UChar *destBuf) const {

    
    int32_t  *low   = fSpoofData->fCFUKeys;
    int32_t  *mid   = NULL;
    int32_t  *limit = low + fSpoofData->fRawData->fCFUKeysSize;
    UChar32   midc;
    do {
        int32_t delta = ((int32_t)(limit-low))/2;
        mid = low + delta;
        midc = *mid & 0x1fffff;
        if (inChar == midc) {
            goto foundChar;
        } else if (inChar < midc) {
            limit = mid;
        } else {
            low = mid;
        }
    } while (low < limit-1);
    mid = low;
    midc = *mid & 0x1fffff;
    if (inChar != midc) {
        
        int i = 0;
        U16_APPEND_UNSAFE(destBuf, i, inChar)
        return i;
    } 
  foundChar:
    int32_t keyFlags = *mid & 0xff000000;
    if ((keyFlags & tableMask) == 0) {
        
        
        if (keyFlags & USPOOF_KEY_MULTIPLE_VALUES) {
            int32_t *altMid;
            for (altMid = mid-1; (*altMid&0x00ffffff) == inChar; altMid--) {
                keyFlags = *altMid & 0xff000000;
                if (keyFlags & tableMask) {
                    mid = altMid;
                    goto foundKey;
                }
            }
            for (altMid = mid+1; (*altMid&0x00ffffff) == inChar; altMid++) {
                keyFlags = *altMid & 0xff000000;
                if (keyFlags & tableMask) {
                    mid = altMid;
                    goto foundKey;
                }
            }
        }
        
        
        int i = 0;
        U16_APPEND_UNSAFE(destBuf, i, inChar)
        return i;
    }

  foundKey:
    int32_t  stringLen = USPOOF_KEY_LENGTH_FIELD(keyFlags) + 1;
    int32_t keyTableIndex = (int32_t)(mid - fSpoofData->fCFUKeys);

    
    
    uint16_t value = fSpoofData->fCFUValues[keyTableIndex];
    if (stringLen == 1) {
        destBuf[0] = value;
        return 1;
    }

    
    
    
    
    

    int32_t ix;
    if (stringLen == 4) {
        int32_t stringLengthsLimit = fSpoofData->fRawData->fCFUStringLengthsSize;
        for (ix = 0; ix < stringLengthsLimit; ix++) {
            if (fSpoofData->fCFUStringLengths[ix].fLastString >= value) {
                stringLen = fSpoofData->fCFUStringLengths[ix].fStrLength;
                break;
            }
        }
        U_ASSERT(ix < stringLengthsLimit);
    }

    U_ASSERT(value + stringLen <= fSpoofData->fRawData->fCFUStringTableLen);
    UChar *src = &fSpoofData->fCFUStrings[value];
    for (ix=0; ix<stringLen; ix++) {
        destBuf[ix] = src[ix];
    }
    return stringLen;
}













void SpoofImpl::wholeScriptCheck(
    const UChar *text, int32_t length, ScriptSet *result, UErrorCode &status) const {

    int32_t       inputIdx = 0;
    UChar32       c;

    UTrie2 *table =
        (fChecks & USPOOF_ANY_CASE) ? fSpoofData->fAnyCaseTrie : fSpoofData->fLowerCaseTrie;
    result->setAll();
    while (inputIdx < length) {
        U16_NEXT(text, inputIdx, length, c);
        uint32_t index = utrie2_get32(table, c);
        if (index == 0) {
            
            
            
            
            UScriptCode cpScript = uscript_getScript(c, &status);
            U_ASSERT(cpScript > USCRIPT_INHERITED);
            result->intersect(cpScript);
        } else if (index == 1) {
            
        } else {
            result->intersect(fSpoofData->fScriptSets[index]);
        }
    }
}


void SpoofImpl::setAllowedLocales(const char *localesList, UErrorCode &status) {
    UnicodeSet    allowedChars;
    UnicodeSet    *tmpSet = NULL;
    const char    *locStart = localesList;
    const char    *locEnd = NULL;
    const char    *localesListEnd = localesList + uprv_strlen(localesList);
    int32_t        localeListCount = 0;   

    
    do {
        locEnd = uprv_strchr(locStart, ',');
        if (locEnd == NULL) {
            locEnd = localesListEnd;
        }
        while (*locStart == ' ') {
            locStart++;
        }
        const char *trimmedEnd = locEnd-1;
        while (trimmedEnd > locStart && *trimmedEnd == ' ') {
            trimmedEnd--;
        }
        if (trimmedEnd <= locStart) {
            break;
        }
        const char *locale = uprv_strndup(locStart, (int32_t)(trimmedEnd + 1 - locStart));
        localeListCount++;

        
        
        
        addScriptChars(locale, &allowedChars, status);
        uprv_free((void *)locale);
        if (U_FAILURE(status)) {
            break;
        }
        locStart = locEnd + 1;
    } while (locStart < localesListEnd);

    
    if (localeListCount == 0) {
        uprv_free((void *)fAllowedLocales);
        fAllowedLocales = uprv_strdup("");
        tmpSet = new UnicodeSet(0, 0x10ffff);
        if (fAllowedLocales == NULL || tmpSet == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        } 
        tmpSet->freeze();
        delete fAllowedCharsSet;
        fAllowedCharsSet = tmpSet;
        fChecks &= ~USPOOF_CHAR_LIMIT;
        return;
    }

        
    
    UnicodeSet tempSet;
    tempSet.applyIntPropertyValue(UCHAR_SCRIPT, USCRIPT_COMMON, status);
    allowedChars.addAll(tempSet);
    tempSet.applyIntPropertyValue(UCHAR_SCRIPT, USCRIPT_INHERITED, status);
    allowedChars.addAll(tempSet);
    
    
    
    if (U_FAILURE(status)) {
        return;
    }

    
    tmpSet = static_cast<UnicodeSet *>(allowedChars.clone());
    const char *tmpLocalesList = uprv_strdup(localesList);
    if (tmpSet == NULL || tmpLocalesList == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    uprv_free((void *)fAllowedLocales);
    fAllowedLocales = tmpLocalesList;
    tmpSet->freeze();
    delete fAllowedCharsSet;
    fAllowedCharsSet = tmpSet;
    fChecks |= USPOOF_CHAR_LIMIT;
}


const char * SpoofImpl::getAllowedLocales(UErrorCode &) {
    return fAllowedLocales;
}





void SpoofImpl::addScriptChars(const char *locale, UnicodeSet *allowedChars, UErrorCode &status) {
    UScriptCode scripts[30];

    int32_t numScripts = uscript_getCode(locale, scripts, sizeof(scripts)/sizeof(UScriptCode), &status);
    if (U_FAILURE(status)) {
        return;
    }
    if (status == U_USING_DEFAULT_WARNING) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    UnicodeSet tmpSet;
    int32_t    i;
    for (i=0; i<numScripts; i++) {
        tmpSet.applyIntPropertyValue(UCHAR_SCRIPT, scripts[i], status);
        allowedChars->addAll(tmpSet);
    }
}


int32_t SpoofImpl::scriptScan
        (const UChar *text, int32_t length, int32_t &pos, UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return 0;
    }
    int32_t       inputIdx = 0;
    UChar32       c;
    int32_t       scriptCount = 0;
    UScriptCode   lastScript = USCRIPT_INVALID_CODE;
    UScriptCode   sc = USCRIPT_INVALID_CODE;
    while ((inputIdx < length || length == -1) && scriptCount < 2) {
        U16_NEXT(text, inputIdx, length, c);
        if (c == 0 && length == -1) {
            break;
        }
        sc = uscript_getScript(c, &status);
        if (sc == USCRIPT_COMMON || sc == USCRIPT_INHERITED || sc == USCRIPT_UNKNOWN) {
            continue;
        }

        
        
        
        

        if (sc == USCRIPT_HIRAGANA || sc == USCRIPT_KATAKANA || sc == USCRIPT_HANGUL) {
            sc = USCRIPT_HAN;
        }

        if (sc != lastScript) {
           scriptCount++;
           lastScript = sc;
        }
    }
    if (scriptCount == 2) {
        pos = inputIdx;
    }
    return scriptCount;
}







UChar32 SpoofImpl::ScanHex(const UChar *s, int32_t start, int32_t limit, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return 0;
    }
    U_ASSERT(limit-start > 0);
    uint32_t val = 0;
    int i;
    for (i=start; i<limit; i++) {
        int digitVal = s[i] - 0x30;
        if (digitVal>9) {
            digitVal = 0xa + (s[i] - 0x41);  
        }
        if (digitVal>15) {
            digitVal = 0xa + (s[i] - 0x61);  
        }
        U_ASSERT(digitVal <= 0xf);
        val <<= 4;
        val += digitVal;
    }
    if (val > 0x10ffff) {
        status = U_PARSE_ERROR;
        val = 0;
    }
    return (UChar32)val;
}










UBool SpoofData::validateDataVersion(const SpoofDataHeader *rawData, UErrorCode &status) {
    if (U_FAILURE(status) ||
        rawData == NULL ||
        rawData->fMagic != USPOOF_MAGIC ||
        rawData->fFormatVersion[0] > 1 ||
        rawData->fFormatVersion[1] > 0) {
            status = U_INVALID_FORMAT_ERROR;
            return FALSE;
    }
    return TRUE;
}





SpoofData *SpoofData::getDefault(UErrorCode &status) {
    

    UDataMemory *udm = udata_open(NULL, "cfu", "confusables", &status);
    if (U_FAILURE(status)) {
        return NULL;
    }
    SpoofData *This = new SpoofData(udm, status);
    if (U_FAILURE(status)) {
        delete This;
        return NULL;
    }
    if (This == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    return This;
}


SpoofData::SpoofData(UDataMemory *udm, UErrorCode &status)
{
    reset();
    if (U_FAILURE(status)) {
        return;
    }
    fRawData = reinterpret_cast<SpoofDataHeader *>
                   ((char *)(udm->pHeader) + udm->pHeader->dataHeader.headerSize);
    fUDM = udm;
    validateDataVersion(fRawData, status);
    initPtrs(status);
}


SpoofData::SpoofData(const void *data, int32_t length, UErrorCode &status)
{
    reset();
    if (U_FAILURE(status)) {
        return;
    }
    if ((size_t)length < sizeof(SpoofDataHeader)) {
        status = U_INVALID_FORMAT_ERROR;
        return;
    }
    void *ncData = const_cast<void *>(data);
    fRawData = static_cast<SpoofDataHeader *>(ncData);
    if (length < fRawData->fLength) {
        status = U_INVALID_FORMAT_ERROR;
        return;
    }
    validateDataVersion(fRawData, status);
    initPtrs(status);
}




SpoofData::SpoofData(UErrorCode &status) {
    reset();
    if (U_FAILURE(status)) {
        return;
    }
    fDataOwned = true;
    fRefCount = 1;

    
    
    uint32_t initialSize = (sizeof(SpoofDataHeader) + 15) & ~15;
    U_ASSERT(initialSize == sizeof(SpoofDataHeader));
    
    fRawData = static_cast<SpoofDataHeader *>(uprv_malloc(initialSize));
    fMemLimit = initialSize;
    if (fRawData == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    uprv_memset(fRawData, 0, initialSize);

    fRawData->fMagic = USPOOF_MAGIC;
    fRawData->fFormatVersion[0] = 1;
    fRawData->fFormatVersion[1] = 0;
    fRawData->fFormatVersion[2] = 0;
    fRawData->fFormatVersion[3] = 0;
    initPtrs(status);
}




void SpoofData::reset() {
   fRawData = NULL;
   fDataOwned = FALSE;
   fUDM      = NULL;
   fMemLimit = 0;
   fRefCount = 1;
   fCFUKeys = NULL;
   fCFUValues = NULL;
   fCFUStringLengths = NULL;
   fCFUStrings = NULL;
   fAnyCaseTrie = NULL;
   fLowerCaseTrie = NULL;
   fScriptSets = NULL;
}

















void SpoofData::initPtrs(UErrorCode &status) {
    fCFUKeys = NULL;
    fCFUValues = NULL;
    fCFUStringLengths = NULL;
    fCFUStrings = NULL;
    if (U_FAILURE(status)) {
        return;
    }
    if (fRawData->fCFUKeys != 0) {
        fCFUKeys = (int32_t *)((char *)fRawData + fRawData->fCFUKeys);
    }
    if (fRawData->fCFUStringIndex != 0) {
        fCFUValues = (uint16_t *)((char *)fRawData + fRawData->fCFUStringIndex);
    }
    if (fRawData->fCFUStringLengths != 0) {
        fCFUStringLengths = (SpoofStringLengthsElement *)((char *)fRawData + fRawData->fCFUStringLengths);
    }
    if (fRawData->fCFUStringTable != 0) {
        fCFUStrings = (UChar *)((char *)fRawData + fRawData->fCFUStringTable);
    }

    if (fAnyCaseTrie ==  NULL && fRawData->fAnyCaseTrie != 0) {
        fAnyCaseTrie = utrie2_openFromSerialized(UTRIE2_16_VALUE_BITS,
            (char *)fRawData + fRawData->fAnyCaseTrie, fRawData->fAnyCaseTrieLength, NULL, &status);
    }
    if (fLowerCaseTrie ==  NULL && fRawData->fLowerCaseTrie != 0) {
        fLowerCaseTrie = utrie2_openFromSerialized(UTRIE2_16_VALUE_BITS,
            (char *)fRawData + fRawData->fLowerCaseTrie, fRawData->fLowerCaseTrieLength, NULL, &status);
    }
    
    if (fRawData->fScriptSets != 0) {
        fScriptSets = (ScriptSet *)((char *)fRawData + fRawData->fScriptSets);
    }
}


SpoofData::~SpoofData() {
    utrie2_close(fAnyCaseTrie);
    fAnyCaseTrie = NULL;
    utrie2_close(fLowerCaseTrie);
    fLowerCaseTrie = NULL;
    if (fDataOwned) {
        uprv_free(fRawData);
    }
    fRawData = NULL;
    if (fUDM != NULL) {
        udata_close(fUDM);
    }
    fUDM = NULL;
}


void SpoofData::removeReference() {
    if (umtx_atomic_dec(&fRefCount) == 0) {
        delete this;
    }
}


SpoofData *SpoofData::addReference() {
    umtx_atomic_inc(&fRefCount);
    return this;
}


void *SpoofData::reserveSpace(int32_t numBytes,  UErrorCode &status) {
    if (U_FAILURE(status)) {
        return NULL;
    }
    if (!fDataOwned) {
        U_ASSERT(FALSE);
        status = U_INTERNAL_PROGRAM_ERROR;
        return NULL;
    }

    numBytes = (numBytes + 15) & ~15;   
    uint32_t returnOffset = fMemLimit;
    fMemLimit += numBytes;
    fRawData = static_cast<SpoofDataHeader *>(uprv_realloc(fRawData, fMemLimit));
    fRawData->fLength = fMemLimit;
    uprv_memset((char *)fRawData + returnOffset, 0, numBytes);
    initPtrs(status);
    return (char *)fRawData + returnOffset;
}







ScriptSet::ScriptSet() {
    for (uint32_t i=0; i<sizeof(bits)/sizeof(uint32_t); i++) {
        bits[i] = 0;
    }
}

ScriptSet::~ScriptSet() {
}

UBool ScriptSet::operator == (const ScriptSet &other) {
    for (uint32_t i=0; i<sizeof(bits)/sizeof(uint32_t); i++) {
        if (bits[i] != other.bits[i]) {
            return FALSE;
        }
    }
    return TRUE;
}

void ScriptSet::Union(UScriptCode script) {
    uint32_t index = script / 32;
    uint32_t bit   = 1 << (script & 31);
    U_ASSERT(index < sizeof(bits)*4);
    bits[index] |= bit;
}


void ScriptSet::Union(const ScriptSet &other) {
    for (uint32_t i=0; i<sizeof(bits)/sizeof(uint32_t); i++) {
        bits[i] |= other.bits[i];
    }
}

void ScriptSet::intersect(const ScriptSet &other) {
    for (uint32_t i=0; i<sizeof(bits)/sizeof(uint32_t); i++) {
        bits[i] &= other.bits[i];
    }
}

void ScriptSet::intersect(UScriptCode script) {
    uint32_t index = script / 32;
    uint32_t bit   = 1 << (script & 31);
    U_ASSERT(index < sizeof(bits)*4);
    uint32_t i;
    for (i=0; i<index; i++) {
        bits[i] = 0;
    }
    bits[index] &= bit;
    for (i=index+1; i<sizeof(bits)/sizeof(uint32_t); i++) {
        bits[i] = 0;
    }
}


ScriptSet & ScriptSet::operator =(const ScriptSet &other) {
    for (uint32_t i=0; i<sizeof(bits)/sizeof(uint32_t); i++) {
        bits[i] = other.bits[i];
    }
    return *this;
}


void ScriptSet::setAll() {
    for (uint32_t i=0; i<sizeof(bits)/sizeof(uint32_t); i++) {
        bits[i] = 0xffffffffu;
    }
}


void ScriptSet::resetAll() {
    for (uint32_t i=0; i<sizeof(bits)/sizeof(uint32_t); i++) {
        bits[i] = 0;
    }
}

int32_t ScriptSet::countMembers() {
    
    
    int32_t count = 0;
    for (uint32_t i=0; i<sizeof(bits)/sizeof(uint32_t); i++) {
        uint32_t x = bits[i];
        while (x > 0) {
            count++;
            x &= (x - 1);    
        }
    }
    return count;
}









NFDBuffer::NFDBuffer(const UChar *text, int32_t length, UErrorCode &status) {
    fNormalizedText = NULL;
    fNormalizedTextLength = 0;
    fOriginalText = text;
    if (U_FAILURE(status)) {
        return;
    }
    fNormalizedText = fSmallBuf;
    fNormalizedTextLength = unorm_normalize(
        text, length, UNORM_NFD, 0, fNormalizedText, USPOOF_STACK_BUFFER_SIZE, &status);
    if (status == U_BUFFER_OVERFLOW_ERROR) {
        status = U_ZERO_ERROR;
        fNormalizedText = (UChar *)uprv_malloc((fNormalizedTextLength+1)*sizeof(UChar));
        if (fNormalizedText == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
        } else {
            fNormalizedTextLength = unorm_normalize(text, length, UNORM_NFD, 0,
                                        fNormalizedText, fNormalizedTextLength+1, &status);
        }
    }
}


NFDBuffer::~NFDBuffer() {
    if (fNormalizedText != fSmallBuf) {
        uprv_free(fNormalizedText);
    }
    fNormalizedText = 0;
}

const UChar *NFDBuffer::getBuffer() {
    return fNormalizedText;
}

int32_t NFDBuffer::getLength() {
    return fNormalizedTextLength;
}





U_NAMESPACE_END

U_NAMESPACE_USE






U_CAPI int32_t U_EXPORT2
uspoof_swap(const UDataSwapper *ds, const void *inData, int32_t length, void *outData,
           UErrorCode *status) {

    if (status == NULL || U_FAILURE(*status)) {
        return 0;
    }
    if(ds==NULL || inData==NULL || length<-1 || (length>0 && outData==NULL)) {
        *status=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    
    
    
    
    const UDataInfo *pInfo = (const UDataInfo *)((const char *)inData+4);
    if(!(  pInfo->dataFormat[0]==0x43 &&   
           pInfo->dataFormat[1]==0x66 &&
           pInfo->dataFormat[2]==0x75 &&
           pInfo->dataFormat[3]==0x20 &&
           pInfo->formatVersion[0]==1  )) {
        udata_printError(ds, "uspoof_swap(): data format %02x.%02x.%02x.%02x "
                             "(format version %02x %02x %02x %02x) is not recognized\n",
                         pInfo->dataFormat[0], pInfo->dataFormat[1],
                         pInfo->dataFormat[2], pInfo->dataFormat[3],
                         pInfo->formatVersion[0], pInfo->formatVersion[1],
                         pInfo->formatVersion[2], pInfo->formatVersion[3]);
        *status=U_UNSUPPORTED_ERROR;
        return 0;
    }

    
    
    
    
    
    
    int32_t headerSize=udata_swapDataHeader(ds, inData, length, outData, status);


    
    
    
    
    const uint8_t   *inBytes =(const uint8_t *)inData+headerSize;
    SpoofDataHeader *spoofDH = (SpoofDataHeader *)inBytes;
    if (ds->readUInt32(spoofDH->fMagic)   != USPOOF_MAGIC ||
        ds->readUInt32(spoofDH->fLength)  <  sizeof(SpoofDataHeader)) 
    {
        udata_printError(ds, "uspoof_swap(): Spoof Data header is invalid.\n");
        *status=U_UNSUPPORTED_ERROR;
        return 0;
    }

    
    
    
    int32_t spoofDataLength = ds->readUInt32(spoofDH->fLength);
    int32_t totalSize = headerSize + spoofDataLength;
    if (length < 0) {
        return totalSize;
    }

    
    
    
    if (length < totalSize) {
        udata_printError(ds, "uspoof_swap(): too few bytes (%d after ICU Data header) for spoof data.\n",
                            spoofDataLength);
        *status=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0;
        }


    
    
    
    
    
    uint8_t          *outBytes = (uint8_t *)outData + headerSize;
    SpoofDataHeader  *outputDH = (SpoofDataHeader *)outBytes;

    int32_t   sectionStart;
    int32_t   sectionLength;

    
    
    
    
    
    if (inBytes != outBytes) {
        uprv_memset(outBytes, 0, spoofDataLength);
    }

    
    sectionStart  = ds->readUInt32(spoofDH->fCFUKeys);
    sectionLength = ds->readUInt32(spoofDH->fCFUKeysSize) * 4;
    ds->swapArray32(ds, inBytes+sectionStart, sectionLength, outBytes+sectionStart, status);

    
    sectionStart  = ds->readUInt32(spoofDH->fCFUStringIndex);
    sectionLength = ds->readUInt32(spoofDH->fCFUStringIndexSize) * 2;
    ds->swapArray16(ds, inBytes+sectionStart, sectionLength, outBytes+sectionStart, status);

    
    sectionStart  = ds->readUInt32(spoofDH->fCFUStringTable);
    sectionLength = ds->readUInt32(spoofDH->fCFUStringTableLen) * 2;
    ds->swapArray16(ds, inBytes+sectionStart, sectionLength, outBytes+sectionStart, status);

    
    sectionStart  = ds->readUInt32(spoofDH->fCFUStringLengths);
    sectionLength = ds->readUInt32(spoofDH->fCFUStringLengthsSize) * 4;
    ds->swapArray16(ds, inBytes+sectionStart, sectionLength, outBytes+sectionStart, status);

    
    sectionStart  = ds->readUInt32(spoofDH->fAnyCaseTrie);
    sectionLength = ds->readUInt32(spoofDH->fAnyCaseTrieLength);
    utrie2_swap(ds, inBytes+sectionStart, sectionLength, outBytes+sectionStart, status);

    
    sectionStart  = ds->readUInt32(spoofDH->fLowerCaseTrie);
    sectionLength = ds->readUInt32(spoofDH->fLowerCaseTrieLength);
    utrie2_swap(ds, inBytes+sectionStart, sectionLength, outBytes+sectionStart, status);

    
    sectionStart  = ds->readUInt32(spoofDH->fScriptSets);
    sectionLength = ds->readUInt32(spoofDH->fScriptSetsLength) * sizeof(ScriptSet);
    ds->swapArray32(ds, inBytes+sectionStart, sectionLength, outBytes+sectionStart, status);

    
    
    
    
    
    uint32_t magic = ds->readUInt32(spoofDH->fMagic);
    ds->writeUInt32((uint32_t *)&outputDH->fMagic, magic);

    if (outputDH->fFormatVersion != spoofDH->fFormatVersion) {
        uprv_memcpy(outputDH->fFormatVersion, spoofDH->fFormatVersion, sizeof(spoofDH->fFormatVersion));
    }
    
    ds->swapArray32(ds, &spoofDH->fLength, sizeof(SpoofDataHeader)-8 , &outputDH->fLength, status);

    return totalSize;
}

#endif



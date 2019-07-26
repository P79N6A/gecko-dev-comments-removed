






#include "unicode/utypes.h"

#if !UCONFIG_NO_NORMALIZATION

#include "unicode/caniter.h"
#include "unicode/normalizer2.h"
#include "unicode/uchar.h"
#include "unicode/uniset.h"
#include "unicode/usetiter.h"
#include "unicode/ustring.h"
#include "unicode/utf16.h"
#include "cmemory.h"
#include "hash.h"
#include "normalizer2impl.h"





































U_NAMESPACE_BEGIN



UOBJECT_DEFINE_RTTI_IMPLEMENTATION(CanonicalIterator)




CanonicalIterator::CanonicalIterator(const UnicodeString &sourceStr, UErrorCode &status) :
    pieces(NULL),
    pieces_length(0),
    pieces_lengths(NULL),
    current(NULL),
    current_length(0),
    nfd(*Normalizer2Factory::getNFDInstance(status)),
    nfcImpl(*Normalizer2Factory::getNFCImpl(status))
{
    if(U_SUCCESS(status) && nfcImpl.ensureCanonIterData(status)) {
      setSource(sourceStr, status);
    }
}

CanonicalIterator::~CanonicalIterator() {
  cleanPieces();
}

void CanonicalIterator::cleanPieces() {
    int32_t i = 0;
    if(pieces != NULL) {
        for(i = 0; i < pieces_length; i++) {
            if(pieces[i] != NULL) {
                delete[] pieces[i];
            }
        }
        uprv_free(pieces);
        pieces = NULL;
        pieces_length = 0;
    }
    if(pieces_lengths != NULL) {
        uprv_free(pieces_lengths);
        pieces_lengths = NULL;
    }
    if(current != NULL) {
        uprv_free(current);
        current = NULL;
        current_length = 0;
    }
}




UnicodeString CanonicalIterator::getSource() {
  return source;
}




void CanonicalIterator::reset() {
    done = FALSE;
    for (int i = 0; i < current_length; ++i) {
        current[i] = 0;
    }
}





UnicodeString CanonicalIterator::next() {
    int32_t i = 0;

    if (done) {
      buffer.setToBogus();
      return buffer;
    }

    
    buffer.remove();

    

    for (i = 0; i < pieces_length; ++i) {
        buffer.append(pieces[i][current[i]]);
    }
    

    

    for (i = current_length - 1; ; --i) {
        if (i < 0) {
            done = TRUE;
            break;
        }
        current[i]++;
        if (current[i] < pieces_lengths[i]) break; 
        current[i] = 0;
    }
    return buffer;
}





void CanonicalIterator::setSource(const UnicodeString &newSource, UErrorCode &status) {
    int32_t list_length = 0;
    UChar32 cp = 0;
    int32_t start = 0;
    int32_t i = 0;
    UnicodeString *list = NULL;

    nfd.normalize(newSource, source, status);
    if(U_FAILURE(status)) {
      return;
    }
    done = FALSE;

    cleanPieces();

    
    if (newSource.length() == 0) {
        pieces = (UnicodeString **)uprv_malloc(sizeof(UnicodeString *));
        pieces_lengths = (int32_t*)uprv_malloc(1 * sizeof(int32_t));
        pieces_length = 1;
        current = (int32_t*)uprv_malloc(1 * sizeof(int32_t));
        current_length = 1;
        if (pieces == NULL || pieces_lengths == NULL || current == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            goto CleanPartialInitialization;
        }
        current[0] = 0;
        pieces[0] = new UnicodeString[1];
        pieces_lengths[0] = 1;
        if (pieces[0] == 0) {
            status = U_MEMORY_ALLOCATION_ERROR;
            goto CleanPartialInitialization;
        }
        return;
    }


    list = new UnicodeString[source.length()];
    if (list == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        goto CleanPartialInitialization;
    }

    
    
    i = U16_LENGTH(source.char32At(0));
    
    
    
    
    
    
    for (; i < source.length(); i += U16_LENGTH(cp)) {
        cp = source.char32At(i);
        if (nfcImpl.isCanonSegmentStarter(cp)) {
            source.extract(start, i-start, list[list_length++]); 
            start = i;
        }
    }
    source.extract(start, i-start, list[list_length++]); 


    
    pieces = (UnicodeString **)uprv_malloc(list_length * sizeof(UnicodeString *));
    pieces_length = list_length;
    pieces_lengths = (int32_t*)uprv_malloc(list_length * sizeof(int32_t));
    current = (int32_t*)uprv_malloc(list_length * sizeof(int32_t));
    current_length = list_length;
    if (pieces == NULL || pieces_lengths == NULL || current == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        goto CleanPartialInitialization;
    }

    for (i = 0; i < current_length; i++) {
        current[i] = 0;
    }
    
    
    for (i = 0; i < pieces_length; ++i) {
        
        pieces[i] = getEquivalents(list[i], pieces_lengths[i], status);
    }

    delete[] list;
    return;

CleanPartialInitialization:
    if (list != NULL) {
        delete[] list;
    }
    cleanPieces();
}







void U_EXPORT2 CanonicalIterator::permute(UnicodeString &source, UBool skipZeros, Hashtable *result, UErrorCode &status) {
    if(U_FAILURE(status)) {
        return;
    }
    
    int32_t i = 0;

    
    
    
    if (source.length() <= 2 && source.countChar32() <= 1) {
        UnicodeString *toPut = new UnicodeString(source);
        
        if (toPut == 0) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        result->put(source, toPut, status);
        return;
    }

    
    UChar32 cp;
    Hashtable subpermute(status);
    if(U_FAILURE(status)) {
        return;
    }
    subpermute.setValueDeleter(uprv_deleteUObject);

    for (i = 0; i < source.length(); i += U16_LENGTH(cp)) {
        cp = source.char32At(i);
        const UHashElement *ne = NULL;
        int32_t el = -1;
        UnicodeString subPermuteString = source;

        
        
        
        if (skipZeros && i != 0 && u_getCombiningClass(cp) == 0) {
            
            continue;
        }

        subpermute.removeAll();

        
        
        permute(subPermuteString.replace(i, U16_LENGTH(cp), NULL, 0), skipZeros, &subpermute, status);
        
        if(U_FAILURE(status)) {
            return;
        }
        
        

        
        ne = subpermute.nextElement(el);
        while (ne != NULL) {
            UnicodeString *permRes = (UnicodeString *)(ne->value.pointer);
            UnicodeString *chStr = new UnicodeString(cp);
            
            if (chStr == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
            chStr->append(*permRes); 
            
            result->put(*chStr, chStr, status);
            ne = subpermute.nextElement(el);
        }
    }
    
}




UnicodeString* CanonicalIterator::getEquivalents(const UnicodeString &segment, int32_t &result_len, UErrorCode &status) {
    Hashtable result(status);
    Hashtable permutations(status);
    Hashtable basic(status);
    if (U_FAILURE(status)) {
        return 0;
    }
    result.setValueDeleter(uprv_deleteUObject);
    permutations.setValueDeleter(uprv_deleteUObject);
    basic.setValueDeleter(uprv_deleteUObject);

    UChar USeg[256];
    int32_t segLen = segment.extract(USeg, 256, status);
    getEquivalents2(&basic, USeg, segLen, status);

    
    
    

    const UHashElement *ne = NULL;
    int32_t el = -1;
    
    ne = basic.nextElement(el);
    
    while (ne != NULL) {
        
        UnicodeString item = *((UnicodeString *)(ne->value.pointer));

        permutations.removeAll();
        permute(item, CANITER_SKIP_ZEROES, &permutations, status);
        const UHashElement *ne2 = NULL;
        int32_t el2 = -1;
        
        ne2 = permutations.nextElement(el2);
        
        while (ne2 != NULL) {
            
            
            UnicodeString possible(*((UnicodeString *)(ne2->value.pointer)));
            UnicodeString attempt;
            nfd.normalize(possible, attempt, status);

            
            if (attempt==segment) {
                
                
                result.put(possible, new UnicodeString(possible), status); 
            } else {
                
            }

            ne2 = permutations.nextElement(el2);
        }
        ne = basic.nextElement(el);
    }

    
    if(U_FAILURE(status)) {
        return 0;
    }
    
    
    UnicodeString *finalResult = NULL;
    int32_t resultCount;
    if((resultCount = result.count())) {
        finalResult = new UnicodeString[resultCount];
        if (finalResult == 0) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }
    }
    else {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
    
    result_len = 0;
    el = -1;
    ne = result.nextElement(el);
    while(ne != NULL) {
        finalResult[result_len++] = *((UnicodeString *)(ne->value.pointer));
        ne = result.nextElement(el);
    }


    return finalResult;
}

Hashtable *CanonicalIterator::getEquivalents2(Hashtable *fillinResult, const UChar *segment, int32_t segLen, UErrorCode &status) {

    if (U_FAILURE(status)) {
        return NULL;
    }

    

    UnicodeString toPut(segment, segLen);

    fillinResult->put(toPut, new UnicodeString(toPut), status);

    UnicodeSet starts;

    
    UChar32 cp;
    for (int32_t i = 0; i < segLen; i += U16_LENGTH(cp)) {
        
        U16_GET(segment, 0, i, segLen, cp);
        if (!nfcImpl.getCanonStartSet(cp, starts)) {
            continue;
        }
        
        UnicodeSetIterator iter(starts);
        while (iter.next()) {
            UChar32 cp2 = iter.getCodepoint();
            Hashtable remainder(status);
            remainder.setValueDeleter(uprv_deleteUObject);
            if (extract(&remainder, cp2, segment, segLen, i, status) == NULL) {
                continue;
            }

            
            UnicodeString prefix(segment, i);
            prefix += cp2;

            int32_t el = -1;
            const UHashElement *ne = remainder.nextElement(el);
            while (ne != NULL) {
                UnicodeString item = *((UnicodeString *)(ne->value.pointer));
                UnicodeString *toAdd = new UnicodeString(prefix);
                
                if (toAdd == 0) {
                    status = U_MEMORY_ALLOCATION_ERROR;
                    return NULL;
                }
                *toAdd += item;
                fillinResult->put(*toAdd, toAdd, status);

                

                ne = remainder.nextElement(el);
            }
        }
    }

    
    if(U_FAILURE(status)) {
        return NULL;
    }
    return fillinResult;
}






Hashtable *CanonicalIterator::extract(Hashtable *fillinResult, UChar32 comp, const UChar *segment, int32_t segLen, int32_t segmentPos, UErrorCode &status) {

    
    

    if (U_FAILURE(status)) {
        return NULL;
    }

    UnicodeString temp(comp);
    int32_t inputLen=temp.length();
    UnicodeString decompString;
    nfd.normalize(temp, decompString, status);
    const UChar *decomp=decompString.getBuffer();
    int32_t decompLen=decompString.length();

    
    UBool ok = FALSE;
    UChar32 cp;
    int32_t decompPos = 0;
    UChar32 decompCp;
    U16_NEXT(decomp, decompPos, decompLen, decompCp);

    int32_t i = segmentPos;
    while(i < segLen) {
        U16_NEXT(segment, i, segLen, cp);

        if (cp == decompCp) { 

            

            if (decompPos == decompLen) { 
                temp.append(segment+i, segLen-i);
                ok = TRUE;
                break;
            }
            U16_NEXT(decomp, decompPos, decompLen, decompCp);
        } else {
            

            
            temp.append(cp);

            










        }
    }
    if (!ok)
        return NULL; 

    

    if (inputLen == temp.length()) {
        fillinResult->put(UnicodeString(), new UnicodeString(), status);
        return fillinResult; 
    }

    
    
    UnicodeString trial;
    nfd.normalize(temp, trial, status);
    if(U_FAILURE(status) || trial.compare(segment+segmentPos, segLen - segmentPos) != 0) {
        return NULL;
    }

    return getEquivalents2(fillinResult, temp.getBuffer()+inputLen, temp.length()-inputLen, status);
}

U_NAMESPACE_END

#endif 

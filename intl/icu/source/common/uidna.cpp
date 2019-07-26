















#include "unicode/utypes.h"

#if !UCONFIG_NO_IDNA

#include "unicode/uidna.h"
#include "unicode/ustring.h"
#include "unicode/usprep.h"
#include "punycode.h"
#include "ustr_imp.h"
#include "cmemory.h"
#include "uassert.h"
#include "sprpimpl.h"


static const UChar ACE_PREFIX[] ={ 0x0078,0x006E,0x002d,0x002d } ;
#define ACE_PREFIX_LENGTH 4

#define MAX_LABEL_LENGTH 63

#define MAX_LABEL_BUFFER_SIZE 100

#define MAX_DOMAIN_NAME_LENGTH 255

#define MAX_IDN_BUFFER_SIZE   MAX_DOMAIN_NAME_LENGTH+1

#define LOWER_CASE_DELTA 0x0020
#define HYPHEN           0x002D
#define FULL_STOP        0x002E
#define CAPITAL_A        0x0041
#define CAPITAL_Z        0x005A

inline static UChar 
toASCIILower(UChar ch){
    if(CAPITAL_A <= ch && ch <= CAPITAL_Z){
        return ch + LOWER_CASE_DELTA;
    }
    return ch;
}

inline static UBool 
startsWithPrefix(const UChar* src , int32_t srcLength){
    UBool startsWithPrefix = TRUE;

    if(srcLength < ACE_PREFIX_LENGTH){
        return FALSE;
    }

    for(int8_t i=0; i< ACE_PREFIX_LENGTH; i++){
        if(toASCIILower(src[i]) != ACE_PREFIX[i]){
            startsWithPrefix = FALSE;
        }
    }
    return startsWithPrefix;
}


inline static int32_t
compareCaseInsensitiveASCII(const UChar* s1, int32_t s1Len, 
                            const UChar* s2, int32_t s2Len){
    
    int32_t minLength;
    int32_t lengthResult;

    
    if(s1Len != s2Len) {
        if(s1Len < s2Len) {
            minLength = s1Len;
            lengthResult = -1;
        } else {
            minLength = s2Len;
            lengthResult = 1;
        }
    } else {
        
        minLength = s1Len;
        lengthResult = 0;
    }

    UChar c1,c2;
    int32_t rc;

    for(int32_t i =0;;i++) {

        
        if(i == minLength) {
            return lengthResult;
        }
        
        c1 = s1[i];
        c2 = s2[i];
        
        
        if(c1!=c2) {
            rc=(int32_t)toASCIILower(c1)-(int32_t)toASCIILower(c2);
            if(rc!=0) {
                lengthResult=rc;
                break;
            }
        }
    }
    return lengthResult;
}










static inline UBool isLabelSeparator(UChar ch){
    switch(ch){
        case 0x002e:
        case 0x3002:
        case 0xFF0E:
        case 0xFF61:
            return TRUE;
        default:
            return FALSE;           
    }
}




static inline int32_t
getNextSeparator(UChar *src, int32_t srcLength,
                 UChar **limit, UBool *done){
    if(srcLength == -1){
        int32_t i;
        for(i=0 ; ;i++){
            if(src[i] == 0){
                *limit = src + i; 
                *done = TRUE;
                return i;
            }
            if(isLabelSeparator(src[i])){
                *limit = src + (i+1); 
                return i;
                
            }
        }
    }else{
        int32_t i;
        for(i=0;i<srcLength;i++){
            if(isLabelSeparator(src[i])){
                *limit = src + (i+1); 
                return i;
            }
        }
        
        
        *limit = src+srcLength;
        *done = TRUE;

        return i;
    }
}
static inline UBool isLDHChar(UChar ch){
    
    if(ch>0x007A){
        return FALSE;
    }
    
    if( (ch==0x002D) || 
        (0x0030 <= ch && ch <= 0x0039) ||
        (0x0041 <= ch && ch <= 0x005A) ||
        (0x0061 <= ch && ch <= 0x007A)
      ){
        return TRUE;
    }
    return FALSE;
}

static int32_t 
_internal_toASCII(const UChar* src, int32_t srcLength, 
                  UChar* dest, int32_t destCapacity,
                  int32_t options,
                  UStringPrepProfile* nameprep,
                  UParseError* parseError,
                  UErrorCode* status)
{

    
    UChar b1Stack[MAX_LABEL_BUFFER_SIZE], b2Stack[MAX_LABEL_BUFFER_SIZE];
    
    UChar  *b1 = b1Stack, *b2 = b2Stack;
    int32_t b1Len=0, b2Len, 
            b1Capacity = MAX_LABEL_BUFFER_SIZE, 
            b2Capacity = MAX_LABEL_BUFFER_SIZE ,
            reqLength=0;

    int32_t namePrepOptions = ((options & UIDNA_ALLOW_UNASSIGNED) != 0) ? USPREP_ALLOW_UNASSIGNED: 0;
    UBool* caseFlags = NULL;
    
    
    UBool srcIsASCII  = TRUE;
    
    UBool srcIsLDH = TRUE; 

    int32_t j=0;

    
    UBool useSTD3ASCIIRules = (UBool)((options & UIDNA_USE_STD3_RULES) != 0);

    int32_t failPos = -1;
    
    if(srcLength == -1){
        srcLength = u_strlen(src);
    }
    
    if(srcLength > b1Capacity){
        b1 = (UChar*) uprv_malloc(srcLength * U_SIZEOF_UCHAR);
        if(b1==NULL){
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto CLEANUP;
        }
        b1Capacity = srcLength;
    }

    
    for( j=0;j<srcLength;j++){
        if(src[j] > 0x7F){
            srcIsASCII = FALSE;
        }
        b1[b1Len++] = src[j];
    }
    
    
    if(srcIsASCII == FALSE){
        
        
        b1Len = usprep_prepare(nameprep, src, srcLength, b1, b1Capacity, namePrepOptions, parseError, status);

        if(*status == U_BUFFER_OVERFLOW_ERROR){
            
            
            if(b1 != b1Stack){
                uprv_free(b1);
            }
            b1 = (UChar*) uprv_malloc(b1Len * U_SIZEOF_UCHAR);
            if(b1==NULL){
                *status = U_MEMORY_ALLOCATION_ERROR;
                goto CLEANUP;
            }

            *status = U_ZERO_ERROR; 
            
            b1Len = usprep_prepare(nameprep, src, srcLength, b1, b1Len, namePrepOptions, parseError, status);
        }
    }
    
    if(U_FAILURE(*status)){
        goto CLEANUP;
    }
    if(b1Len == 0){
        *status = U_IDNA_ZERO_LENGTH_LABEL_ERROR;
        goto CLEANUP;
    }

    
    srcIsASCII = TRUE;
    for( j=0;j<b1Len;j++){
        
        if(b1[j] > 0x7F){
            srcIsASCII = FALSE;
        }else if(isLDHChar(b1[j])==FALSE){  
            srcIsLDH = FALSE;
            failPos = j;
        }
    }
    if(useSTD3ASCIIRules == TRUE){
        
        
        
        
        
        
        if( srcIsLDH == FALSE 
            || b1[0] ==  HYPHEN || b1[b1Len-1] == HYPHEN){
            *status = U_IDNA_STD3_ASCII_RULES_ERROR;

            
            if(srcIsLDH==FALSE){
                
                uprv_syntaxError(b1,failPos, b1Len,parseError);
            }else if(b1[0] == HYPHEN){
                
                uprv_syntaxError(b1,0,b1Len,parseError);
            }else{
                
                uprv_syntaxError(b1, (b1Len>0) ? b1Len-1 : b1Len, b1Len,parseError);
            }

            goto CLEANUP;
        }
    }
    
    if(srcIsASCII){
        if(b1Len <= destCapacity){
            uprv_memmove(dest, b1, b1Len * U_SIZEOF_UCHAR);
            reqLength = b1Len;
        }else{
            reqLength = b1Len;
            goto CLEANUP;
        }
    }else{
        
        if(!startsWithPrefix(b1,b1Len)){

            

            
            
            
            

            b2Len = u_strToPunycode(b1,b1Len,b2,b2Capacity,caseFlags, status);

            if(*status == U_BUFFER_OVERFLOW_ERROR){
                
                
                b2 = (UChar*) uprv_malloc(b2Len * U_SIZEOF_UCHAR); 
                if(b2 == NULL){
                    *status = U_MEMORY_ALLOCATION_ERROR;
                    goto CLEANUP;
                }

                *status = U_ZERO_ERROR; 
                
                b2Len = u_strToPunycode(b1,b1Len,b2,b2Len,caseFlags, status);
            }
            
            if(U_FAILURE(*status)){
                goto CLEANUP;
            }
            
            
            
            reqLength = b2Len+ACE_PREFIX_LENGTH;

            if(reqLength > destCapacity){
                *status = U_BUFFER_OVERFLOW_ERROR;
                goto CLEANUP;
            }
            
            uprv_memcpy(dest,ACE_PREFIX,ACE_PREFIX_LENGTH * U_SIZEOF_UCHAR);
            
            uprv_memcpy(dest+ACE_PREFIX_LENGTH, b2, b2Len * U_SIZEOF_UCHAR);

        }else{
            *status = U_IDNA_ACE_PREFIX_ERROR; 
            
            uprv_syntaxError(b1,0,b1Len,parseError);
            goto CLEANUP;
        }
    }
    
    if(reqLength > MAX_LABEL_LENGTH){
        *status = U_IDNA_LABEL_TOO_LONG_ERROR;
    }

CLEANUP:
    if(b1 != b1Stack){
        uprv_free(b1);
    }
    if(b2 != b2Stack){
        uprv_free(b2);
    }
    uprv_free(caseFlags);
    
    return u_terminateUChars(dest, destCapacity, reqLength, status);
}

static int32_t
_internal_toUnicode(const UChar* src, int32_t srcLength,
                    UChar* dest, int32_t destCapacity,
                    int32_t options,
                    UStringPrepProfile* nameprep,
                    UParseError* parseError,
                    UErrorCode* status)
{

    
    
    int32_t namePrepOptions = ((options & UIDNA_ALLOW_UNASSIGNED) != 0) ? USPREP_ALLOW_UNASSIGNED: 0; 

    
    UChar b1Stack[MAX_LABEL_BUFFER_SIZE], b2Stack[MAX_LABEL_BUFFER_SIZE], b3Stack[MAX_LABEL_BUFFER_SIZE];

    
    UChar  *b1 = b1Stack, *b2 = b2Stack, *b1Prime=NULL, *b3=b3Stack;
    int32_t b1Len, b2Len, b1PrimeLen, b3Len,
            b1Capacity = MAX_LABEL_BUFFER_SIZE, 
            b2Capacity = MAX_LABEL_BUFFER_SIZE,
            b3Capacity = MAX_LABEL_BUFFER_SIZE,
            reqLength=0;

    b1Len = 0;
    UBool* caseFlags = NULL;

    UBool srcIsASCII = TRUE;
    


    
    if(srcLength==-1){
        srcLength = 0;
        for(;src[srcLength]!=0;){
            if(src[srcLength]> 0x7f){
                srcIsASCII = FALSE;
            }






            srcLength++;
        }
    }else if(srcLength > 0){
        for(int32_t j=0; j<srcLength; j++){
            if(src[j]> 0x7f){
                srcIsASCII = FALSE;
            }






        }
    }else{
        return 0;
    }
    
    if(srcIsASCII == FALSE){
        
        b1Len = usprep_prepare(nameprep, src, srcLength, b1, b1Capacity, namePrepOptions, parseError, status);
        if(*status == U_BUFFER_OVERFLOW_ERROR){
            
            
            b1 = (UChar*) uprv_malloc(b1Len * U_SIZEOF_UCHAR);
            if(b1==NULL){
                *status = U_MEMORY_ALLOCATION_ERROR;
                goto CLEANUP;
            }

            *status = U_ZERO_ERROR; 
            
            b1Len = usprep_prepare(nameprep, src, srcLength, b1, b1Len, namePrepOptions, parseError, status);
        }
        
        if(U_FAILURE(*status)){
            goto CLEANUP;
        }
    }else{

        
        b1 = (UChar*) src;
        b1Len = srcLength;
    }

    
    
    
    
    

    
    if(startsWithPrefix(b1,b1Len)){

        
        b1Prime = b1 + ACE_PREFIX_LENGTH;
        b1PrimeLen  = b1Len - ACE_PREFIX_LENGTH;

        
        b2Len = u_strFromPunycode(b1Prime, b1PrimeLen, b2, b2Capacity, caseFlags,status);

        if(*status == U_BUFFER_OVERFLOW_ERROR){
            
            
            b2 = (UChar*) uprv_malloc(b2Len * U_SIZEOF_UCHAR);
            if(b2==NULL){
                *status = U_MEMORY_ALLOCATION_ERROR;
                goto CLEANUP;
            }

            *status = U_ZERO_ERROR; 

            b2Len =  u_strFromPunycode(b1Prime, b1PrimeLen, b2, b2Len, caseFlags, status);
        }


        
        b3Len = uidna_toASCII(b2, b2Len, b3, b3Capacity, options, parseError, status);

        if(*status == U_BUFFER_OVERFLOW_ERROR){
            
            
            b3 = (UChar*) uprv_malloc(b3Len * U_SIZEOF_UCHAR);
            if(b3==NULL){
                *status = U_MEMORY_ALLOCATION_ERROR;
                goto CLEANUP;
            }

            *status = U_ZERO_ERROR; 

            b3Len =  uidna_toASCII(b2,b2Len,b3,b3Len,options,parseError, status);

        }
        
        if(U_FAILURE(*status)){
            goto CLEANUP;
        }

        
        if(compareCaseInsensitiveASCII(b1, b1Len, b3, b3Len) !=0){
            
            *status = U_IDNA_VERIFICATION_ERROR;
            goto CLEANUP;
        }

        
        reqLength = b2Len;
        if(b2Len <= destCapacity) {
            uprv_memmove(dest, b2, b2Len * U_SIZEOF_UCHAR);
        }
    }
    else{
        
        
        



















        
        
        if(srcLength <= destCapacity){
            uprv_memmove(dest,src,srcLength * U_SIZEOF_UCHAR);
        }
        reqLength = srcLength;
    }


CLEANUP:

    if(b1 != b1Stack && b1!=src){
        uprv_free(b1);
    }
    if(b2 != b2Stack){
        uprv_free(b2);
    }
    uprv_free(caseFlags);

    
    
    
    
    
    
    if(U_FAILURE(*status)){
        
        if(dest && srcLength <= destCapacity){
            
            U_ASSERT(srcLength >= 0);
            uprv_memmove(dest,src,srcLength * U_SIZEOF_UCHAR);
        }
        reqLength = srcLength;
        *status = U_ZERO_ERROR;
    }

    return u_terminateUChars(dest, destCapacity, reqLength, status);
}

U_CAPI int32_t U_EXPORT2
uidna_toASCII(const UChar* src, int32_t srcLength, 
              UChar* dest, int32_t destCapacity,
              int32_t options,
              UParseError* parseError,
              UErrorCode* status){
    
    if(status == NULL || U_FAILURE(*status)){
        return 0;
    }
    if((src==NULL) || (srcLength < -1) || (destCapacity<0) || (!dest && destCapacity > 0)){
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    UStringPrepProfile* nameprep = usprep_openByType(USPREP_RFC3491_NAMEPREP, status);
    
    if(U_FAILURE(*status)){
        return -1;
    }
    
    int32_t retLen = _internal_toASCII(src, srcLength, dest, destCapacity, options, nameprep, parseError, status);
    
    
    usprep_close(nameprep);
    
    return retLen;
}

U_CAPI int32_t U_EXPORT2
uidna_toUnicode(const UChar* src, int32_t srcLength,
                UChar* dest, int32_t destCapacity,
                int32_t options,
                UParseError* parseError,
                UErrorCode* status){

    if(status == NULL || U_FAILURE(*status)){
        return 0;
    }
    if( (src==NULL) || (srcLength < -1) || (destCapacity<0) || (!dest && destCapacity > 0)){
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }  

    UStringPrepProfile* nameprep = usprep_openByType(USPREP_RFC3491_NAMEPREP, status);
    
    if(U_FAILURE(*status)){
        return -1;
    }
    
    int32_t retLen = _internal_toUnicode(src, srcLength, dest, destCapacity, options, nameprep, parseError, status);

    usprep_close(nameprep);
    
    return retLen;
}


U_CAPI int32_t U_EXPORT2
uidna_IDNToASCII(  const UChar *src, int32_t srcLength,
                   UChar* dest, int32_t destCapacity,
                   int32_t options,
                   UParseError *parseError,
                   UErrorCode *status){

    if(status == NULL || U_FAILURE(*status)){
        return 0;
    }
    if((src==NULL) || (srcLength < -1) || (destCapacity<0) || (!dest && destCapacity > 0)){
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    int32_t reqLength = 0;

    UStringPrepProfile* nameprep = usprep_openByType(USPREP_RFC3491_NAMEPREP, status);
    
    if(U_FAILURE(*status)){
        return 0;
    }

    
    UChar *delimiter = (UChar*)src;
    UChar *labelStart = (UChar*)src;
    UChar *currentDest = (UChar*) dest;
    int32_t remainingLen = srcLength;
    int32_t remainingDestCapacity = destCapacity;
    int32_t labelLen = 0, labelReqLength = 0;
    UBool done = FALSE;


    for(;;){

        labelLen = getNextSeparator(labelStart,remainingLen, &delimiter,&done);
        labelReqLength = 0;
        if(!(labelLen==0 && done)){
        
            labelReqLength = _internal_toASCII( labelStart, labelLen, 
                                                currentDest, remainingDestCapacity, 
                                                options, nameprep, 
                                                parseError, status);
    
            if(*status == U_BUFFER_OVERFLOW_ERROR){
                
                *status = U_ZERO_ERROR; 
                remainingDestCapacity = 0;
            }
        }

    
        if(U_FAILURE(*status)){
            break;
        }
        
        reqLength +=labelReqLength;
        
        if(labelReqLength < remainingDestCapacity){
            currentDest = currentDest + labelReqLength;
            remainingDestCapacity -= labelReqLength;
        }else{
            
            remainingDestCapacity = 0;
        }

        if(done == TRUE){
            break;
        }

        
        if(remainingDestCapacity > 0){
            *currentDest++ = FULL_STOP;
            remainingDestCapacity--;
        }
        reqLength++;

        labelStart = delimiter;
        if(remainingLen >0 ){
            remainingLen = (int32_t)(srcLength - (delimiter - src));
        }

    }

    if(reqLength > MAX_DOMAIN_NAME_LENGTH){
        *status = U_IDNA_DOMAIN_NAME_TOO_LONG_ERROR;
    }

    usprep_close(nameprep);
    
    return u_terminateUChars(dest, destCapacity, reqLength, status);
}

U_CAPI int32_t U_EXPORT2
uidna_IDNToUnicode(  const UChar* src, int32_t srcLength,
                     UChar* dest, int32_t destCapacity,
                     int32_t options,
                     UParseError* parseError,
                     UErrorCode* status){
    
    if(status == NULL || U_FAILURE(*status)){
        return 0;
    }
    if((src==NULL) || (srcLength < -1) || (destCapacity<0) || (!dest && destCapacity > 0)){
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    int32_t reqLength = 0;

    UStringPrepProfile* nameprep = usprep_openByType(USPREP_RFC3491_NAMEPREP, status);
    
    if(U_FAILURE(*status)){
        return 0;
    }

    
    UChar *delimiter = (UChar*)src;
    UChar *labelStart = (UChar*)src;
    UChar *currentDest = (UChar*) dest;
    int32_t remainingLen = srcLength;
    int32_t remainingDestCapacity = destCapacity;
    int32_t labelLen = 0, labelReqLength = 0;
    UBool done = FALSE;

    for(;;){

        labelLen = getNextSeparator(labelStart,remainingLen, &delimiter,&done);
        
        
        
        
        
        
        
        



        
        labelReqLength = _internal_toUnicode(labelStart, labelLen, 
                                             currentDest, remainingDestCapacity, 
                                             options, nameprep, 
                                             parseError, status);

        if(*status == U_BUFFER_OVERFLOW_ERROR){
            *status = U_ZERO_ERROR; 
            remainingDestCapacity = 0;
        }

        if(U_FAILURE(*status)){
            break;
        }
        
        reqLength +=labelReqLength;
        
        if(labelReqLength < remainingDestCapacity){
            currentDest = currentDest + labelReqLength;
            remainingDestCapacity -= labelReqLength;
        }else{
            
            remainingDestCapacity = 0;
        }

        if(done == TRUE){
            break;
        }

        
        
        if(remainingDestCapacity > 0){
            *currentDest++ = *(labelStart + labelLen);
            remainingDestCapacity--;
        }
        reqLength++;

        labelStart = delimiter;
        if(remainingLen >0 ){
            remainingLen = (int32_t)(srcLength - (delimiter - src));
        }

    }

    if(reqLength > MAX_DOMAIN_NAME_LENGTH){
        *status = U_IDNA_DOMAIN_NAME_TOO_LONG_ERROR;
    }

    usprep_close(nameprep);
    
    return u_terminateUChars(dest, destCapacity, reqLength, status);
}

U_CAPI int32_t U_EXPORT2
uidna_compare(  const UChar *s1, int32_t length1,
                const UChar *s2, int32_t length2,
                int32_t options,
                UErrorCode* status){

    if(status == NULL || U_FAILURE(*status)){
        return -1;
    }

    UChar b1Stack[MAX_IDN_BUFFER_SIZE], b2Stack[MAX_IDN_BUFFER_SIZE];
    UChar *b1 = b1Stack, *b2 = b2Stack;
    int32_t b1Len, b2Len, b1Capacity = MAX_IDN_BUFFER_SIZE, b2Capacity = MAX_IDN_BUFFER_SIZE;
    int32_t result=-1;
    
    UParseError parseError;

    b1Len = uidna_IDNToASCII(s1, length1, b1, b1Capacity, options, &parseError, status);
    if(*status == U_BUFFER_OVERFLOW_ERROR){
        
        b1 = (UChar*) uprv_malloc(b1Len * U_SIZEOF_UCHAR);
        if(b1==NULL){
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto CLEANUP;
        }

        *status = U_ZERO_ERROR; 
        
        b1Len = uidna_IDNToASCII(s1,length1,b1,b1Len, options, &parseError, status);
        
    }

    b2Len = uidna_IDNToASCII(s2,length2, b2,b2Capacity, options, &parseError, status);
    if(*status == U_BUFFER_OVERFLOW_ERROR){
        
        b2 = (UChar*) uprv_malloc(b2Len * U_SIZEOF_UCHAR);
        if(b2==NULL){
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto CLEANUP;
        }

        *status = U_ZERO_ERROR; 
        
        b2Len = uidna_IDNToASCII(s2, length2, b2, b2Len, options, &parseError, status);
        
    }
    
    result = compareCaseInsensitiveASCII(b1,b1Len,b2,b2Len);

CLEANUP:
    if(b1 != b1Stack){
        uprv_free(b1);
    }

    if(b2 != b2Stack){
        uprv_free(b2);
    }

    return result;
}

#endif 

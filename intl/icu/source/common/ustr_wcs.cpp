


















#include "unicode/utypes.h"
#include "unicode/ustring.h"
#include "cstring.h"
#include "cwchar.h"
#include "cmemory.h"
#include "ustr_imp.h"
#include "ustr_cnv.h"

#if defined(U_WCHAR_IS_UTF16) || defined(U_WCHAR_IS_UTF32) || !UCONFIG_NO_CONVERSION

#define _STACK_BUFFER_CAPACITY 1000
#define _BUFFER_CAPACITY_MULTIPLIER 2

#if !defined(U_WCHAR_IS_UTF16) && !defined(U_WCHAR_IS_UTF32)


static inline UBool 
u_growAnyBufferFromStatic(void *context,
                       void **pBuffer, int32_t *pCapacity, int32_t reqCapacity,
                       int32_t length, int32_t size) {
    
    
    char *newBuffer=(char *)uprv_malloc(reqCapacity*size);
    if(newBuffer!=NULL) {
        if(length>0) {
            uprv_memcpy(newBuffer, *pBuffer, length*size);
        }
        *pCapacity=reqCapacity;
    } else {
        *pCapacity=0;
    }

    
    if(*pBuffer!=(char *)context) {
        uprv_free(*pBuffer);
    }

    *pBuffer=newBuffer;
    return (UBool)(newBuffer!=NULL);
}


static wchar_t* 
_strToWCS(wchar_t *dest, 
           int32_t destCapacity,
           int32_t *pDestLength,
           const UChar *src, 
           int32_t srcLength,
           UErrorCode *pErrorCode){

    char stackBuffer [_STACK_BUFFER_CAPACITY];
    char* tempBuf = stackBuffer;
    int32_t tempBufCapacity = _STACK_BUFFER_CAPACITY;
    char* tempBufLimit = stackBuffer + tempBufCapacity;
    UConverter* conv = NULL;
    char* saveBuf = tempBuf;
    wchar_t* intTarget=NULL;
    int32_t intTargetCapacity=0;
    int count=0,retVal=0;
    
    const UChar *pSrcLimit =NULL;
    const UChar *pSrc = src;

    conv = u_getDefaultConverter(pErrorCode);
    
    if(U_FAILURE(*pErrorCode)){
        return NULL;
    }
    
    if(srcLength == -1){
        srcLength = u_strlen(pSrc);
    }
    
    pSrcLimit = pSrc + srcLength;

    for(;;) {
        
        *pErrorCode = U_ZERO_ERROR;

        
        ucnv_fromUnicode(conv,&tempBuf,tempBufLimit,&pSrc,pSrcLimit,NULL,(UBool)(pSrc==pSrcLimit),pErrorCode);
        count =(tempBuf - saveBuf);
        
        
        if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR){
            tempBuf = saveBuf;
            
            
            int32_t newCapacity = 2 * srcLength;
            if(newCapacity <= tempBufCapacity) {
                newCapacity = _BUFFER_CAPACITY_MULTIPLIER * tempBufCapacity;
            }
            if(!u_growAnyBufferFromStatic(stackBuffer,(void**) &tempBuf, &tempBufCapacity,
                    newCapacity, count, 1)) {
                goto cleanup;
            }
          
           saveBuf = tempBuf;
           tempBufLimit = tempBuf + tempBufCapacity;
           tempBuf = tempBuf + count;

        } else {
            break;
        }
    }

    if(U_FAILURE(*pErrorCode)){
        goto cleanup;
    }

    
    if(count>=tempBufCapacity){
        tempBuf = saveBuf;
        
        if(!u_growAnyBufferFromStatic(stackBuffer,(void**) &tempBuf, &tempBufCapacity, 
                count+1, count, 1)) {
            goto cleanup;
        }              
       saveBuf = tempBuf;
    }
    
    saveBuf[count]=0;
      

    



    intTargetCapacity =  (count * _BUFFER_CAPACITY_MULTIPLIER + 1) ;
    intTarget = (wchar_t*)uprv_malloc( intTargetCapacity * sizeof(wchar_t) );

    if(intTarget){

        int32_t nulLen = 0;
        int32_t remaining = intTargetCapacity;
        wchar_t* pIntTarget=intTarget;
        tempBuf = saveBuf;
        
        
        for(;;){
            
            


            retVal = uprv_mbstowcs(pIntTarget,(tempBuf+nulLen),remaining);
            
            if(retVal==-1){
                *pErrorCode = U_INVALID_CHAR_FOUND;
                break;
            }else if(retVal== remaining){
                int numWritten = (pIntTarget-intTarget);
                u_growAnyBufferFromStatic(NULL,(void**) &intTarget,
                                          &intTargetCapacity,
                                          intTargetCapacity * _BUFFER_CAPACITY_MULTIPLIER,
                                          numWritten,
                                          sizeof(wchar_t));
                pIntTarget = intTarget;
                remaining=intTargetCapacity;

                if(nulLen!=count){ 
                    pIntTarget+=numWritten;
                    remaining-=numWritten;
                }

            }else{
                int32_t nulVal;
                
                
                while(tempBuf[nulLen++] != 0){
                }
                nulVal = (nulLen < srcLength) ? 1 : 0; 
                pIntTarget = pIntTarget + retVal+nulVal;
                remaining -=(retVal+nulVal);
            
                
                if(nulLen>=(count)){
                    break;
                }
            }
        }
        count = (int32_t)(pIntTarget-intTarget);
       
        if(0 < count && count <= destCapacity){
            uprv_memcpy(dest,intTarget,count*sizeof(wchar_t));
        }  

        if(pDestLength){
            *pDestLength = count;
        }

        
        uprv_free(intTarget);

    }else{
        *pErrorCode = U_MEMORY_ALLOCATION_ERROR;
    }
cleanup:
    
    if(stackBuffer != saveBuf){
        uprv_free(saveBuf);
    }
    u_terminateWChars(dest,destCapacity,count,pErrorCode);

    u_releaseDefaultConverter(conv);

    return dest;
}
#endif

U_CAPI wchar_t* U_EXPORT2
u_strToWCS(wchar_t *dest, 
           int32_t destCapacity,
           int32_t *pDestLength,
           const UChar *src, 
           int32_t srcLength,
           UErrorCode *pErrorCode){

    
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)){
        return NULL;
    }
        
    if( (src==NULL && srcLength!=0) || srcLength < -1 ||
        (destCapacity<0) || (dest == NULL && destCapacity > 0)
    ) {
        *pErrorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
    
#ifdef U_WCHAR_IS_UTF16
    
    if(srcLength == -1){
        srcLength = u_strlen(src);
    }
    if(0 < srcLength && srcLength <= destCapacity){
        uprv_memcpy(dest,src,srcLength*U_SIZEOF_UCHAR);
    }
    if(pDestLength){
       *pDestLength = srcLength;
    }

    u_terminateUChars(dest,destCapacity,srcLength,pErrorCode);

    return dest;

#elif defined U_WCHAR_IS_UTF32
    
    return (wchar_t*)u_strToUTF32((UChar32*)dest, destCapacity, pDestLength,
                                  src, srcLength, pErrorCode);

#else
    
    return _strToWCS(dest,destCapacity,pDestLength,src,srcLength, pErrorCode);
    
#endif

}

#if !defined(U_WCHAR_IS_UTF16) && !defined(U_WCHAR_IS_UTF32)

static UChar* 
_strFromWCS( UChar   *dest,
             int32_t destCapacity, 
             int32_t *pDestLength,
             const wchar_t *src,
             int32_t srcLength,
             UErrorCode *pErrorCode)
{
    int32_t retVal =0, count =0 ;
    UConverter* conv = NULL;
    UChar* pTarget = NULL;
    UChar* pTargetLimit = NULL;
    UChar* target = NULL;
    
    UChar uStack [_STACK_BUFFER_CAPACITY];

    wchar_t wStack[_STACK_BUFFER_CAPACITY];
    wchar_t* pWStack = wStack;


    char cStack[_STACK_BUFFER_CAPACITY];
    int32_t cStackCap = _STACK_BUFFER_CAPACITY;
    char* pCSrc=cStack;
    char* pCSave=pCSrc;
    char* pCSrcLimit=NULL;

    const wchar_t* pSrc = src;
    const wchar_t* pSrcLimit = NULL;

    if(srcLength ==-1){
        



        for(;;){
            
            retVal = uprv_wcstombs(pCSrc,src, cStackCap);
    
            if(retVal == -1){
                *pErrorCode = U_ILLEGAL_CHAR_FOUND;
                goto cleanup;
            }else if(retVal >= (cStackCap-1)){
                
                u_growAnyBufferFromStatic(cStack,(void**)&pCSrc,&cStackCap,
                    cStackCap * _BUFFER_CAPACITY_MULTIPLIER, 0, sizeof(char));
                pCSave = pCSrc;
            }else{
                
                pCSrc = pCSrc+retVal;
                break;
            }
        }
        
    }else{
        



        int32_t remaining =cStackCap;
        
        pSrcLimit = src + srcLength;

        for(;;){
            register int32_t nulLen = 0;

            
            while(nulLen<srcLength && pSrc[nulLen++]!=0){
            }

            if((pSrc+nulLen) < pSrcLimit){
                
                if(remaining < (nulLen * MB_CUR_MAX)){
                    
                    int32_t len = (pCSrc-pCSave);
                    pCSrc = pCSave;
                    
                    u_growAnyBufferFromStatic(cStack,(void**)&pCSrc,&cStackCap,
                           _BUFFER_CAPACITY_MULTIPLIER*cStackCap+(nulLen*MB_CUR_MAX),len,sizeof(char));

                    pCSave = pCSrc;
                    pCSrc = pCSave+len;
                    remaining = cStackCap-(pCSrc - pCSave);
                }

                


                retVal = uprv_wcstombs(pCSrc,pSrc,remaining);

                if(retVal==-1){
                    
                    *pErrorCode = U_ILLEGAL_CHAR_FOUND;
                    goto cleanup;
                }

                pCSrc += retVal+1 ;

                pSrc += nulLen; 
                srcLength-=nulLen; 
                remaining -= (pCSrc-pCSave);


            }else{
                



                if(nulLen >= _STACK_BUFFER_CAPACITY){
                    
                    
                    pWStack =(wchar_t*) uprv_malloc(sizeof(wchar_t) * (nulLen + 1));
                    if(pWStack==NULL){
                        *pErrorCode = U_MEMORY_ALLOCATION_ERROR;
                        goto cleanup;
                    }
                }
                if(nulLen>0){
                    
                    uprv_memcpy(pWStack,pSrc,nulLen*sizeof(wchar_t));
                }
            
                
                pWStack[nulLen] =0 ;
            
                if(remaining < (nulLen * MB_CUR_MAX)){
                    
                    int32_t len = (pCSrc-pCSave);
                    pCSrc = pCSave;
                    
                    u_growAnyBufferFromStatic(cStack,(void**)&pCSrc,&cStackCap,
                           cStackCap+(nulLen*MB_CUR_MAX),len,sizeof(char));

                    pCSave = pCSrc;
                    pCSrc = pCSave+len;
                    remaining = cStackCap-(pCSrc - pCSave);
                }
                
                retVal = uprv_wcstombs(pCSrc,pWStack,remaining);
            
                pCSrc += retVal;
                pSrc  += nulLen;
                srcLength-=nulLen; 
                break;
            }
        }
    }

    


    pCSrcLimit = pCSrc;
    pCSrc = pCSave;
    pTarget = target= dest;
    pTargetLimit = dest + destCapacity;    
    
    conv= u_getDefaultConverter(pErrorCode);
    
    if(U_FAILURE(*pErrorCode)|| conv==NULL){
        goto cleanup;
    }
    
    for(;;) {
        
        *pErrorCode = U_ZERO_ERROR;
        
        
        ucnv_toUnicode(conv,&pTarget,pTargetLimit,(const char**)&pCSrc,pCSrcLimit,NULL,(UBool)(pCSrc==pCSrcLimit),pErrorCode);
        
        
        count+= pTarget - target;
        
        if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR){
            target = uStack;
            pTarget = uStack;
            pTargetLimit = uStack + _STACK_BUFFER_CAPACITY;
        } else {
            break;
        }
        
    }
    
    if(pDestLength){
        *pDestLength =count;
    }

    u_terminateUChars(dest,destCapacity,count,pErrorCode);
    
cleanup:
 
    if(cStack != pCSave){
        uprv_free(pCSave);
    }

    if(wStack != pWStack){
        uprv_free(pWStack);
    }
    
    u_releaseDefaultConverter(conv);

    return dest;
}
#endif

U_CAPI UChar* U_EXPORT2
u_strFromWCS(UChar   *dest,
             int32_t destCapacity, 
             int32_t *pDestLength,
             const wchar_t *src,
             int32_t srcLength,
             UErrorCode *pErrorCode)
{

    
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)){
        return NULL;
    }

    if( (src==NULL && srcLength!=0) || srcLength < -1 ||
        (destCapacity<0) || (dest == NULL && destCapacity > 0)
    ) {
        *pErrorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

#ifdef U_WCHAR_IS_UTF16
    
    if(srcLength == -1){
        srcLength = u_strlen(src);
    }
    if(0 < srcLength && srcLength <= destCapacity){
        uprv_memcpy(dest,src,srcLength*U_SIZEOF_UCHAR);
    }
    if(pDestLength){
       *pDestLength = srcLength;
    }

    u_terminateUChars(dest,destCapacity,srcLength,pErrorCode);

    return dest;

#elif defined U_WCHAR_IS_UTF32
    
    return u_strFromUTF32(dest, destCapacity, pDestLength,
                          (UChar32*)src, srcLength, pErrorCode);

#else

    return _strFromWCS(dest,destCapacity,pDestLength,src,srcLength,pErrorCode);  

#endif

}

#endif 

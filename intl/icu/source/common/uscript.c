














#include "unicode/uscript.h"
#include "unicode/ures.h"
#include "unicode/uchar.h"
#include "unicode/putil.h"
#include "uprops.h"
#include "cmemory.h"
#include "cstring.h"

static const char kLocaleScript[] = "LocaleScript";


U_CAPI int32_t  U_EXPORT2
uscript_getCode(const char* nameOrAbbrOrLocale,
                UScriptCode* fillIn,
                int32_t capacity,
                UErrorCode* err){

    UScriptCode code = USCRIPT_INVALID_CODE;
    int32_t numFilled=0;
    int32_t len=0;
    
    if(err==NULL ||U_FAILURE(*err)){
        return numFilled;
    }
    if(nameOrAbbrOrLocale==NULL || fillIn == NULL || capacity<0){
        *err = U_ILLEGAL_ARGUMENT_ERROR;
        return numFilled;
    }

    if(uprv_strchr(nameOrAbbrOrLocale, '-')==NULL && uprv_strchr(nameOrAbbrOrLocale, '_')==NULL ){
        
        code = (UScriptCode) u_getPropertyValueEnum(UCHAR_SCRIPT, nameOrAbbrOrLocale);
        
    }
    if(code==(UScriptCode)UCHAR_INVALID_CODE){
       
        UErrorCode localErrorCode = U_ZERO_ERROR;
        UResourceBundle* resB = ures_open(NULL,nameOrAbbrOrLocale,&localErrorCode);
        if(U_SUCCESS(localErrorCode)&& localErrorCode != U_USING_DEFAULT_WARNING){
            UResourceBundle* resD = ures_getByKey(resB,kLocaleScript,NULL,&localErrorCode);
            if(U_SUCCESS(localErrorCode) ){
                len =0;
                while(ures_hasNext(resD)){
                    const UChar* name = ures_getNextString(resD,&len,NULL,&localErrorCode);
                    if(U_SUCCESS(localErrorCode)){
                        char cName[50] = {'\0'};
                        u_UCharsToChars(name,cName,len);
                        code = (UScriptCode) u_getPropertyValueEnum(UCHAR_SCRIPT, cName);
                        
                        if(numFilled<capacity){ 
                            *(fillIn)++=code;
                            numFilled++;
                        }else{
                            ures_close(resD);
                            ures_close(resB);
                            *err=U_BUFFER_OVERFLOW_ERROR;
                            return len;
                        }
                    }
                }
            }
            ures_close(resD); 
        }
        ures_close(resB);
        code = USCRIPT_INVALID_CODE;
    }
    if(code==(UScriptCode)UCHAR_INVALID_CODE){
       
        code = (UScriptCode) u_getPropertyValueEnum(UCHAR_SCRIPT, nameOrAbbrOrLocale);
    }
    if(code!=(UScriptCode)UCHAR_INVALID_CODE){
        
        if(numFilled<capacity){ 
            *(fillIn)++=code;
            numFilled++;
        }else{
            *err=U_BUFFER_OVERFLOW_ERROR;
            return len;
        }
    }
    return numFilled;
}

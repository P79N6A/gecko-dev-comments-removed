















#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION && !UCONFIG_NO_LEGACY_CONVERSION

#include "cmemory.h"
#include "unicode/ucnv.h"
#include "unicode/ucnv_cb.h"
#include "unicode/uset.h"
#include "unicode/utf16.h"
#include "ucnv_bld.h"
#include "ucnv_cnv.h"
#include "ucnv_imp.h"

#define UCNV_TILDE 0x7E          /* ~ */
#define UCNV_OPEN_BRACE 0x7B     /* { */
#define UCNV_CLOSE_BRACE 0x7D   /* } */
#define SB_ESCAPE    "\x7E\x7D"
#define DB_ESCAPE    "\x7E\x7B"
#define TILDE_ESCAPE "\x7E\x7E"
#define ESC_LEN       2


#define CONCAT_ESCAPE_MACRO( args, targetIndex,targetLength,strToAppend, err, len,sourceIndex){                             \
    while(len-->0){                                                                                                         \
        if(targetIndex < targetLength){                                                                                     \
            args->target[targetIndex] = (unsigned char) *strToAppend;                                                       \
            if(args->offsets!=NULL){                                                                                        \
                *(offsets++) = sourceIndex-1;                                                                               \
            }                                                                                                               \
            targetIndex++;                                                                                                  \
        }                                                                                                                   \
        else{                                                                                                               \
            args->converter->charErrorBuffer[(int)args->converter->charErrorBufferLength++] = (unsigned char) *strToAppend; \
            *err =U_BUFFER_OVERFLOW_ERROR;                                                                                  \
        }                                                                                                                   \
        strToAppend++;                                                                                                      \
    }                                                                                                                       \
}


typedef struct{
    UConverter* gbConverter;
    int32_t targetIndex;
    int32_t sourceIndex;
    UBool isEscapeAppended;
    UBool isStateDBCS;
    UBool isTargetUCharDBCS;
    UBool isEmptySegment;
}UConverterDataHZ;



static void 
_HZOpen(UConverter *cnv, UConverterLoadArgs *pArgs, UErrorCode *errorCode){
    UConverter *gbConverter;
    if(pArgs->onlyTestIsLoadable) {
        ucnv_canCreateConverter("GBK", errorCode);  
        return;
    }
    gbConverter = ucnv_open("GBK", errorCode);
    if(U_FAILURE(*errorCode)) {
        return;
    }
    cnv->toUnicodeStatus = 0;
    cnv->fromUnicodeStatus= 0;
    cnv->mode=0;
    cnv->fromUChar32=0x0000;
    cnv->extraInfo = uprv_calloc(1, sizeof(UConverterDataHZ));
    if(cnv->extraInfo != NULL){
        ((UConverterDataHZ*)cnv->extraInfo)->gbConverter = gbConverter;
    }
    else {
        ucnv_close(gbConverter);
        *errorCode = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
}

static void 
_HZClose(UConverter *cnv){
    if(cnv->extraInfo != NULL) {
        ucnv_close (((UConverterDataHZ *) (cnv->extraInfo))->gbConverter);
        if(!cnv->isExtraLocal) {
            uprv_free(cnv->extraInfo);
        }
        cnv->extraInfo = NULL;
    }
}

static void 
_HZReset(UConverter *cnv, UConverterResetChoice choice){
    if(choice<=UCNV_RESET_TO_UNICODE) {
        cnv->toUnicodeStatus = 0;
        cnv->mode=0;
        if(cnv->extraInfo != NULL){
            ((UConverterDataHZ*)cnv->extraInfo)->isStateDBCS = FALSE;
            ((UConverterDataHZ*)cnv->extraInfo)->isEmptySegment = FALSE;
        }
    }
    if(choice!=UCNV_RESET_TO_UNICODE) {
        cnv->fromUnicodeStatus= 0;
        cnv->fromUChar32=0x0000; 
        if(cnv->extraInfo != NULL){
            ((UConverterDataHZ*)cnv->extraInfo)->isEscapeAppended = FALSE;
            ((UConverterDataHZ*)cnv->extraInfo)->targetIndex = 0;
            ((UConverterDataHZ*)cnv->extraInfo)->sourceIndex = 0;
            ((UConverterDataHZ*)cnv->extraInfo)->isTargetUCharDBCS = FALSE;
        }
    }
}


























static void 
UConverter_toUnicode_HZ_OFFSETS_LOGIC(UConverterToUnicodeArgs *args,
                                                            UErrorCode* err){
    char tempBuf[2];
    const char *mySource = ( char *) args->source;
    UChar *myTarget = args->target;
    const char *mySourceLimit = args->sourceLimit;
    UChar32 targetUniChar = 0x0000;
    int32_t mySourceChar = 0x0000;
    UConverterDataHZ* myData=(UConverterDataHZ*)(args->converter->extraInfo);
    tempBuf[0]=0; 
    tempBuf[1]=0;

    
    



    
    while(mySource< mySourceLimit){
        
        if(myTarget < args->targetLimit){
            
            mySourceChar= (unsigned char) *mySource++;

            if(args->converter->mode == UCNV_TILDE) {
                
                args->converter->mode=0;
                switch(mySourceChar) {
                case 0x0A:
                    
                    continue;
                case UCNV_TILDE:
                    if(args->offsets) {
                        args->offsets[myTarget - args->target]=(int32_t)(mySource - args->source - 2);
                    }
                    *(myTarget++)=(UChar)mySourceChar;
                    myData->isEmptySegment = FALSE;
                    continue;
                case UCNV_OPEN_BRACE:
                case UCNV_CLOSE_BRACE:
                    myData->isStateDBCS = (mySourceChar == UCNV_OPEN_BRACE);
                    if (myData->isEmptySegment) {
                        myData->isEmptySegment = FALSE; 
                        *err = U_ILLEGAL_ESCAPE_SEQUENCE;
                        args->converter->toUCallbackReason = UCNV_IRREGULAR;
                        args->converter->toUBytes[0] = UCNV_TILDE;
                        args->converter->toUBytes[1] = mySourceChar;
                        args->converter->toULength = 2;
                        args->target = myTarget;
                        args->source = mySource;
                        return;
                    }
                    myData->isEmptySegment = TRUE;
                    continue;
                default:
                     


                    





                    myData->isEmptySegment = FALSE; 
                    *err = U_ILLEGAL_ESCAPE_SEQUENCE;
                    args->converter->toUBytes[0] = UCNV_TILDE;
                    if( myData->isStateDBCS ?
                            (0x21 <= mySourceChar && mySourceChar <= 0x7e) :
                            mySourceChar <= 0x7f
                    ) {
                        
                        args->converter->toULength = 1;
                        --mySource;
                    } else {
                        
                        args->converter->toUBytes[1] = mySourceChar;
                        args->converter->toULength = 2;
                    }
                    args->target = myTarget;
                    args->source = mySource;
                    return;
                }
            } else if(myData->isStateDBCS) {
                if(args->converter->toUnicodeStatus == 0x00){
                    
                    if(mySourceChar == UCNV_TILDE) {
                        args->converter->mode = UCNV_TILDE;
                    } else {
                        
                        args->converter->toUnicodeStatus = (uint32_t) (mySourceChar | 0x100);
                        myData->isEmptySegment = FALSE; 
                    }
                    continue;
                }
                else{
                    
                    int leadIsOk, trailIsOk;
                    uint32_t leadByte = args->converter->toUnicodeStatus & 0xff;
                    targetUniChar = 0xffff;
                    









                    leadIsOk = (uint8_t)(leadByte - 0x21) <= (0x7d - 0x21);
                    trailIsOk = (uint8_t)(mySourceChar - 0x21) <= (0x7e - 0x21);
                    if (leadIsOk && trailIsOk) {
                        tempBuf[0] = (char) (leadByte+0x80) ;
                        tempBuf[1] = (char) (mySourceChar+0x80);
                        targetUniChar = ucnv_MBCSSimpleGetNextUChar(myData->gbConverter->sharedData,
                            tempBuf, 2, args->converter->useFallback);
                        mySourceChar= (leadByte << 8) | mySourceChar;
                    } else if (trailIsOk) {
                        
                        --mySource;
                        mySourceChar = (int32_t)leadByte;
                    } else {
                        
                        
                        mySourceChar= 0x10000 | (leadByte << 8) | mySourceChar;
                    }
                    args->converter->toUnicodeStatus =0x00;
                }
            }
            else{
                if(mySourceChar == UCNV_TILDE) {
                    args->converter->mode = UCNV_TILDE;
                    continue;
                } else if(mySourceChar <= 0x7f) {
                    targetUniChar = (UChar)mySourceChar;  
                    myData->isEmptySegment = FALSE; 
                } else {
                    targetUniChar = 0xffff;
                    myData->isEmptySegment = FALSE; 
                }
            }
            if(targetUniChar < 0xfffe){
                if(args->offsets) {
                    args->offsets[myTarget - args->target]=(int32_t)(mySource - args->source - 1-(myData->isStateDBCS));
                }

                *(myTarget++)=(UChar)targetUniChar;
            }
            else  {
                if(targetUniChar == 0xfffe){
                    *err = U_INVALID_CHAR_FOUND;
                }
                else{
                    *err = U_ILLEGAL_CHAR_FOUND;
                }
                if(mySourceChar > 0xff){
                    args->converter->toUBytes[0] = (uint8_t)(mySourceChar >> 8);
                    args->converter->toUBytes[1] = (uint8_t)mySourceChar;
                    args->converter->toULength=2;
                }
                else{
                    args->converter->toUBytes[0] = (uint8_t)mySourceChar;
                    args->converter->toULength=1;
                }
                break;
            }
        }
        else{
            *err =U_BUFFER_OVERFLOW_ERROR;
            break;
        }
    }

    args->target = myTarget;
    args->source = mySource;
}


static void 
UConverter_fromUnicode_HZ_OFFSETS_LOGIC (UConverterFromUnicodeArgs * args,
                                                      UErrorCode * err){
    const UChar *mySource = args->source;
    char *myTarget = args->target;
    int32_t* offsets = args->offsets;
    int32_t mySourceIndex = 0;
    int32_t myTargetIndex = 0;
    int32_t targetLength = (int32_t)(args->targetLimit - myTarget);
    int32_t mySourceLength = (int32_t)(args->sourceLimit - args->source);
    int32_t length=0;
    uint32_t targetUniChar = 0x0000;
    UChar32 mySourceChar = 0x0000;
    UConverterDataHZ *myConverterData=(UConverterDataHZ*)args->converter->extraInfo;
    UBool isTargetUCharDBCS = (UBool) myConverterData->isTargetUCharDBCS;
    UBool oldIsTargetUCharDBCS = isTargetUCharDBCS;
    int len =0;
    const char* escSeq=NULL;
    
    
    



    if(args->converter->fromUChar32!=0 && myTargetIndex < targetLength) {
        goto getTrail;
    }
    
    while (mySourceIndex < mySourceLength){
        targetUniChar = missingCharMarker;
        if (myTargetIndex < targetLength){
            
            mySourceChar = (UChar) mySource[mySourceIndex++];
            

            oldIsTargetUCharDBCS = isTargetUCharDBCS;
            if(mySourceChar ==UCNV_TILDE){
                
                len = ESC_LEN;
                escSeq = TILDE_ESCAPE;
                CONCAT_ESCAPE_MACRO(args, myTargetIndex, targetLength, escSeq,err,len,mySourceIndex);
                continue;
            } else if(mySourceChar <= 0x7f) {
                length = 1;
                targetUniChar = mySourceChar;
            } else {
                length= ucnv_MBCSFromUChar32(myConverterData->gbConverter->sharedData,
                    mySourceChar,&targetUniChar,args->converter->useFallback);
                
                if( length == 2 &&
                    (uint16_t)(targetUniChar - 0xa1a1) <= (0xfdfe - 0xa1a1) &&
                    (uint8_t)(targetUniChar - 0xa1) <= (0xfe - 0xa1)
                ) {
                    targetUniChar -= 0x8080;
                } else {
                    targetUniChar = missingCharMarker;
                }
            }
            if (targetUniChar != missingCharMarker){
               myConverterData->isTargetUCharDBCS = isTargetUCharDBCS = (UBool)(targetUniChar>0x00FF);     
                 if(oldIsTargetUCharDBCS != isTargetUCharDBCS || !myConverterData->isEscapeAppended ){
                    
                    if(!isTargetUCharDBCS){
                        len =ESC_LEN;
                        escSeq = SB_ESCAPE;
                        CONCAT_ESCAPE_MACRO(args, myTargetIndex, targetLength, escSeq,err,len,mySourceIndex);
                        myConverterData->isEscapeAppended = TRUE;
                    }
                    else{ 
                        len =ESC_LEN;
                        escSeq = DB_ESCAPE;
                        CONCAT_ESCAPE_MACRO(args, myTargetIndex, targetLength, escSeq,err,len,mySourceIndex);
                        myConverterData->isEscapeAppended = TRUE;
                        
                    }
                }
            
                if(isTargetUCharDBCS){
                    if( myTargetIndex <targetLength){
                        myTarget[myTargetIndex++] =(char) (targetUniChar >> 8);
                        if(offsets){
                            *(offsets++) = mySourceIndex-1;
                        }
                        if(myTargetIndex < targetLength){
                            myTarget[myTargetIndex++] =(char) targetUniChar;
                            if(offsets){
                                *(offsets++) = mySourceIndex-1;
                            }
                        }else{
                            args->converter->charErrorBuffer[args->converter->charErrorBufferLength++] = (char) targetUniChar;
                            *err = U_BUFFER_OVERFLOW_ERROR;
                        } 
                    }else{
                        args->converter->charErrorBuffer[args->converter->charErrorBufferLength++] =(char) (targetUniChar >> 8);
                        args->converter->charErrorBuffer[args->converter->charErrorBufferLength++] = (char) targetUniChar;
                        *err = U_BUFFER_OVERFLOW_ERROR;
                    }

                }else{
                    if( myTargetIndex <targetLength){
                        myTarget[myTargetIndex++] = (char) (targetUniChar );
                        if(offsets){
                            *(offsets++) = mySourceIndex-1;
                        }
                        
                    }else{
                        args->converter->charErrorBuffer[args->converter->charErrorBufferLength++] = (char) targetUniChar;
                        *err = U_BUFFER_OVERFLOW_ERROR;
                    }
                }

            }
            else{
                
                
                
                if(U16_IS_SURROGATE(mySourceChar)) {
                    if(U16_IS_SURROGATE_LEAD(mySourceChar)) {
                        args->converter->fromUChar32=mySourceChar;
getTrail:
                        
                        if(mySourceIndex <  mySourceLength) {
                            
                            UChar trail=(UChar) args->source[mySourceIndex];
                            if(U16_IS_TRAIL(trail)) {
                                ++mySourceIndex;
                                mySourceChar=U16_GET_SUPPLEMENTARY(args->converter->fromUChar32, trail);
                                args->converter->fromUChar32=0x00;
                                
                                *err = U_INVALID_CHAR_FOUND;
                                
                            } else {
                                
                                
                                *err=U_ILLEGAL_CHAR_FOUND;
                            }
                        } else {
                            
                            *err = U_ZERO_ERROR;
                        }
                    } else {
                        
                        
                        *err=U_ILLEGAL_CHAR_FOUND;
                    }
                } else {
                    
                    *err = U_INVALID_CHAR_FOUND;
                }

                args->converter->fromUChar32=mySourceChar;
                break;
            }
        }
        else{
            *err = U_BUFFER_OVERFLOW_ERROR;
            break;
        }
        targetUniChar=missingCharMarker;
    }

    args->target += myTargetIndex;
    args->source += mySourceIndex;
    myConverterData->isTargetUCharDBCS = isTargetUCharDBCS;
}

static void
_HZ_WriteSub(UConverterFromUnicodeArgs *args, int32_t offsetIndex, UErrorCode *err) {
    UConverter *cnv = args->converter;
    UConverterDataHZ *convData=(UConverterDataHZ *) cnv->extraInfo;
    char *p;
    char buffer[4];
    p = buffer;
    
    if( convData->isTargetUCharDBCS){
        *p++= UCNV_TILDE;
        *p++= UCNV_CLOSE_BRACE;
        convData->isTargetUCharDBCS=FALSE;
    }
    *p++= (char)cnv->subChars[0];

    ucnv_cbFromUWriteBytes(args,
                           buffer, (int32_t)(p - buffer),
                           offsetIndex, err);
}











struct cloneHZStruct
{
    UConverter cnv;
    UConverter subCnv;
    UAlignedMemory deadSpace;
    UConverterDataHZ mydata;
};


static UConverter * 
_HZ_SafeClone(const UConverter *cnv, 
              void *stackBuffer, 
              int32_t *pBufferSize, 
              UErrorCode *status)
{
    struct cloneHZStruct * localClone;
    int32_t size, bufferSizeNeeded = sizeof(struct cloneHZStruct);

    if (U_FAILURE(*status)){
        return 0;
    }

    if (*pBufferSize == 0){ 
        *pBufferSize = bufferSizeNeeded;
        return 0;
    }

    localClone = (struct cloneHZStruct *)stackBuffer;
    

    uprv_memcpy(&localClone->mydata, cnv->extraInfo, sizeof(UConverterDataHZ));
    localClone->cnv.extraInfo = &localClone->mydata;
    localClone->cnv.isExtraLocal = TRUE;

    
    size = (int32_t)(sizeof(UConverter) + sizeof(UAlignedMemory)); 
    ((UConverterDataHZ*)localClone->cnv.extraInfo)->gbConverter =
        ucnv_safeClone(((UConverterDataHZ*)cnv->extraInfo)->gbConverter, &localClone->subCnv, &size, status);

    return &localClone->cnv;
}

static void
_HZ_GetUnicodeSet(const UConverter *cnv,
                  const USetAdder *sa,
                  UConverterUnicodeSet which,
                  UErrorCode *pErrorCode) {
    
    sa->addRange(sa->set, 0, 0x7f);

    
    ucnv_MBCSGetFilteredUnicodeSetForUnicode(
        ((UConverterDataHZ*)cnv->extraInfo)->gbConverter->sharedData,
        sa, which, UCNV_SET_FILTER_HZ,
        pErrorCode);
}

static const UConverterImpl _HZImpl={

    UCNV_HZ,
    
    NULL,
    NULL,
    
    _HZOpen,
    _HZClose,
    _HZReset,
    
    UConverter_toUnicode_HZ_OFFSETS_LOGIC,
    UConverter_toUnicode_HZ_OFFSETS_LOGIC,
    UConverter_fromUnicode_HZ_OFFSETS_LOGIC,
    UConverter_fromUnicode_HZ_OFFSETS_LOGIC,
    NULL,
    
    NULL,
    NULL,
    _HZ_WriteSub,
    _HZ_SafeClone,
    _HZ_GetUnicodeSet
};

static const UConverterStaticData _HZStaticData={
    sizeof(UConverterStaticData),
        "HZ",
         0, 
         UCNV_IBM, 
         UCNV_HZ, 
         1, 
         4,
        { 0x1a, 0, 0, 0 },
        1,
        FALSE, 
        FALSE,
        0,
        0,
        { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, 

};
            
            
const UConverterSharedData _HZData={
    sizeof(UConverterSharedData),
        ~((uint32_t) 0),
        NULL, 
        NULL, 
        &_HZStaticData, 
        FALSE, 
        &_HZImpl, 
        0
};

#endif 

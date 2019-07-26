



























#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION && !UCONFIG_NO_LEGACY_CONVERSION

#include "unicode/ucnv.h"
#include "unicode/uset.h"
#include "unicode/ucnv_err.h"
#include "unicode/ucnv_cb.h"
#include "unicode/utf16.h"
#include "ucnv_imp.h"
#include "ucnv_bld.h"
#include "ucnv_cnv.h"
#include "ucnvmbcs.h"
#include "cstring.h"
#include "cmemory.h"
#include "uassert.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

#ifdef U_ENABLE_GENERIC_ISO_2022





























#endif

static const char SHIFT_IN_STR[]  = "\x0F";


#define CR      0x0D
#define LF      0x0A
#define H_TAB   0x09
#define V_TAB   0x0B
#define SPACE   0x20

enum {
    HWKANA_START=0xff61,
    HWKANA_END=0xff9f
};









enum {
    GR94_START=0xa1,
    GR94_END=0xfe,
    GR96_START=0xa0,
    GR96_END=0xff
};







#define IS_2022_CONTROL(c) (((c)<0x20) && (((uint32_t)1<<(c))&0x0800c000)!=0)


typedef enum  {
        
        INVALID_STATE=-1,
        ASCII = 0,

        SS2_STATE=0x10,
        SS3_STATE,

        
        ISO8859_1 = 1 ,
        ISO8859_7 = 2 ,
        JISX201  = 3,
        JISX208 = 4,
        JISX212 = 5,
        GB2312  =6,
        KSC5601 =7,
        HWKANA_7BIT=8,    

        
        
        GB2312_1=1,
        ISO_IR_165=2,
        CNS_11643=3,

        



        CNS_11643_0=0x20,
        CNS_11643_1,
        CNS_11643_2,
        CNS_11643_3,
        CNS_11643_4,
        CNS_11643_5,
        CNS_11643_6,
        CNS_11643_7
} StateEnum;


#define IS_JP_DBCS(cs) (JISX208<=(cs) && (cs)<=KSC5601)

#define CSM(cs) ((uint16_t)1<<(cs))










enum { MAX_JA_VERSION=4 };
static const uint16_t jpCharsetMasks[MAX_JA_VERSION+1]={
    CSM(ASCII)|CSM(JISX201)|CSM(JISX208)|CSM(HWKANA_7BIT),
    CSM(ASCII)|CSM(JISX201)|CSM(JISX208)|CSM(HWKANA_7BIT)|CSM(JISX212),
    CSM(ASCII)|CSM(JISX201)|CSM(JISX208)|CSM(HWKANA_7BIT)|CSM(JISX212)|CSM(GB2312)|CSM(KSC5601)|CSM(ISO8859_1)|CSM(ISO8859_7),
    CSM(ASCII)|CSM(JISX201)|CSM(JISX208)|CSM(HWKANA_7BIT)|CSM(JISX212)|CSM(GB2312)|CSM(KSC5601)|CSM(ISO8859_1)|CSM(ISO8859_7),
    CSM(ASCII)|CSM(JISX201)|CSM(JISX208)|CSM(HWKANA_7BIT)|CSM(JISX212)|CSM(GB2312)|CSM(KSC5601)|CSM(ISO8859_1)|CSM(ISO8859_7)
};

typedef enum {
        ASCII1=0,
        LATIN1,
        SBCS,
        DBCS,
        MBCS,
        HWKANA
}Cnv2022Type;

typedef struct ISO2022State {
    int8_t cs[4];       
    int8_t g;           
    int8_t prevG;       
} ISO2022State;

#define UCNV_OPTIONS_VERSION_MASK 0xf
#define UCNV_2022_MAX_CONVERTERS 10

typedef struct{
    UConverterSharedData *myConverterArray[UCNV_2022_MAX_CONVERTERS];
    UConverter *currentConverter;
    Cnv2022Type currentType;
    ISO2022State toU2022State, fromU2022State;
    uint32_t key;
    uint32_t version;
#ifdef U_ENABLE_GENERIC_ISO_2022
    UBool isFirstBuffer;
#endif
    UBool isEmptySegment;
    char name[30];
    char locale[3];
}UConverterDataISO2022;





U_CFUNC void
ucnv_fromUnicode_UTF8(UConverterFromUnicodeArgs * args,
                      UErrorCode * err);
U_CFUNC void
ucnv_fromUnicode_UTF8_OFFSETS_LOGIC(UConverterFromUnicodeArgs * args,
                                    UErrorCode * err);

#define ESC_2022 0x1B /*ESC*/

typedef enum
{
        INVALID_2022 = -1, 
        VALID_NON_TERMINAL_2022 = 0, 
        VALID_TERMINAL_2022 = 1, 
        VALID_MAYBE_TERMINAL_2022 = 2 
} UCNV_TableStates_2022;






































static const int8_t normalize_esq_chars_2022[256] = {


         0     ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0
        ,0     ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0
        ,0     ,0      ,0      ,0      ,0      ,0      ,0      ,1      ,0      ,0
        ,0     ,0      ,0      ,0      ,0      ,0      ,4      ,7      ,29      ,0
        ,2     ,24     ,26     ,27     ,0      ,3      ,23     ,6      ,0      ,0
        ,0     ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0
        ,0     ,0      ,0      ,0      ,5      ,8      ,9      ,10     ,11     ,12
        ,13    ,14     ,15     ,16     ,17     ,18     ,19     ,20     ,25     ,28
        ,0     ,0      ,21     ,0      ,0      ,0      ,0      ,0      ,0      ,0
        ,22    ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0
        ,0     ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0
        ,0     ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0
        ,0     ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0
        ,0     ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0
        ,0     ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0
        ,0     ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0
        ,0     ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0
        ,0     ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0
        ,0     ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0
        ,0     ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0
        ,0     ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0
        ,0     ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0
        ,0     ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0
        ,0     ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0
        ,0     ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,0
        ,0     ,0      ,0      ,0      ,0      ,0
};

#ifdef U_ENABLE_GENERIC_ISO_2022
















#endif

#define MAX_STATES_2022 74
static const int32_t escSeqStateTable_Key_2022[MAX_STATES_2022] = {


     1          ,34         ,36         ,39         ,55         ,57         ,60         ,61         ,1093       ,1096
    ,1097       ,1098       ,1099       ,1100       ,1101       ,1102       ,1103       ,1104       ,1105       ,1106
    ,1109       ,1154       ,1157       ,1160       ,1161       ,1176       ,1178       ,1179       ,1254       ,1257
    ,1768       ,1773       ,1957       ,35105      ,36933      ,36936      ,36937      ,36938      ,36939      ,36940
    ,36942      ,36943      ,36944      ,36945      ,36946      ,36947      ,36948      ,37640      ,37642      ,37644
    ,37646      ,37711      ,37744      ,37745      ,37746      ,37747      ,37748      ,40133      ,40136      ,40138
    ,40139      ,40140      ,40141      ,1123363    ,35947624   ,35947625   ,35947626   ,35947627   ,35947629   ,35947630
    ,35947631   ,35947635   ,35947636   ,35947638
};

#ifdef U_ENABLE_GENERIC_ISO_2022

static const char* const escSeqStateTable_Result_2022[MAX_STATES_2022] = {
 

     NULL                   ,NULL                   ,NULL                   ,NULL               ,NULL               ,NULL                   ,NULL                   ,NULL                   ,"latin1"               ,"latin1"
    ,"latin1"               ,"ibm-865"              ,"ibm-865"              ,"ibm-865"          ,"ibm-865"          ,"ibm-865"              ,"ibm-865"              ,"JISX0201"             ,"JISX0201"             ,"latin1"
    ,"latin1"               ,NULL                   ,"JISX-208"             ,"ibm-5478"         ,"JISX-208"         ,NULL                   ,NULL                   ,NULL                   ,NULL                   ,"UTF8"
    ,"ISO-8859-1"           ,"ISO-8859-7"           ,"JIS-X-208"            ,NULL               ,"ibm-955"          ,"ibm-367"              ,"ibm-952"              ,"ibm-949"              ,"JISX-212"             ,"ibm-1383"
    ,"ibm-952"              ,"ibm-964"              ,"ibm-964"              ,"ibm-964"          ,"ibm-964"          ,"ibm-964"              ,"ibm-964"              ,"ibm-5478"         ,"ibm-949"              ,"ISO-IR-165"
    ,"CNS-11643-1992,1"     ,"CNS-11643-1992,2"     ,"CNS-11643-1992,3"     ,"CNS-11643-1992,4" ,"CNS-11643-1992,5" ,"CNS-11643-1992,6"     ,"CNS-11643-1992,7"     ,"UTF16_PlatformEndian" ,"UTF16_PlatformEndian" ,"UTF16_PlatformEndian"
    ,"UTF16_PlatformEndian" ,"UTF16_PlatformEndian" ,"UTF16_PlatformEndian" ,NULL               ,"latin1"           ,"ibm-912"              ,"ibm-913"              ,"ibm-914"              ,"ibm-813"              ,"ibm-1089"
    ,"ibm-920"              ,"ibm-915"              ,"ibm-915"              ,"latin1"
};

#endif

static const int8_t escSeqStateTable_Value_2022[MAX_STATES_2022] = {

     VALID_NON_TERMINAL_2022    ,VALID_NON_TERMINAL_2022    ,VALID_NON_TERMINAL_2022    ,VALID_NON_TERMINAL_2022     ,VALID_NON_TERMINAL_2022   ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_NON_TERMINAL_2022    ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022
    ,VALID_MAYBE_TERMINAL_2022  ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022
    ,VALID_TERMINAL_2022        ,VALID_NON_TERMINAL_2022    ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_NON_TERMINAL_2022    ,VALID_NON_TERMINAL_2022    ,VALID_NON_TERMINAL_2022    ,VALID_NON_TERMINAL_2022    ,VALID_TERMINAL_2022
    ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_NON_TERMINAL_2022    ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022
    ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022
    ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022
    ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_NON_TERMINAL_2022    ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022
    ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022        ,VALID_TERMINAL_2022
};



typedef enum{
#ifdef U_ENABLE_GENERIC_ISO_2022
    ISO_2022=0,
#endif
    ISO_2022_JP=1,
    ISO_2022_KR=2,
    ISO_2022_CN=3
} Variant2022;


static void
_ISO2022Open(UConverter *cnv, UConverterLoadArgs *pArgs, UErrorCode *errorCode);

static void
 _ISO2022Close(UConverter *converter);

static void
_ISO2022Reset(UConverter *converter, UConverterResetChoice choice);

static const char*
_ISO2022getName(const UConverter* cnv);

static void
_ISO_2022_WriteSub(UConverterFromUnicodeArgs *args, int32_t offsetIndex, UErrorCode *err);

static UConverter *
_ISO_2022_SafeClone(const UConverter *cnv, void *stackBuffer, int32_t *pBufferSize, UErrorCode *status);

#ifdef U_ENABLE_GENERIC_ISO_2022
static void
T_UConverter_toUnicode_ISO_2022_OFFSETS_LOGIC(UConverterToUnicodeArgs* args, UErrorCode* err);
#endif

namespace {


extern const UConverterSharedData _ISO2022JPData;
extern const UConverterSharedData _ISO2022KRData;
extern const UConverterSharedData _ISO2022CNData;

}  




static inline void
fromUWriteUInt8(UConverter *cnv,
                 const char *bytes, int32_t length,
                 uint8_t **target, const char *targetLimit,
                 int32_t **offsets,
                 int32_t sourceIndex,
                 UErrorCode *pErrorCode)
{
    char *targetChars = (char *)*target;
    ucnv_fromUWriteBytes(cnv, bytes, length, &targetChars, targetLimit,
                         offsets, sourceIndex, pErrorCode);
    *target = (uint8_t*)targetChars;

}

static inline void
setInitialStateToUnicodeKR(UConverter* , UConverterDataISO2022 *myConverterData){
    if(myConverterData->version == 1) {
        UConverter *cnv = myConverterData->currentConverter;

        cnv->toUnicodeStatus=0;     
        cnv->mode=0;                
        cnv->toULength=0;           
    }
}

static inline void
setInitialStateFromUnicodeKR(UConverter* converter,UConverterDataISO2022 *myConverterData){
   


    if( converter->charErrorBufferLength==0){

        converter->charErrorBufferLength = 4;
        converter->charErrorBuffer[0] = 0x1b;
        converter->charErrorBuffer[1] = 0x24;
        converter->charErrorBuffer[2] = 0x29;
        converter->charErrorBuffer[3] = 0x43;
    }
    if(myConverterData->version == 1) {
        UConverter *cnv = myConverterData->currentConverter;

        cnv->fromUChar32=0;
        cnv->fromUnicodeStatus=1;   
    }
}

static void
_ISO2022Open(UConverter *cnv, UConverterLoadArgs *pArgs, UErrorCode *errorCode){

    char myLocale[6]={' ',' ',' ',' ',' ',' '};

    cnv->extraInfo = uprv_malloc (sizeof (UConverterDataISO2022));
    if(cnv->extraInfo != NULL) {
        UConverterNamePieces stackPieces;
        UConverterLoadArgs stackArgs=UCNV_LOAD_ARGS_INITIALIZER;
        UConverterDataISO2022 *myConverterData=(UConverterDataISO2022 *) cnv->extraInfo;
        uint32_t version;

        stackArgs.onlyTestIsLoadable = pArgs->onlyTestIsLoadable;

        uprv_memset(myConverterData, 0, sizeof(UConverterDataISO2022));
        myConverterData->currentType = ASCII1;
        cnv->fromUnicodeStatus =FALSE;
        if(pArgs->locale){
            uprv_strncpy(myLocale, pArgs->locale, sizeof(myLocale));
        }
        version = pArgs->options & UCNV_OPTIONS_VERSION_MASK;
        myConverterData->version = version;
        if(myLocale[0]=='j' && (myLocale[1]=='a'|| myLocale[1]=='p') &&
            (myLocale[2]=='_' || myLocale[2]=='\0'))
        {
            size_t len=0;
            
            if(version>MAX_JA_VERSION) {
                
                myConverterData->version = version = 0;
            }
            if(jpCharsetMasks[version]&CSM(ISO8859_7)) {
                myConverterData->myConverterArray[ISO8859_7] =
                    ucnv_loadSharedData("ISO8859_7", &stackPieces, &stackArgs, errorCode);
            }
            myConverterData->myConverterArray[JISX208] =
                ucnv_loadSharedData("Shift-JIS", &stackPieces, &stackArgs, errorCode);
            if(jpCharsetMasks[version]&CSM(JISX212)) {
                myConverterData->myConverterArray[JISX212] =
                    ucnv_loadSharedData("jisx-212", &stackPieces, &stackArgs, errorCode);
            }
            if(jpCharsetMasks[version]&CSM(GB2312)) {
                myConverterData->myConverterArray[GB2312] =
                    ucnv_loadSharedData("ibm-5478", &stackPieces, &stackArgs, errorCode);   
            }
            if(jpCharsetMasks[version]&CSM(KSC5601)) {
                myConverterData->myConverterArray[KSC5601] =
                    ucnv_loadSharedData("ksc_5601", &stackPieces, &stackArgs, errorCode);
            }

            
            cnv->sharedData=(UConverterSharedData*)(&_ISO2022JPData);
            uprv_strcpy(myConverterData->locale,"ja");

            (void)uprv_strcpy(myConverterData->name,"ISO_2022,locale=ja,version=");
            len = uprv_strlen(myConverterData->name);
            myConverterData->name[len]=(char)(myConverterData->version+(int)'0');
            myConverterData->name[len+1]='\0';
        }
        else if(myLocale[0]=='k' && (myLocale[1]=='o'|| myLocale[1]=='r') &&
            (myLocale[2]=='_' || myLocale[2]=='\0'))
        {
            const char *cnvName;
            if(version==1) {
                cnvName="icu-internal-25546";
            } else {
                cnvName="ibm-949";
                myConverterData->version=version=0;
            }
            if(pArgs->onlyTestIsLoadable) {
                ucnv_canCreateConverter(cnvName, errorCode);  
                uprv_free(cnv->extraInfo);
                cnv->extraInfo=NULL;
                return;
            } else {
                myConverterData->currentConverter=ucnv_open(cnvName, errorCode);
                if (U_FAILURE(*errorCode)) {
                    _ISO2022Close(cnv);
                    return;
                }

                if(version==1) {
                    (void)uprv_strcpy(myConverterData->name,"ISO_2022,locale=ko,version=1");
                    uprv_memcpy(cnv->subChars, myConverterData->currentConverter->subChars, 4);
                    cnv->subCharLen = myConverterData->currentConverter->subCharLen;
                }else{
                    (void)uprv_strcpy(myConverterData->name,"ISO_2022,locale=ko,version=0");
                }

                
                setInitialStateToUnicodeKR(cnv, myConverterData);
                setInitialStateFromUnicodeKR(cnv, myConverterData);

                
                cnv->sharedData=(UConverterSharedData*)&_ISO2022KRData;
                uprv_strcpy(myConverterData->locale,"ko");
            }
        }
        else if(((myLocale[0]=='z' && myLocale[1]=='h') || (myLocale[0]=='c'&& myLocale[1]=='n'))&&
            (myLocale[2]=='_' || myLocale[2]=='\0'))
        {

            
            myConverterData->myConverterArray[GB2312_1] =
                ucnv_loadSharedData("ibm-5478", &stackPieces, &stackArgs, errorCode);
            if(version==1) {
                myConverterData->myConverterArray[ISO_IR_165] =
                    ucnv_loadSharedData("iso-ir-165", &stackPieces, &stackArgs, errorCode);
            }
            myConverterData->myConverterArray[CNS_11643] =
                ucnv_loadSharedData("cns-11643-1992", &stackPieces, &stackArgs, errorCode);


            
            cnv->sharedData=(UConverterSharedData*)&_ISO2022CNData;
            uprv_strcpy(myConverterData->locale,"cn");

            if (version==0){
                myConverterData->version = 0;
                (void)uprv_strcpy(myConverterData->name,"ISO_2022,locale=zh,version=0");
            }else if (version==1){
                myConverterData->version = 1;
                (void)uprv_strcpy(myConverterData->name,"ISO_2022,locale=zh,version=1");
            }else {
                myConverterData->version = 2;
                (void)uprv_strcpy(myConverterData->name,"ISO_2022,locale=zh,version=2");
            }
        }
        else{
#ifdef U_ENABLE_GENERIC_ISO_2022
            myConverterData->isFirstBuffer = TRUE;

            
            cnv->charErrorBufferLength = 3;
            cnv->charErrorBuffer[0] = 0x1b;
            cnv->charErrorBuffer[1] = 0x25;
            cnv->charErrorBuffer[2] = 0x42;

            cnv->sharedData=(UConverterSharedData*)&_ISO2022Data;
            
            uprv_strcpy(myConverterData->name,"ISO_2022");
#else
            *errorCode = U_UNSUPPORTED_ERROR;
            return;
#endif
        }

        cnv->maxBytesPerUChar=cnv->sharedData->staticData->maxBytesPerChar;

        if(U_FAILURE(*errorCode) || pArgs->onlyTestIsLoadable) {
            _ISO2022Close(cnv);
        }
    } else {
        *errorCode = U_MEMORY_ALLOCATION_ERROR;
    }
}


static void
_ISO2022Close(UConverter *converter) {
    UConverterDataISO2022* myData =(UConverterDataISO2022 *) (converter->extraInfo);
    UConverterSharedData **array = myData->myConverterArray;
    int32_t i;

    if (converter->extraInfo != NULL) {
        
        for (i=0; i<UCNV_2022_MAX_CONVERTERS; i++) {
            if(array[i]!=NULL) {
                ucnv_unloadSharedDataIfReady(array[i]);
            }
        }

        ucnv_close(myData->currentConverter);

        if(!converter->isExtraLocal){
            uprv_free (converter->extraInfo);
            converter->extraInfo = NULL;
        }
    }
}

static void
_ISO2022Reset(UConverter *converter, UConverterResetChoice choice) {
    UConverterDataISO2022 *myConverterData=(UConverterDataISO2022 *) (converter->extraInfo);
    if(choice<=UCNV_RESET_TO_UNICODE) {
        uprv_memset(&myConverterData->toU2022State, 0, sizeof(ISO2022State));
        myConverterData->key = 0;
        myConverterData->isEmptySegment = FALSE;
    }
    if(choice!=UCNV_RESET_TO_UNICODE) {
        uprv_memset(&myConverterData->fromU2022State, 0, sizeof(ISO2022State));
    }
#ifdef U_ENABLE_GENERIC_ISO_2022
    if(myConverterData->locale[0] == 0){
        if(choice<=UCNV_RESET_TO_UNICODE) {
            myConverterData->isFirstBuffer = TRUE;
            myConverterData->key = 0;
            if (converter->mode == UCNV_SO){
                ucnv_close (myConverterData->currentConverter);
                myConverterData->currentConverter=NULL;
            }
            converter->mode = UCNV_SI;
        }
        if(choice!=UCNV_RESET_TO_UNICODE) {
            
            converter->charErrorBufferLength = 3;
            converter->charErrorBuffer[0] = 0x1b;
            converter->charErrorBuffer[1] = 0x28;
            converter->charErrorBuffer[2] = 0x42;
        }
    }
    else
#endif
    {
        
        if(myConverterData->locale[0] == 'k'){
            if(choice<=UCNV_RESET_TO_UNICODE) {
                setInitialStateToUnicodeKR(converter, myConverterData);
            }
            if(choice!=UCNV_RESET_TO_UNICODE) {
                setInitialStateFromUnicodeKR(converter, myConverterData);
            }
        }
    }
}

static const char*
_ISO2022getName(const UConverter* cnv){
    if(cnv->extraInfo){
        UConverterDataISO2022* myData= (UConverterDataISO2022*)cnv->extraInfo;
        return myData->name;
    }
    return NULL;
}
















static const int8_t nextStateToUnicodeJP[MAX_STATES_2022]= {

    INVALID_STATE   ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,SS2_STATE      ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE
    ,ASCII          ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,JISX201        ,HWKANA_7BIT    ,JISX201        ,INVALID_STATE
    ,INVALID_STATE  ,INVALID_STATE  ,JISX208        ,GB2312         ,JISX208        ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE
    ,ISO8859_1      ,ISO8859_7      ,JISX208        ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,KSC5601        ,JISX212        ,INVALID_STATE
    ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE
    ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE
    ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE
    ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE
};


static const int8_t nextStateToUnicodeCN[MAX_STATES_2022]= {

     INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,SS2_STATE      ,SS3_STATE      ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE
    ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE
    ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE
    ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE
    ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,GB2312_1       ,INVALID_STATE  ,ISO_IR_165
    ,CNS_11643_1    ,CNS_11643_2    ,CNS_11643_3    ,CNS_11643_4    ,CNS_11643_5    ,CNS_11643_6    ,CNS_11643_7    ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE
    ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE
    ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE  ,INVALID_STATE
};


static UCNV_TableStates_2022
getKey_2022(char c,int32_t* key,int32_t* offset){
    int32_t togo;
    int32_t low = 0;
    int32_t hi = MAX_STATES_2022;
    int32_t oldmid=0;

    togo = normalize_esq_chars_2022[(uint8_t)c];
    if(togo == 0) {
        
        *key = 0;
        *offset = 0;
        return INVALID_2022;
    }
    togo = (*key << 5) + togo;

    while (hi != low)  {

        register int32_t mid = (hi+low) >> 1; 

        if (mid == oldmid)
            break;

        if (escSeqStateTable_Key_2022[mid] > togo){
            hi = mid;
        }
        else if (escSeqStateTable_Key_2022[mid] < togo){
            low = mid;
        }
        else {
            *key = togo;
            *offset = mid;
            return (UCNV_TableStates_2022)escSeqStateTable_Value_2022[mid];
        }
        oldmid = mid;

    }

    *key = 0;
    *offset = 0;
    return INVALID_2022;
}



static void
changeState_2022(UConverter* _this,
                const char** source,
                const char* sourceLimit,
                Variant2022 var,
                UErrorCode* err){
    UCNV_TableStates_2022 value;
    UConverterDataISO2022* myData2022 = ((UConverterDataISO2022*)_this->extraInfo);
    uint32_t key = myData2022->key;
    int32_t offset = 0;
    int8_t initialToULength = _this->toULength;
    char c;

    value = VALID_NON_TERMINAL_2022;
    while (*source < sourceLimit) {
        c = *(*source)++;
        _this->toUBytes[_this->toULength++]=(uint8_t)c;
        value = getKey_2022(c,(int32_t *) &key, &offset);

        switch (value){

        case VALID_NON_TERMINAL_2022 :
            
            break;

        case VALID_TERMINAL_2022:
            key = 0;
            goto DONE;

        case INVALID_2022:
            goto DONE;

        case VALID_MAYBE_TERMINAL_2022:
#ifdef U_ENABLE_GENERIC_ISO_2022
            
            if(var == ISO_2022) {
                
                _this->toULength = 0;

                

                
                value = VALID_NON_TERMINAL_2022;
                break;
            } else
#endif
            {
                
                value = VALID_TERMINAL_2022;
                key = 0;
                goto DONE;
            }
        }
    }

DONE:
    myData2022->key = key;

    if (value == VALID_NON_TERMINAL_2022) {
        
        return;
    } else if (value == INVALID_2022 ) {
        *err = U_ILLEGAL_ESCAPE_SEQUENCE;
    } else  {
        switch(var){
#ifdef U_ENABLE_GENERIC_ISO_2022
        case ISO_2022:
        {
            const char *chosenConverterName = escSeqStateTable_Result_2022[offset];
            if(chosenConverterName == NULL) {
                
                *err = U_UNSUPPORTED_ESCAPE_SEQUENCE;
                _this->toUCallbackReason = UCNV_UNASSIGNED;
                return;
            }

            _this->mode = UCNV_SI;
            ucnv_close(myData2022->currentConverter);
            myData2022->currentConverter = myUConverter = ucnv_open(chosenConverterName, err);
            if(U_SUCCESS(*err)) {
                myUConverter->fromCharErrorBehaviour = UCNV_TO_U_CALLBACK_STOP;
                _this->mode = UCNV_SO;
            }
            break;
        }
#endif
        case ISO_2022_JP:
            {
                StateEnum tempState=(StateEnum)nextStateToUnicodeJP[offset];
                switch(tempState) {
                case INVALID_STATE:
                    *err = U_UNSUPPORTED_ESCAPE_SEQUENCE;
                    break;
                case SS2_STATE:
                    if(myData2022->toU2022State.cs[2]!=0) {
                        if(myData2022->toU2022State.g<2) {
                            myData2022->toU2022State.prevG=myData2022->toU2022State.g;
                        }
                        myData2022->toU2022State.g=2;
                    } else {
                        
                        *err = U_ILLEGAL_ESCAPE_SEQUENCE;
                    }
                    break;
                
                case ISO8859_1:
                case ISO8859_7:
                    if((jpCharsetMasks[myData2022->version] & CSM(tempState)) == 0) {
                        *err = U_UNSUPPORTED_ESCAPE_SEQUENCE;
                    } else {
                        
                        myData2022->toU2022State.cs[2]=(int8_t)tempState;
                    }
                    break;
                default:
                    if((jpCharsetMasks[myData2022->version] & CSM(tempState)) == 0) {
                        *err = U_UNSUPPORTED_ESCAPE_SEQUENCE;
                    } else {
                        
                        myData2022->toU2022State.cs[0]=(int8_t)tempState;
                    }
                    break;
                }
            }
            break;
        case ISO_2022_CN:
            {
                StateEnum tempState=(StateEnum)nextStateToUnicodeCN[offset];
                switch(tempState) {
                case INVALID_STATE:
                    *err = U_UNSUPPORTED_ESCAPE_SEQUENCE;
                    break;
                case SS2_STATE:
                    if(myData2022->toU2022State.cs[2]!=0) {
                        if(myData2022->toU2022State.g<2) {
                            myData2022->toU2022State.prevG=myData2022->toU2022State.g;
                        }
                        myData2022->toU2022State.g=2;
                    } else {
                        
                        *err = U_ILLEGAL_ESCAPE_SEQUENCE;
                    }
                    break;
                case SS3_STATE:
                    if(myData2022->toU2022State.cs[3]!=0) {
                        if(myData2022->toU2022State.g<2) {
                            myData2022->toU2022State.prevG=myData2022->toU2022State.g;
                        }
                        myData2022->toU2022State.g=3;
                    } else {
                        
                        *err = U_ILLEGAL_ESCAPE_SEQUENCE;
                    }
                    break;
                case ISO_IR_165:
                    if(myData2022->version==0) {
                        *err = U_UNSUPPORTED_ESCAPE_SEQUENCE;
                        break;
                    }
                    
                case GB2312_1:
                    
                case CNS_11643_1:
                    myData2022->toU2022State.cs[1]=(int8_t)tempState;
                    break;
                case CNS_11643_2:
                    myData2022->toU2022State.cs[2]=(int8_t)tempState;
                    break;
                default:
                    
                    if(myData2022->version==0) {
                        *err = U_UNSUPPORTED_ESCAPE_SEQUENCE;
                    } else {
                       myData2022->toU2022State.cs[3]=(int8_t)tempState;
                    }
                    break;
                }
            }
            break;
        case ISO_2022_KR:
            if(offset==0x30){
                
            } else {
                *err = U_UNSUPPORTED_ESCAPE_SEQUENCE;
            }
            break;

        default:
            *err = U_ILLEGAL_ESCAPE_SEQUENCE;
            break;
        }
    }
    if(U_SUCCESS(*err)) {
        _this->toULength = 0;
    } else if(*err==U_ILLEGAL_ESCAPE_SEQUENCE) {
        if(_this->toULength>1) {
            










            
            int8_t backOutDistance=_this->toULength-1;
            int8_t bytesFromThisBuffer=_this->toULength-initialToULength;
            if(backOutDistance<=bytesFromThisBuffer) {
                
                *source-=backOutDistance;
            } else {
                
                _this->preToULength=(int8_t)(bytesFromThisBuffer-backOutDistance);
                
                
                uprv_memcpy(_this->preToU, _this->toUBytes+1, -_this->preToULength);
                *source-=bytesFromThisBuffer;
            }
            _this->toULength=1;
        }
    } else if(*err==U_UNSUPPORTED_ESCAPE_SEQUENCE) {
        _this->toUCallbackReason = UCNV_UNASSIGNED;
    }
}









static inline const char*
getEndOfBuffer_2022(const char** source,
                   const char* sourceLimit,
                   UBool ){

    const char* mySource = *source;

#ifdef U_ENABLE_GENERIC_ISO_2022
    if (*source >= sourceLimit)
        return sourceLimit;

    do{

        if (*mySource == ESC_2022){
            int8_t i;
            int32_t key = 0;
            int32_t offset;
            UCNV_TableStates_2022 value = VALID_NON_TERMINAL_2022;

            





            for (i=0;
            (mySource+i < sourceLimit)&&(value == VALID_NON_TERMINAL_2022);
            i++) {
                value =  getKey_2022(*(mySource+i), &key, &offset);
            }
            if (value > 0 || *mySource==ESC_2022)
                return mySource;

            if ((value == VALID_NON_TERMINAL_2022)&&(!flush) )
                return sourceLimit;
        }
    }while (++mySource < sourceLimit);

    return sourceLimit;
#else
    while(mySource < sourceLimit && *mySource != ESC_2022) {
        ++mySource;
    }
    return mySource;
#endif
}






static inline int32_t
MBCS_FROM_UCHAR32_ISO2022(UConverterSharedData* sharedData,
                                         UChar32 c,
                                         uint32_t* value,
                                         UBool useFallback,
                                         int outputType)
{
    const int32_t *cx;
    const uint16_t *table;
    uint32_t stage2Entry;
    uint32_t myValue;
    int32_t length;
    const uint8_t *p;
    




    
    if(c<0x10000 || (sharedData->mbcs.unicodeMask&UCNV_HAS_SUPPLEMENTARY)) {
        table=sharedData->mbcs.fromUnicodeTable;
        stage2Entry=MBCS_STAGE_2_FROM_U(table, c);
        
        if(outputType==MBCS_OUTPUT_2){
            myValue=MBCS_VALUE_2_FROM_STAGE_2(sharedData->mbcs.fromUnicodeBytes, stage2Entry, c);
            if(myValue<=0xff) {
                length=1;
            } else {
                length=2;
            }
        } else  {
            p=MBCS_POINTER_3_FROM_STAGE_2(sharedData->mbcs.fromUnicodeBytes, stage2Entry, c);
            myValue=((uint32_t)*p<<16)|((uint32_t)p[1]<<8)|p[2];
            if(myValue<=0xff) {
                length=1;
            } else if(myValue<=0xffff) {
                length=2;
            } else {
                length=3;
            }
        }
        
        if((stage2Entry&(1<<(16+(c&0xf))))!=0) {
            
            *value=myValue;
            return length;
        } else if(FROM_U_USE_FALLBACK(useFallback, c) && myValue!=0) {
            




            *value=myValue;
            return -length;
        }
    }

    cx=sharedData->mbcs.extIndexes;
    if(cx!=NULL) {
        return ucnv_extSimpleMatchFromU(cx, c, value, useFallback);
    }

    
    return 0;
}






static inline int32_t
MBCS_SINGLE_FROM_UCHAR32(UConverterSharedData* sharedData,
                                       UChar32 c,
                                       uint32_t* retval,
                                       UBool useFallback)
{
    const uint16_t *table;
    int32_t value;
    
    if(c>=0x10000 && !(sharedData->mbcs.unicodeMask&UCNV_HAS_SUPPLEMENTARY)) {
        return 0;
    }
    
    table=sharedData->mbcs.fromUnicodeTable;
    
    value=MBCS_SINGLE_RESULT_FROM_U(table, (uint16_t *)sharedData->mbcs.fromUnicodeBytes, c);
    
    *retval=(uint32_t)(value&0xff);
    if(value>=0xf00) {
        return 1;  
    } else if(useFallback ? value>=0x800 : value>=0xc00) {
        return -1;  
    } else {
        return 0;  
    }
}







static inline uint32_t
_2022FromGR94DBCS(uint32_t value) {
    if( (uint16_t)(value - 0xa1a1) <= (0xfefe - 0xa1a1) &&
        (uint8_t)(value - 0xa1) <= (0xfe - 0xa1)
    ) {
        return value - 0x8080;  
    } else {
        return 0;  
    }
}

#if 0 





static inline uint32_t
_2022ToGR94DBCS(uint32_t value) {
    uint32_t returnValue = value + 0x8080;
    if( (uint16_t)(returnValue - 0xa1a1) <= (0xfefe - 0xa1a1) &&
        (uint8_t)(returnValue - 0xa1) <= (0xfe - 0xa1)) {
        return returnValue;
    } else {
        return value;
    }
}
#endif

#ifdef U_ENABLE_GENERIC_ISO_2022







static void
T_UConverter_toUnicode_ISO_2022_OFFSETS_LOGIC(UConverterToUnicodeArgs* args,
                                                           UErrorCode* err){
    const char* mySourceLimit, *realSourceLimit;
    const char* sourceStart;
    const UChar* myTargetStart;
    UConverter* saveThis;
    UConverterDataISO2022* myData;
    int8_t length;

    saveThis = args->converter;
    myData=((UConverterDataISO2022*)(saveThis->extraInfo));

    realSourceLimit = args->sourceLimit;
    while (args->source < realSourceLimit) {
        if(myData->key == 0) { 
            
            mySourceLimit = getEndOfBuffer_2022(&(args->source), realSourceLimit, args->flush);

            if(args->source < mySourceLimit) {
                if(myData->currentConverter==NULL) {
                    myData->currentConverter = ucnv_open("ASCII",err);
                    if(U_FAILURE(*err)){
                        return;
                    }

                    myData->currentConverter->fromCharErrorBehaviour = UCNV_TO_U_CALLBACK_STOP;
                    saveThis->mode = UCNV_SO;
                }

                
                myData->isFirstBuffer=FALSE;
                sourceStart = args->source;
                myTargetStart = args->target;
                args->converter = myData->currentConverter;
                ucnv_toUnicode(args->converter,
                    &args->target,
                    args->targetLimit,
                    &args->source,
                    mySourceLimit,
                    args->offsets,
                    (UBool)(args->flush && mySourceLimit == realSourceLimit),
                    err);
                args->converter = saveThis;

                if (*err == U_BUFFER_OVERFLOW_ERROR) {
                    
                    length = saveThis->UCharErrorBufferLength = myData->currentConverter->UCharErrorBufferLength;
                    myData->currentConverter->UCharErrorBufferLength = 0;
                    if(length > 0) {
                        uprv_memcpy(saveThis->UCharErrorBuffer,
                                    myData->currentConverter->UCharErrorBuffer,
                                    length*U_SIZEOF_UCHAR);
                    }
                    return;
                }

                








                if (U_FAILURE(*err) ||
                    (args->source == realSourceLimit) ||
                    (args->offsets != NULL && (args->target != myTargetStart || args->source != sourceStart) ||
                    (mySourceLimit < realSourceLimit && myData->currentConverter->toULength > 0))
                ) {
                    
                    if(U_FAILURE(*err)) {
                        length = saveThis->invalidCharLength = myData->currentConverter->invalidCharLength;
                        if(length > 0) {
                            uprv_memcpy(saveThis->invalidCharBuffer, myData->currentConverter->invalidCharBuffer, length);
                        }
                    } else {
                        length = saveThis->toULength = myData->currentConverter->toULength;
                        if(length > 0) {
                            uprv_memcpy(saveThis->toUBytes, myData->currentConverter->toUBytes, length);
                            if(args->source < mySourceLimit) {
                                *err = U_TRUNCATED_CHAR_FOUND; 
                            }
                        }
                    }
                    return;
                }
            }
        }

        sourceStart = args->source;
        changeState_2022(args->converter,
               &(args->source),
               realSourceLimit,
               ISO_2022,
               err);
        if (U_FAILURE(*err) || (args->source != sourceStart && args->offsets != NULL)) {
            
            return;
        }
    }
}

#endif




static void
toUnicodeCallback(UConverter *cnv,
                  const uint32_t sourceChar, const uint32_t targetUniChar,
                  UErrorCode* err){
    if(sourceChar>0xff){
        cnv->toUBytes[0] = (uint8_t)(sourceChar>>8);
        cnv->toUBytes[1] = (uint8_t)sourceChar;
        cnv->toULength = 2;
    }
    else{
        cnv->toUBytes[0] =(char) sourceChar;
        cnv->toULength = 1;
    }

    if(targetUniChar == (missingCharMarker-1)){
        *err = U_INVALID_CHAR_FOUND;
    }
    else{
        *err = U_ILLEGAL_CHAR_FOUND;
    }
}












































static const StateEnum jpCharsetPref[]={
    ASCII,
    JISX201,
    ISO8859_1,
    ISO8859_7,
    JISX208,
    JISX212,
    GB2312,
    KSC5601,
    HWKANA_7BIT
};





static const char escSeqChars[][6] ={
    "\x1B\x28\x42",         
    "\x1B\x2E\x41",         
    "\x1B\x2E\x46",         
    "\x1B\x28\x4A",         
    "\x1B\x24\x42",         
    "\x1B\x24\x28\x44",     
    "\x1B\x24\x41",         
    "\x1B\x24\x28\x43",     
    "\x1B\x28\x49"          

};
static  const int8_t escSeqCharsLen[] ={
    3, 
    3, 
    3, 
    3, 
    3, 
    4, 
    3, 
    4, 
    3  
};



















static inline uint32_t
jisx201ToU(uint32_t value) {
    if(value < 0x5c) {
        return value;
    } else if(value == 0x5c) {
        return 0xa5;
    } else if(value == 0x7e) {
        return 0x203e;
    } else  {
        return value;
    }
}


static inline uint32_t
jisx201FromU(uint32_t value) {
    if(value<=0x7f) {
        if(value!=0x5c && value!=0x7e) {
            return value;
        }
    } else if(value==0xa5) {
        return 0x5c;
    } else if(value==0x203e) {
        return 0x7e;
    }
    return 0xfffe;
}






static inline uint32_t
_2022FromSJIS(uint32_t value) {
    uint8_t trail;

    if(value > 0xEFFC) {
        return 0;  
    }

    trail = (uint8_t)value;

    value &= 0xff00;  
    if(value <= 0x9f00) {
        value -= 0x7000;
    } else  {
        value -= 0xb000;
    }
    value <<= 1;

    if(trail <= 0x9e) {
        value -= 0x100;
        if(trail <= 0x7e) {
            value |= trail - 0x1f;
        } else {
            value |= trail - 0x20;
        }
    } else  {
        value |= trail - 0x7e;
    }
    return value;
}








static inline void
_2022ToSJIS(uint8_t c1, uint8_t c2, char bytes[2]) {
    if(c1&1) {
        ++c1;
        if(c2 <= 0x5f) {
            c2 += 0x1f;
        } else if(c2 <= 0x7e) {
            c2 += 0x20;
        } else {
            c2 = 0;  
        }
    } else {
        if((uint8_t)(c2-0x21) <= ((0x7e)-0x21)) {
            c2 += 0x7e;
        } else {
            c2 = 0;  
        }
    }
    c1 >>= 1;
    if(c1 <= 0x2f) {
        c1 += 0x70;
    } else if(c1 <= 0x3f) {
        c1 += 0xb0;
    } else {
        c1 = 0;  
    }
    bytes[0] = (char)c1;
    bytes[1] = (char)c2;
}








static const uint16_t hwkana_fb[HWKANA_END - HWKANA_START + 1] = {
    0x2123,  
    0x2156,
    0x2157,
    0x2122,
    0x2126,
    0x2572,
    0x2521,
    0x2523,
    0x2525,
    0x2527,
    0x2529,
    0x2563,
    0x2565,
    0x2567,
    0x2543,
    0x213C,  
    0x2522,
    0x2524,
    0x2526,
    0x2528,
    0x252A,
    0x252B,
    0x252D,
    0x252F,
    0x2531,
    0x2533,
    0x2535,
    0x2537,
    0x2539,
    0x253B,
    0x253D,
    0x253F,  
    0x2541,
    0x2544,
    0x2546,
    0x2548,
    0x254A,
    0x254B,
    0x254C,
    0x254D,
    0x254E,
    0x254F,
    0x2552,
    0x2555,
    0x2558,
    0x255B,
    0x255E,
    0x255F,  
    0x2560,
    0x2561,
    0x2562,
    0x2564,
    0x2566,
    0x2568,
    0x2569,
    0x256A,
    0x256B,
    0x256C,
    0x256D,
    0x256F,
    0x2573,
    0x212B,
    0x212C   
};

static void
UConverter_fromUnicode_ISO_2022_JP_OFFSETS_LOGIC(UConverterFromUnicodeArgs* args, UErrorCode* err) {
    UConverter *cnv = args->converter;
    UConverterDataISO2022 *converterData;
    ISO2022State *pFromU2022State;
    uint8_t *target = (uint8_t *) args->target;
    const uint8_t *targetLimit = (const uint8_t *) args->targetLimit;
    const UChar* source = args->source;
    const UChar* sourceLimit = args->sourceLimit;
    int32_t* offsets = args->offsets;
    UChar32 sourceChar;
    char buffer[8];
    int32_t len, outLen;
    int8_t choices[10];
    int32_t choiceCount;
    uint32_t targetValue = 0;
    UBool useFallback;

    int32_t i;
    int8_t cs, g;

    
    converterData     = (UConverterDataISO2022*)cnv->extraInfo;
    pFromU2022State   = &converterData->fromU2022State;

    choiceCount = 0;

    
    if((sourceChar = cnv->fromUChar32)!=0 && target< targetLimit) {
        goto getTrail;
    }

    while(source < sourceLimit) {
        if(target < targetLimit) {

            sourceChar  = *(source++);
            
            if(U16_IS_SURROGATE(sourceChar)) {
                if(U16_IS_SURROGATE_LEAD(sourceChar)) {
getTrail:
                    
                    if(source < sourceLimit) {
                        
                        UChar trail=(UChar) *source;
                        if(U16_IS_TRAIL(trail)) {
                            source++;
                            sourceChar=U16_GET_SUPPLEMENTARY(sourceChar, trail);
                            cnv->fromUChar32=0x00;
                            
                            
                        } else {
                            
                            
                            *err=U_ILLEGAL_CHAR_FOUND;
                            cnv->fromUChar32=sourceChar;
                            break;
                        }
                    } else {
                        
                        cnv->fromUChar32=sourceChar;
                        break;
                    }
                } else {
                    
                    
                    *err=U_ILLEGAL_CHAR_FOUND;
                    cnv->fromUChar32=sourceChar;
                    break;
                }
            }

            
            if(IS_2022_CONTROL(sourceChar)) {
                
                *err=U_ILLEGAL_CHAR_FOUND;
                cnv->fromUChar32=sourceChar;
                break;
            }

            

            if(choiceCount == 0) {
                uint16_t csm;

                



                csm = jpCharsetMasks[converterData->version];
                choiceCount = 0;

                
                if(converterData->version == 3 || converterData->version == 4) {
                    choices[choiceCount++] = (int8_t)HWKANA_7BIT;
                }
                
                csm &= ~CSM(HWKANA_7BIT);

                
                choices[choiceCount++] = cs = pFromU2022State->cs[0];
                csm &= ~CSM(cs);

                
                if((cs = pFromU2022State->cs[2]) != 0) {
                    choices[choiceCount++] = cs;
                    csm &= ~CSM(cs);
                }

                
                for(i = 0; i < LENGTHOF(jpCharsetPref); ++i) {
                    cs = (int8_t)jpCharsetPref[i];
                    if(CSM(cs) & csm) {
                        choices[choiceCount++] = cs;
                        csm &= ~CSM(cs);
                    }
                }
            }

            cs = g = 0;
            




            len = 0;
            





            useFallback = cnv->useFallback;

            for(i = 0; i < choiceCount && len <= 0; ++i) {
                uint32_t value;
                int32_t len2;
                int8_t cs0 = choices[i];
                switch(cs0) {
                case ASCII:
                    if(sourceChar <= 0x7f) {
                        targetValue = (uint32_t)sourceChar;
                        len = 1;
                        cs = cs0;
                        g = 0;
                    }
                    break;
                case ISO8859_1:
                    if(GR96_START <= sourceChar && sourceChar <= GR96_END) {
                        targetValue = (uint32_t)sourceChar - 0x80;
                        len = 1;
                        cs = cs0;
                        g = 2;
                    }
                    break;
                case HWKANA_7BIT:
                    if((uint32_t)(sourceChar - HWKANA_START) <= (HWKANA_END - HWKANA_START)) {
                        if(converterData->version==3) {
                            
                            
                            targetValue = (uint32_t)(sourceChar - (HWKANA_START - 0x21));
                            len = 1;
                            pFromU2022State->cs[1] = cs = cs0; 
                            g = 1;
                        } else if(converterData->version==4) {
                            
                            
                            targetValue = (uint32_t)(sourceChar - (HWKANA_START - 0xa1));
                            len = 1;

                            cs = pFromU2022State->cs[0];
                            if(IS_JP_DBCS(cs)) {
                                
                                cs = (int8_t)JISX201;
                            }
                            
                            g = 0;
                        }
                        
                    }
                    break;
                case JISX201:
                    
                    value = jisx201FromU(sourceChar);
                    if(value <= 0x7f) {
                        targetValue = value;
                        len = 1;
                        cs = cs0;
                        g = 0;
                        useFallback = FALSE;
                    }
                    break;
                case JISX208:
                    
                    len2 = MBCS_FROM_UCHAR32_ISO2022(
                                converterData->myConverterArray[cs0],
                                sourceChar, &value,
                                useFallback, MBCS_OUTPUT_2);
                    if(len2 == 2 || (len2 == -2 && len == 0)) {  
                        value = _2022FromSJIS(value);
                        if(value != 0) {
                            targetValue = value;
                            len = len2;
                            cs = cs0;
                            g = 0;
                            useFallback = FALSE;
                        }
                    } else if(len == 0 && useFallback &&
                              (uint32_t)(sourceChar - HWKANA_START) <= (HWKANA_END - HWKANA_START)) {
                        targetValue = hwkana_fb[sourceChar - HWKANA_START];
                        len = -2;
                        cs = cs0;
                        g = 0;
                        useFallback = FALSE;
                    }
                    break;
                case ISO8859_7:
                    
                    len2 = MBCS_SINGLE_FROM_UCHAR32(
                                converterData->myConverterArray[cs0],
                                sourceChar, &value,
                                useFallback);
                    if(len2 != 0 && !(len2 < 0 && len != 0) && GR96_START <= value && value <= GR96_END) {
                        targetValue = value - 0x80;
                        len = len2;
                        cs = cs0;
                        g = 2;
                        useFallback = FALSE;
                    }
                    break;
                default:
                    
                    len2 = MBCS_FROM_UCHAR32_ISO2022(
                                converterData->myConverterArray[cs0],
                                sourceChar, &value,
                                useFallback, MBCS_OUTPUT_2);
                    if(len2 == 2 || (len2 == -2 && len == 0)) {  
                        if(cs0 == KSC5601) {
                            




                            value = _2022FromGR94DBCS(value);
                            if(value == 0) {
                                break;
                            }
                        }
                        targetValue = value;
                        len = len2;
                        cs = cs0;
                        g = 0;
                        useFallback = FALSE;
                    }
                    break;
                }
            }

            if(len != 0) {
                if(len < 0) {
                    len = -len;  
                }
                outLen = 0; 

                
                if(pFromU2022State->g == 1 && g == 0) {
                    buffer[outLen++] = UCNV_SI;
                    pFromU2022State->g = 0;
                }

                
                if(cs != pFromU2022State->cs[g]) {
                    int32_t escLen = escSeqCharsLen[cs];
                    uprv_memcpy(buffer + outLen, escSeqChars[cs], escLen);
                    outLen += escLen;
                    pFromU2022State->cs[g] = cs;

                    
                    choiceCount = 0;
                }

                
                if(g != pFromU2022State->g) {
                    switch(g) {
                    
                    case 1:
                        buffer[outLen++] = UCNV_SO;
                        pFromU2022State->g = 1;
                        break;
                    default: 
                        buffer[outLen++] = 0x1b;
                        buffer[outLen++] = 0x4e;
                        break;
                    
                    }
                }

                
                if(len == 1) {
                    buffer[outLen++] = (char)targetValue;
                } else  {
                    buffer[outLen++] = (char)(targetValue >> 8);
                    buffer[outLen++] = (char)targetValue;
                }
            } else {
                



                *err = U_INVALID_CHAR_FOUND;
                cnv->fromUChar32=sourceChar;
                break;
            }

            if(sourceChar == CR || sourceChar == LF) {
                
                pFromU2022State->cs[2] = 0;
                choiceCount = 0;
            }

            
            if(outLen == 1) {
                *target++ = buffer[0];
                if(offsets) {
                    *offsets++ = (int32_t)(source - args->source - 1); 
                }
            } else if(outLen == 2 && (target + 2) <= targetLimit) {
                *target++ = buffer[0];
                *target++ = buffer[1];
                if(offsets) {
                    int32_t sourceIndex = (int32_t)(source - args->source - U16_LENGTH(sourceChar));
                    *offsets++ = sourceIndex;
                    *offsets++ = sourceIndex;
                }
            } else {
                fromUWriteUInt8(
                    cnv,
                    buffer, outLen,
                    &target, (const char *)targetLimit,
                    &offsets, (int32_t)(source - args->source - U16_LENGTH(sourceChar)),
                    err);
                if(U_FAILURE(*err)) {
                    break;
                }
            }
        } 
        else{
            *err =U_BUFFER_OVERFLOW_ERROR;
            break;
        }

    }

    









    if( U_SUCCESS(*err) &&
        (pFromU2022State->g!=0 || pFromU2022State->cs[0]!=ASCII) &&
        args->flush && source>=sourceLimit && cnv->fromUChar32==0
    ) {
        int32_t sourceIndex;

        outLen = 0;

        if(pFromU2022State->g != 0) {
            buffer[outLen++] = UCNV_SI;
            pFromU2022State->g = 0;
        }

        if(pFromU2022State->cs[0] != ASCII) {
            int32_t escLen = escSeqCharsLen[ASCII];
            uprv_memcpy(buffer + outLen, escSeqChars[ASCII], escLen);
            outLen += escLen;
            pFromU2022State->cs[0] = (int8_t)ASCII;
        }

        
        






        sourceIndex=(int32_t)(source-args->source);
        if(sourceIndex>0) {
            --sourceIndex;
            if( U16_IS_TRAIL(args->source[sourceIndex]) &&
                (sourceIndex==0 || U16_IS_LEAD(args->source[sourceIndex-1]))
            ) {
                --sourceIndex;
            }
        } else {
            sourceIndex=-1;
        }

        fromUWriteUInt8(
            cnv,
            buffer, outLen,
            &target, (const char *)targetLimit,
            &offsets, sourceIndex,
            err);
    }

    
    args->source = source;
    args->target = (char*)target;
}



static void
UConverter_toUnicode_ISO_2022_JP_OFFSETS_LOGIC(UConverterToUnicodeArgs *args,
                                               UErrorCode* err){
    char tempBuf[2];
    const char *mySource = (char *) args->source;
    UChar *myTarget = args->target;
    const char *mySourceLimit = args->sourceLimit;
    uint32_t targetUniChar = 0x0000;
    uint32_t mySourceChar = 0x0000;
    uint32_t tmpSourceChar = 0x0000;
    UConverterDataISO2022* myData;
    ISO2022State *pToU2022State;
    StateEnum cs;

    myData=(UConverterDataISO2022*)(args->converter->extraInfo);
    pToU2022State = &myData->toU2022State;

    if(myData->key != 0) {
        
        goto escape;
    } else if(args->converter->toULength == 1 && mySource < mySourceLimit && myTarget < args->targetLimit) {
        
        mySourceChar = args->converter->toUBytes[0];
        args->converter->toULength = 0;
        cs = (StateEnum)pToU2022State->cs[pToU2022State->g];
        targetUniChar = missingCharMarker;
        goto getTrailByte;
    }

    while(mySource < mySourceLimit){

        targetUniChar =missingCharMarker;

        if(myTarget < args->targetLimit){

            mySourceChar= (unsigned char) *mySource++;

            switch(mySourceChar) {
            case UCNV_SI:
                if(myData->version==3) {
                    pToU2022State->g=0;
                    continue;
                } else {
                    
                    myData->isEmptySegment = FALSE;	
                    break;
                }

            case UCNV_SO:
                if(myData->version==3) {
                    
                    pToU2022State->cs[1] = (int8_t)HWKANA_7BIT;
                    pToU2022State->g=1;
                    continue;
                } else {
                    
                    myData->isEmptySegment = FALSE;	
                    break;
                }

            case ESC_2022:
                mySource--;
escape:
                {
                    const char * mySourceBefore = mySource;
                    int8_t toULengthBefore = args->converter->toULength;

                    changeState_2022(args->converter,&(mySource),
                        mySourceLimit, ISO_2022_JP,err);

                    
                    if(myData->version==0 && myData->key==0 && U_SUCCESS(*err) && myData->isEmptySegment) {
                        *err = U_ILLEGAL_ESCAPE_SEQUENCE;
                        args->converter->toUCallbackReason = UCNV_IRREGULAR;
                        args->converter->toULength = (int8_t)(toULengthBefore + (mySource - mySourceBefore));
                    }
                }

                
                if(U_FAILURE(*err)){
                    args->target = myTarget;
                    args->source = mySource;
                    myData->isEmptySegment = FALSE;	
                    return;
                }
                
                if(myData->key==0) {
                    myData->isEmptySegment = TRUE;
                }
                continue;

            

            case CR:
                
            case LF:
                
                if((StateEnum)pToU2022State->cs[0] != ASCII && (StateEnum)pToU2022State->cs[0] != JISX201) {
                    pToU2022State->cs[0] = (int8_t)ASCII;
                }
                pToU2022State->cs[2] = 0;
                pToU2022State->g = 0;
                
            default:
                
                myData->isEmptySegment = FALSE;
                cs = (StateEnum)pToU2022State->cs[pToU2022State->g];
                if( (uint8_t)(mySourceChar - 0xa1) <= (0xdf - 0xa1) && myData->version==4 &&
                    !IS_JP_DBCS(cs)
                ) {
                    
                    targetUniChar = mySourceChar + (HWKANA_START - 0xa1);

                    
                    if(pToU2022State->g >= 2) {
                        pToU2022State->g=pToU2022State->prevG;
                    }
                } else switch(cs) {
                case ASCII:
                    if(mySourceChar <= 0x7f) {
                        targetUniChar = mySourceChar;
                    }
                    break;
                case ISO8859_1:
                    if(mySourceChar <= 0x7f) {
                        targetUniChar = mySourceChar + 0x80;
                    }
                    
                    pToU2022State->g=pToU2022State->prevG;
                    break;
                case ISO8859_7:
                    if(mySourceChar <= 0x7f) {
                        
                        targetUniChar =
                            _MBCS_SINGLE_SIMPLE_GET_NEXT_BMP(
                                myData->myConverterArray[cs],
                                mySourceChar + 0x80);
                    }
                    
                    pToU2022State->g=pToU2022State->prevG;
                    break;
                case JISX201:
                    if(mySourceChar <= 0x7f) {
                        targetUniChar = jisx201ToU(mySourceChar);
                    }
                    break;
                case HWKANA_7BIT:
                    if((uint8_t)(mySourceChar - 0x21) <= (0x5f - 0x21)) {
                        
                        targetUniChar = mySourceChar + (HWKANA_START - 0x21);
                    }
                    break;
                default:
                    
                    if(mySource < mySourceLimit) {
                        int leadIsOk, trailIsOk;
                        uint8_t trailByte;
getTrailByte:
                        trailByte = (uint8_t)*mySource;
                        









                        leadIsOk = (uint8_t)(mySourceChar - 0x21) <= (0x7e - 0x21);
                        trailIsOk = (uint8_t)(trailByte - 0x21) <= (0x7e - 0x21);
                        if (leadIsOk && trailIsOk) {
                            ++mySource;
                            tmpSourceChar = (mySourceChar << 8) | trailByte;
                            if(cs == JISX208) {
                                _2022ToSJIS((uint8_t)mySourceChar, trailByte, tempBuf);
                                mySourceChar = tmpSourceChar;
                            } else {
                                
                                mySourceChar = tmpSourceChar;
                                if (cs == KSC5601) {
                                    tmpSourceChar += 0x8080;  
                                }
                                tempBuf[0] = (char)(tmpSourceChar >> 8);
                                tempBuf[1] = (char)(tmpSourceChar);
                            }
                            targetUniChar = ucnv_MBCSSimpleGetNextUChar(myData->myConverterArray[cs], tempBuf, 2, FALSE);
                        } else if (!(trailIsOk || IS_2022_CONTROL(trailByte))) {
                            
                            ++mySource;
                            
                            mySourceChar = 0x10000 | (mySourceChar << 8) | trailByte;
                        }
                    } else {
                        args->converter->toUBytes[0] = (uint8_t)mySourceChar;
                        args->converter->toULength = 1;
                        goto endloop;
                    }
                }  
                break;
            }  
            if(targetUniChar < (missingCharMarker-1)){
                if(args->offsets){
                    args->offsets[myTarget - args->target] = (int32_t)(mySource - args->source - (mySourceChar <= 0xff ? 1 : 2));
                }
                *(myTarget++)=(UChar)targetUniChar;
            }
            else if(targetUniChar > missingCharMarker){
                
                targetUniChar-=0x0010000;
                *myTarget = (UChar)(0xd800+(UChar)(targetUniChar>>10));
                if(args->offsets){
                    args->offsets[myTarget - args->target] = (int32_t)(mySource - args->source - (mySourceChar <= 0xff ? 1 : 2));
                }
                ++myTarget;
                if(myTarget< args->targetLimit){
                    *myTarget = (UChar)(0xdc00+(UChar)(targetUniChar&0x3ff));
                    if(args->offsets){
                        args->offsets[myTarget - args->target] = (int32_t)(mySource - args->source - (mySourceChar <= 0xff ? 1 : 2));
                    }
                    ++myTarget;
                }else{
                    args->converter->UCharErrorBuffer[args->converter->UCharErrorBufferLength++]=
                                    (UChar)(0xdc00+(UChar)(targetUniChar&0x3ff));
                }

            }
            else{
                
                toUnicodeCallback(args->converter,mySourceChar,targetUniChar,err);
                break;
            }
        }
        else{    
            *err =U_BUFFER_OVERFLOW_ERROR;
            break;
        }
    }
endloop:
    args->target = myTarget;
    args->source = mySource;
}










static void
UConverter_fromUnicode_ISO_2022_KR_OFFSETS_LOGIC_IBM(UConverterFromUnicodeArgs* args, UErrorCode* err){

    UConverter* saveConv = args->converter;
    UConverterDataISO2022 *myConverterData=(UConverterDataISO2022*)saveConv->extraInfo;
    args->converter=myConverterData->currentConverter;

    myConverterData->currentConverter->fromUChar32 = saveConv->fromUChar32;
    ucnv_MBCSFromUnicodeWithOffsets(args,err);
    saveConv->fromUChar32 = myConverterData->currentConverter->fromUChar32;

    if(*err == U_BUFFER_OVERFLOW_ERROR) {
        if(myConverterData->currentConverter->charErrorBufferLength > 0) {
            uprv_memcpy(
                saveConv->charErrorBuffer,
                myConverterData->currentConverter->charErrorBuffer,
                myConverterData->currentConverter->charErrorBufferLength);
        }
        saveConv->charErrorBufferLength = myConverterData->currentConverter->charErrorBufferLength;
        myConverterData->currentConverter->charErrorBufferLength = 0;
    }
    args->converter=saveConv;
}

static void
UConverter_fromUnicode_ISO_2022_KR_OFFSETS_LOGIC(UConverterFromUnicodeArgs* args, UErrorCode* err){

    const UChar *source = args->source;
    const UChar *sourceLimit = args->sourceLimit;
    unsigned char *target = (unsigned char *) args->target;
    unsigned char *targetLimit = (unsigned char *) args->targetLimit;
    int32_t* offsets = args->offsets;
    uint32_t targetByteUnit = 0x0000;
    UChar32 sourceChar = 0x0000;
    UBool isTargetByteDBCS;
    UBool oldIsTargetByteDBCS;
    UConverterDataISO2022 *converterData;
    UConverterSharedData* sharedData;
    UBool useFallback;
    int32_t length =0;

    converterData=(UConverterDataISO2022*)args->converter->extraInfo;
    



    if(converterData->version==1){
        UConverter_fromUnicode_ISO_2022_KR_OFFSETS_LOGIC_IBM(args,err);
        return;
    }

    
    sharedData = converterData->currentConverter->sharedData;
    useFallback = args->converter->useFallback;
    isTargetByteDBCS=(UBool)args->converter->fromUnicodeStatus;
    oldIsTargetByteDBCS = isTargetByteDBCS;

    isTargetByteDBCS   = (UBool) args->converter->fromUnicodeStatus;
    if((sourceChar = args->converter->fromUChar32)!=0 && target <targetLimit) {
        goto getTrail;
    }
    while(source < sourceLimit){

        targetByteUnit = missingCharMarker;

        if(target < (unsigned char*) args->targetLimit){
            sourceChar = *source++;

            
            if(IS_2022_CONTROL(sourceChar)) {
                
                *err=U_ILLEGAL_CHAR_FOUND;
                args->converter->fromUChar32=sourceChar;
                break;
            }

            length = MBCS_FROM_UCHAR32_ISO2022(sharedData,sourceChar,&targetByteUnit,useFallback,MBCS_OUTPUT_2);
            if(length < 0) {
                length = -length;  
            }
            
            
            if( length > 2 || length==0 ||
                (length == 1 && targetByteUnit > 0x7f) ||
                (length == 2 &&
                    ((uint16_t)(targetByteUnit - 0xa1a1) > (0xfefe - 0xa1a1) ||
                    (uint8_t)(targetByteUnit - 0xa1) > (0xfe - 0xa1)))
            ) {
                targetByteUnit=missingCharMarker;
            }
            if (targetByteUnit != missingCharMarker){

                oldIsTargetByteDBCS = isTargetByteDBCS;
                isTargetByteDBCS = (UBool)(targetByteUnit>0x00FF);
                  
                if (oldIsTargetByteDBCS != isTargetByteDBCS ){

                    if (isTargetByteDBCS)
                        *target++ = UCNV_SO;
                    else
                        *target++ = UCNV_SI;
                    if(offsets)
                        *(offsets++) = (int32_t)(source - args->source-1);
                }
                
                if(targetByteUnit <= 0x00FF){
                    if( target < targetLimit){
                        *(target++) = (unsigned char) targetByteUnit;
                        if(offsets){
                            *(offsets++) = (int32_t)(source - args->source-1);
                        }

                    }else{
                        args->converter->charErrorBuffer[args->converter->charErrorBufferLength++] = (unsigned char) (targetByteUnit);
                        *err = U_BUFFER_OVERFLOW_ERROR;
                    }
                }else{
                    if(target < targetLimit){
                        *(target++) =(unsigned char) ((targetByteUnit>>8) -0x80);
                        if(offsets){
                            *(offsets++) = (int32_t)(source - args->source-1);
                        }
                        if(target < targetLimit){
                            *(target++) =(unsigned char) (targetByteUnit -0x80);
                            if(offsets){
                                *(offsets++) = (int32_t)(source - args->source-1);
                            }
                        }else{
                            args->converter->charErrorBuffer[args->converter->charErrorBufferLength++] = (unsigned char) (targetByteUnit -0x80);
                            *err = U_BUFFER_OVERFLOW_ERROR;
                        }
                    }else{
                        args->converter->charErrorBuffer[args->converter->charErrorBufferLength++] = (unsigned char) ((targetByteUnit>>8) -0x80);
                        args->converter->charErrorBuffer[args->converter->charErrorBufferLength++] = (unsigned char) (targetByteUnit-0x80);
                        *err = U_BUFFER_OVERFLOW_ERROR;
                    }
                }

            }
            else{
                



                
                if(U16_IS_SURROGATE(sourceChar)) {
                    if(U16_IS_SURROGATE_LEAD(sourceChar)) {
getTrail:
                        
                        if(source <  sourceLimit) {
                            
                            UChar trail=(UChar) *source;
                            if(U16_IS_TRAIL(trail)) {
                                source++;
                                sourceChar=U16_GET_SUPPLEMENTARY(sourceChar, trail);
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

                args->converter->fromUChar32=sourceChar;
                break;
            }
        } 
        else{
            *err =U_BUFFER_OVERFLOW_ERROR;
            break;
        }

    }

    









    if( U_SUCCESS(*err) &&
        isTargetByteDBCS &&
        args->flush && source>=sourceLimit && args->converter->fromUChar32==0
    ) {
        int32_t sourceIndex;

        
        isTargetByteDBCS=FALSE;

        
        






        sourceIndex=(int32_t)(source-args->source);
        if(sourceIndex>0) {
            --sourceIndex;
            if( U16_IS_TRAIL(args->source[sourceIndex]) &&
                (sourceIndex==0 || U16_IS_LEAD(args->source[sourceIndex-1]))
            ) {
                --sourceIndex;
            }
        } else {
            sourceIndex=-1;
        }

        fromUWriteUInt8(
            args->converter,
            SHIFT_IN_STR, 1,
            &target, (const char *)targetLimit,
            &offsets, sourceIndex,
            err);
    }

    
    args->source = source;
    args->target = (char*)target;
    args->converter->fromUnicodeStatus = (uint32_t)isTargetByteDBCS;
}



static void
UConverter_toUnicode_ISO_2022_KR_OFFSETS_LOGIC_IBM(UConverterToUnicodeArgs *args,
                                                            UErrorCode* err){
    char const* sourceStart;
    UConverterDataISO2022* myData=(UConverterDataISO2022*)(args->converter->extraInfo);

    UConverterToUnicodeArgs subArgs;
    int32_t minArgsSize;

    
    if(args->size<sizeof(UConverterToUnicodeArgs)) {
        minArgsSize = args->size;
    } else {
        minArgsSize = (int32_t)sizeof(UConverterToUnicodeArgs);
    }

    uprv_memcpy(&subArgs, args, minArgsSize);
    subArgs.size = (uint16_t)minArgsSize;
    subArgs.converter = myData->currentConverter;

    
    sourceStart = args->source;

    if(myData->key != 0) {
        
        goto escape;
    }

    while(U_SUCCESS(*err) && args->source < args->sourceLimit) {
        
        subArgs.source = args->source;
        subArgs.sourceLimit = getEndOfBuffer_2022(&(args->source), args->sourceLimit, args->flush);
        if(subArgs.source != subArgs.sourceLimit) {
            






            if(args->converter->toULength > 0) {
                uprv_memcpy(subArgs.converter->toUBytes, args->converter->toUBytes, args->converter->toULength);
            }
            subArgs.converter->toULength = args->converter->toULength;

            




            ucnv_MBCSToUnicodeWithOffsets(&subArgs, err);

            if(args->offsets != NULL && sourceStart != args->source) {
                
                int32_t *offsets = args->offsets;
                UChar *target = args->target;
                int32_t delta = (int32_t)(args->source - sourceStart);
                while(target < subArgs.target) {
                    if(*offsets >= 0) {
                        *offsets += delta;
                    }
                    ++offsets;
                    ++target;
                }
            }
            args->source = subArgs.source;
            args->target = subArgs.target;
            args->offsets = subArgs.offsets;

            
            if(subArgs.converter->toULength > 0) {
                uprv_memcpy(args->converter->toUBytes, subArgs.converter->toUBytes, subArgs.converter->toULength);
            }
            args->converter->toULength = subArgs.converter->toULength;

            if(*err == U_BUFFER_OVERFLOW_ERROR) {
                if(subArgs.converter->UCharErrorBufferLength > 0) {
                    uprv_memcpy(args->converter->UCharErrorBuffer, subArgs.converter->UCharErrorBuffer,
                                subArgs.converter->UCharErrorBufferLength);
                }
                args->converter->UCharErrorBufferLength=subArgs.converter->UCharErrorBufferLength;
                subArgs.converter->UCharErrorBufferLength = 0;
            }
        }

        if (U_FAILURE(*err) || (args->source == args->sourceLimit)) {
            return;
        }

escape:
        changeState_2022(args->converter,
               &(args->source),
               args->sourceLimit,
               ISO_2022_KR,
               err);
    }
}

static void
UConverter_toUnicode_ISO_2022_KR_OFFSETS_LOGIC(UConverterToUnicodeArgs *args,
                                                            UErrorCode* err){
    char tempBuf[2];
    const char *mySource = ( char *) args->source;
    UChar *myTarget = args->target;
    const char *mySourceLimit = args->sourceLimit;
    UChar32 targetUniChar = 0x0000;
    UChar mySourceChar = 0x0000;
    UConverterDataISO2022* myData;
    UConverterSharedData* sharedData ;
    UBool useFallback;

    myData=(UConverterDataISO2022*)(args->converter->extraInfo);
    if(myData->version==1){
        UConverter_toUnicode_ISO_2022_KR_OFFSETS_LOGIC_IBM(args,err);
        return;
    }

    
    sharedData = myData->currentConverter->sharedData;
    useFallback = args->converter->useFallback;

    if(myData->key != 0) {
        
        goto escape;
    } else if(args->converter->toULength == 1 && mySource < mySourceLimit && myTarget < args->targetLimit) {
        
        mySourceChar = args->converter->toUBytes[0];
        args->converter->toULength = 0;
        goto getTrailByte;
    }

    while(mySource< mySourceLimit){

        if(myTarget < args->targetLimit){

            mySourceChar= (unsigned char) *mySource++;

            if(mySourceChar==UCNV_SI){
                myData->toU2022State.g = 0;
                if (myData->isEmptySegment) {
                    myData->isEmptySegment = FALSE;	
                    *err = U_ILLEGAL_ESCAPE_SEQUENCE;
                    args->converter->toUCallbackReason = UCNV_IRREGULAR;
                    args->converter->toUBytes[0] = (uint8_t)mySourceChar;
                    args->converter->toULength = 1;
                    args->target = myTarget;
                    args->source = mySource;
                    return;
                }
                
                continue;
            }else if(mySourceChar==UCNV_SO){
                myData->toU2022State.g = 1;
                myData->isEmptySegment = TRUE;	
                
                continue;
            }else if(mySourceChar==ESC_2022){
                mySource--;
escape:
                myData->isEmptySegment = FALSE;	
                changeState_2022(args->converter,&(mySource),
                                mySourceLimit, ISO_2022_KR, err);
                if(U_FAILURE(*err)){
                    args->target = myTarget;
                    args->source = mySource;
                    return;
                }
                continue;
            }

            myData->isEmptySegment = FALSE;	
            if(myData->toU2022State.g == 1) {
                if(mySource < mySourceLimit) {
                    int leadIsOk, trailIsOk;
                    uint8_t trailByte;
getTrailByte:
                    targetUniChar = missingCharMarker;
                    trailByte = (uint8_t)*mySource;
                    









                    leadIsOk = (uint8_t)(mySourceChar - 0x21) <= (0x7e - 0x21);
                    trailIsOk = (uint8_t)(trailByte - 0x21) <= (0x7e - 0x21);
                    if (leadIsOk && trailIsOk) {
                        ++mySource;
                        tempBuf[0] = (char)(mySourceChar + 0x80);
                        tempBuf[1] = (char)(trailByte + 0x80);
                        targetUniChar = ucnv_MBCSSimpleGetNextUChar(sharedData, tempBuf, 2, useFallback);
                        mySourceChar = (mySourceChar << 8) | trailByte;
                    } else if (!(trailIsOk || IS_2022_CONTROL(trailByte))) {
                        
                        ++mySource;
                        
                        mySourceChar = 0x10000 | (mySourceChar << 8) | trailByte;
                    }
                } else {
                    args->converter->toUBytes[0] = (uint8_t)mySourceChar;
                    args->converter->toULength = 1;
                    break;
                }
            }
            else if(mySourceChar <= 0x7f) {
                targetUniChar = ucnv_MBCSSimpleGetNextUChar(sharedData, mySource - 1, 1, useFallback);
            } else {
                targetUniChar = 0xffff;
            }
            if(targetUniChar < 0xfffe){
                if(args->offsets) {
                    args->offsets[myTarget - args->target] = (int32_t)(mySource - args->source - (mySourceChar <= 0xff ? 1 : 2));
                }
                *(myTarget++)=(UChar)targetUniChar;
            }
            else {
                
                toUnicodeCallback(args->converter,mySourceChar,targetUniChar,err);
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



















































































static const char GB_2312_80_STR[] = "\x1B\x24\x29\x41";
static const char ISO_IR_165_STR[] = "\x1B\x24\x29\x45";
static const char CNS_11643_1992_Plane_1_STR[] = "\x1B\x24\x29\x47";
static const char CNS_11643_1992_Plane_2_STR[] = "\x1B\x24\x2A\x48";
static const char CNS_11643_1992_Plane_3_STR[] = "\x1B\x24\x2B\x49";
static const char CNS_11643_1992_Plane_4_STR[] = "\x1B\x24\x2B\x4A";
static const char CNS_11643_1992_Plane_5_STR[] = "\x1B\x24\x2B\x4B";
static const char CNS_11643_1992_Plane_6_STR[] = "\x1B\x24\x2B\x4C";
static const char CNS_11643_1992_Plane_7_STR[] = "\x1B\x24\x2B\x4D";


static const char* const escSeqCharsCN[10] ={
        SHIFT_IN_STR,                   
        GB_2312_80_STR,                 
        ISO_IR_165_STR,                 
        CNS_11643_1992_Plane_1_STR,
        CNS_11643_1992_Plane_2_STR,
        CNS_11643_1992_Plane_3_STR,
        CNS_11643_1992_Plane_4_STR,
        CNS_11643_1992_Plane_5_STR,
        CNS_11643_1992_Plane_6_STR,
        CNS_11643_1992_Plane_7_STR
};

static void
UConverter_fromUnicode_ISO_2022_CN_OFFSETS_LOGIC(UConverterFromUnicodeArgs* args, UErrorCode* err){
    UConverter *cnv = args->converter;
    UConverterDataISO2022 *converterData;
    ISO2022State *pFromU2022State;
    uint8_t *target = (uint8_t *) args->target;
    const uint8_t *targetLimit = (const uint8_t *) args->targetLimit;
    const UChar* source = args->source;
    const UChar* sourceLimit = args->sourceLimit;
    int32_t* offsets = args->offsets;
    UChar32 sourceChar;
    char buffer[8];
    int32_t len;
    int8_t choices[3];
    int32_t choiceCount;
    uint32_t targetValue = 0;
    UBool useFallback;

    
    converterData     = (UConverterDataISO2022*)cnv->extraInfo;
    pFromU2022State   = &converterData->fromU2022State;

    choiceCount = 0;

    
    if((sourceChar = cnv->fromUChar32)!=0 && target< targetLimit) {
        goto getTrail;
    }

    while( source < sourceLimit){
        if(target < targetLimit){

            sourceChar  = *(source++);
            
             if(U16_IS_SURROGATE(sourceChar)) {
                if(U16_IS_SURROGATE_LEAD(sourceChar)) {
getTrail:
                    
                    if(source < sourceLimit) {
                        
                        UChar trail=(UChar) *source;
                        if(U16_IS_TRAIL(trail)) {
                            source++;
                            sourceChar=U16_GET_SUPPLEMENTARY(sourceChar, trail);
                            cnv->fromUChar32=0x00;
                            
                            
                        } else {
                            
                            
                            *err=U_ILLEGAL_CHAR_FOUND;
                            cnv->fromUChar32=sourceChar;
                            break;
                        }
                    } else {
                        
                        cnv->fromUChar32=sourceChar;
                        break;
                    }
                } else {
                    
                    
                    *err=U_ILLEGAL_CHAR_FOUND;
                    cnv->fromUChar32=sourceChar;
                    break;
                }
            }

            
            if(sourceChar <= 0x007f ){
                
                if(IS_2022_CONTROL(sourceChar)) {
                    
                    *err=U_ILLEGAL_CHAR_FOUND;
                    cnv->fromUChar32=sourceChar;
                    break;
                }

                
                if(pFromU2022State->g == 0) {
                    buffer[0] = (char)sourceChar;
                    len = 1;
                } else {
                    buffer[0] = UCNV_SI;
                    buffer[1] = (char)sourceChar;
                    len = 2;
                    pFromU2022State->g = 0;
                    choiceCount = 0;
                }
                if(sourceChar == CR || sourceChar == LF) {
                    
                    uprv_memset(pFromU2022State, 0, sizeof(ISO2022State));
                    choiceCount = 0;
                }
            }
            else{
                
                int32_t i;
                int8_t cs, g;

                if(choiceCount == 0) {
                    
                    choices[0] = pFromU2022State->cs[1];

                    
                    if(choices[0] == 0) {
                        choices[0] = GB2312_1;
                    }

                    if(converterData->version == 0) {
                        

                        
                        if(choices[0] == GB2312_1) {
                            choices[1] = (int8_t)CNS_11643_1;
                        } else {
                            choices[1] = (int8_t)GB2312_1;
                        }

                        choiceCount = 2;
                    } else if (converterData->version == 1) {
                        

                        
                        switch(choices[0]) {
                        case GB2312_1:
                            choices[1] = (int8_t)CNS_11643_1;
                            choices[2] = (int8_t)ISO_IR_165;
                            break;
                        case ISO_IR_165:
                            choices[1] = (int8_t)GB2312_1;
                            choices[2] = (int8_t)CNS_11643_1;
                            break;
                        default: 
                            choices[1] = (int8_t)GB2312_1;
                            choices[2] = (int8_t)ISO_IR_165;
                            break;
                        }

                        choiceCount = 3;
                    } else {
                        choices[0] = (int8_t)CNS_11643_1;
                        choices[1] = (int8_t)GB2312_1;
                    }
                }

                cs = g = 0;
                




                len = 0;
                





                useFallback = cnv->useFallback;

                for(i = 0; i < choiceCount && len <= 0; ++i) {
                    int8_t cs0 = choices[i];
                    if(cs0 > 0) {
                        uint32_t value;
                        int32_t len2;
                        if(cs0 >= CNS_11643_0) {
                            len2 = MBCS_FROM_UCHAR32_ISO2022(
                                        converterData->myConverterArray[CNS_11643],
                                        sourceChar,
                                        &value,
                                        useFallback,
                                        MBCS_OUTPUT_3);
                            if(len2 == 3 || (len2 == -3 && len == 0)) {
                                targetValue = value;
                                cs = (int8_t)(CNS_11643_0 + (value >> 16) - 0x80);
                                if(len2 >= 0) {
                                    len = 2;
                                } else {
                                    len = -2;
                                    useFallback = FALSE;
                                }
                                if(cs == CNS_11643_1) {
                                    g = 1;
                                } else if(cs == CNS_11643_2) {
                                    g = 2;
                                } else  if(converterData->version == 1) {
                                    g = 3;
                                } else {
                                    
                                    len = 0;
                                }
                            }
                        } else {
                            
                            U_ASSERT(cs0<UCNV_2022_MAX_CONVERTERS);
                            len2 = MBCS_FROM_UCHAR32_ISO2022(
                                        converterData->myConverterArray[cs0],
                                        sourceChar,
                                        &value,
                                        useFallback,
                                        MBCS_OUTPUT_2);
                            if(len2 == 2 || (len2 == -2 && len == 0)) {
                                targetValue = value;
                                len = len2;
                                cs = cs0;
                                g = 1;
                                useFallback = FALSE;
                            }
                        }
                    }
                }

                if(len != 0) {
                    len = 0; 

                    
                    if(cs != pFromU2022State->cs[g]) {
                        if(cs < CNS_11643) {
                            uprv_memcpy(buffer, escSeqCharsCN[cs], 4);
                        } else {
                            U_ASSERT(cs >= CNS_11643_1);
                            uprv_memcpy(buffer, escSeqCharsCN[CNS_11643 + (cs - CNS_11643_1)], 4);
                        }
                        len = 4;
                        pFromU2022State->cs[g] = cs;
                        if(g == 1) {
                            
                            choiceCount = 0;
                        }
                    }

                    
                    if(g != pFromU2022State->g) {
                        switch(g) {
                        case 1:
                            buffer[len++] = UCNV_SO;

                            
                            pFromU2022State->g = 1;
                            break;
                        case 2:
                            buffer[len++] = 0x1b;
                            buffer[len++] = 0x4e;
                            break;
                        default: 
                            buffer[len++] = 0x1b;
                            buffer[len++] = 0x4f;
                            break;
                        }
                    }

                    
                    buffer[len++] = (char)(targetValue >> 8);
                    buffer[len++] = (char)targetValue;
                } else {
                    


                    *err = U_INVALID_CHAR_FOUND;
                    cnv->fromUChar32=sourceChar;
                    break;
                }
            }

            
            if(len == 1) {
                *target++ = buffer[0];
                if(offsets) {
                    *offsets++ = (int32_t)(source - args->source - 1); 
                }
            } else if(len == 2 && (target + 2) <= targetLimit) {
                *target++ = buffer[0];
                *target++ = buffer[1];
                if(offsets) {
                    int32_t sourceIndex = (int32_t)(source - args->source - U16_LENGTH(sourceChar));
                    *offsets++ = sourceIndex;
                    *offsets++ = sourceIndex;
                }
            } else {
                fromUWriteUInt8(
                    cnv,
                    buffer, len,
                    &target, (const char *)targetLimit,
                    &offsets, (int32_t)(source - args->source - U16_LENGTH(sourceChar)),
                    err);
                if(U_FAILURE(*err)) {
                    break;
                }
            }
        } 
        else{
            *err =U_BUFFER_OVERFLOW_ERROR;
            break;
        }

    }

    









    if( U_SUCCESS(*err) &&
        pFromU2022State->g!=0 &&
        args->flush && source>=sourceLimit && cnv->fromUChar32==0
    ) {
        int32_t sourceIndex;

        
        pFromU2022State->g=0;

        
        






        sourceIndex=(int32_t)(source-args->source);
        if(sourceIndex>0) {
            --sourceIndex;
            if( U16_IS_TRAIL(args->source[sourceIndex]) &&
                (sourceIndex==0 || U16_IS_LEAD(args->source[sourceIndex-1]))
            ) {
                --sourceIndex;
            }
        } else {
            sourceIndex=-1;
        }

        fromUWriteUInt8(
            cnv,
            SHIFT_IN_STR, 1,
            &target, (const char *)targetLimit,
            &offsets, sourceIndex,
            err);
    }

    
    args->source = source;
    args->target = (char*)target;
}


static void
UConverter_toUnicode_ISO_2022_CN_OFFSETS_LOGIC(UConverterToUnicodeArgs *args,
                                               UErrorCode* err){
    char tempBuf[3];
    const char *mySource = (char *) args->source;
    UChar *myTarget = args->target;
    const char *mySourceLimit = args->sourceLimit;
    uint32_t targetUniChar = 0x0000;
    uint32_t mySourceChar = 0x0000;
    UConverterDataISO2022* myData;
    ISO2022State *pToU2022State;

    myData=(UConverterDataISO2022*)(args->converter->extraInfo);
    pToU2022State = &myData->toU2022State;

    if(myData->key != 0) {
        
        goto escape;
    } else if(args->converter->toULength == 1 && mySource < mySourceLimit && myTarget < args->targetLimit) {
        
        mySourceChar = args->converter->toUBytes[0];
        args->converter->toULength = 0;
        targetUniChar = missingCharMarker;
        goto getTrailByte;
    }

    while(mySource < mySourceLimit){

        targetUniChar =missingCharMarker;

        if(myTarget < args->targetLimit){

            mySourceChar= (unsigned char) *mySource++;

            switch(mySourceChar){
            case UCNV_SI:
                pToU2022State->g=0;
                if (myData->isEmptySegment) {
                    myData->isEmptySegment = FALSE;	
                    *err = U_ILLEGAL_ESCAPE_SEQUENCE;
                    args->converter->toUCallbackReason = UCNV_IRREGULAR;
                    args->converter->toUBytes[0] = mySourceChar;
                    args->converter->toULength = 1;
                    args->target = myTarget;
                    args->source = mySource;
                    return;
                }
                continue;

            case UCNV_SO:
                if(pToU2022State->cs[1] != 0) {
                    pToU2022State->g=1;
                    myData->isEmptySegment = TRUE;	
                    continue;
                } else {
                    
                    myData->isEmptySegment = FALSE;	
                    break;
                }

            case ESC_2022:
                mySource--;
escape:
                {
                    const char * mySourceBefore = mySource;
                    int8_t toULengthBefore = args->converter->toULength;

                    changeState_2022(args->converter,&(mySource),
                        mySourceLimit, ISO_2022_CN,err);

                    
                    if(myData->key==0 && U_SUCCESS(*err) && myData->isEmptySegment) {
                        *err = U_ILLEGAL_ESCAPE_SEQUENCE;
                        args->converter->toUCallbackReason = UCNV_IRREGULAR;
                        args->converter->toULength = (int8_t)(toULengthBefore + (mySource - mySourceBefore));
                    }
                }

                
                if(U_FAILURE(*err)){
                    args->target = myTarget;
                    args->source = mySource;
                    myData->isEmptySegment = FALSE;	
                    return;
                }
                continue;

            

            case CR:
                
            case LF:
                uprv_memset(pToU2022State, 0, sizeof(ISO2022State));
                
            default:
                
                myData->isEmptySegment = FALSE;
                if(pToU2022State->g != 0) {
                    if(mySource < mySourceLimit) {
                        UConverterSharedData *cnv;
                        StateEnum tempState;
                        int32_t tempBufLen;
                        int leadIsOk, trailIsOk;
                        uint8_t trailByte;
getTrailByte:
                        trailByte = (uint8_t)*mySource;
                        









                        leadIsOk = (uint8_t)(mySourceChar - 0x21) <= (0x7e - 0x21);
                        trailIsOk = (uint8_t)(trailByte - 0x21) <= (0x7e - 0x21);
                        if (leadIsOk && trailIsOk) {
                            ++mySource;
                            tempState = (StateEnum)pToU2022State->cs[pToU2022State->g];
                            if(tempState >= CNS_11643_0) {
                                cnv = myData->myConverterArray[CNS_11643];
                                tempBuf[0] = (char) (0x80+(tempState-CNS_11643_0));
                                tempBuf[1] = (char) (mySourceChar);
                                tempBuf[2] = (char) trailByte;
                                tempBufLen = 3;

                            }else{
                                U_ASSERT(tempState<UCNV_2022_MAX_CONVERTERS);
                                cnv = myData->myConverterArray[tempState];
                                tempBuf[0] = (char) (mySourceChar);
                                tempBuf[1] = (char) trailByte;
                                tempBufLen = 2;
                            }
                            targetUniChar = ucnv_MBCSSimpleGetNextUChar(cnv, tempBuf, tempBufLen, FALSE);
                            mySourceChar = (mySourceChar << 8) | trailByte;
                        } else if (!(trailIsOk || IS_2022_CONTROL(trailByte))) {
                            
                            ++mySource;
                            
                            mySourceChar = 0x10000 | (mySourceChar << 8) | trailByte;
                        }
                        if(pToU2022State->g>=2) {
                            
                            pToU2022State->g=pToU2022State->prevG;
                        }
                    } else {
                        args->converter->toUBytes[0] = (uint8_t)mySourceChar;
                        args->converter->toULength = 1;
                        goto endloop;
                    }
                }
                else{
                    if(mySourceChar <= 0x7f) {
                        targetUniChar = (UChar) mySourceChar;
                    }
                }
                break;
            }
            if(targetUniChar < (missingCharMarker-1)){
                if(args->offsets){
                    args->offsets[myTarget - args->target] = (int32_t)(mySource - args->source - (mySourceChar <= 0xff ? 1 : 2));
                }
                *(myTarget++)=(UChar)targetUniChar;
            }
            else if(targetUniChar > missingCharMarker){
                
                targetUniChar-=0x0010000;
                *myTarget = (UChar)(0xd800+(UChar)(targetUniChar>>10));
                if(args->offsets){
                    args->offsets[myTarget - args->target] = (int32_t)(mySource - args->source - (mySourceChar <= 0xff ? 1 : 2));
                }
                ++myTarget;
                if(myTarget< args->targetLimit){
                    *myTarget = (UChar)(0xdc00+(UChar)(targetUniChar&0x3ff));
                    if(args->offsets){
                        args->offsets[myTarget - args->target] = (int32_t)(mySource - args->source - (mySourceChar <= 0xff ? 1 : 2));
                    }
                    ++myTarget;
                }else{
                    args->converter->UCharErrorBuffer[args->converter->UCharErrorBufferLength++]=
                                    (UChar)(0xdc00+(UChar)(targetUniChar&0x3ff));
                }

            }
            else{
                
                toUnicodeCallback(args->converter,mySourceChar,targetUniChar,err);
                break;
            }
        }
        else{
            *err =U_BUFFER_OVERFLOW_ERROR;
            break;
        }
    }
endloop:
    args->target = myTarget;
    args->source = mySource;
}

static void
_ISO_2022_WriteSub(UConverterFromUnicodeArgs *args, int32_t offsetIndex, UErrorCode *err) {
    UConverter *cnv = args->converter;
    UConverterDataISO2022 *myConverterData=(UConverterDataISO2022 *) cnv->extraInfo;
    ISO2022State *pFromU2022State=&myConverterData->fromU2022State;
    char *p, *subchar;
    char buffer[8];
    int32_t length;

    subchar=(char *)cnv->subChars;
    length=cnv->subCharLen; 

    p = buffer;
    switch(myConverterData->locale[0]){
    case 'j':
        {
            int8_t cs;

            if(pFromU2022State->g == 1) {
                
                pFromU2022State->g = 0;
                *p++ = UCNV_SI;
            }

            cs = pFromU2022State->cs[0];
            if(cs != ASCII && cs != JISX201) {
                
                pFromU2022State->cs[0] = (int8_t)ASCII;
                *p++ = '\x1b';
                *p++ = '\x28';
                *p++ = '\x42';
            }

            *p++ = subchar[0];
            break;
        }
    case 'c':
        if(pFromU2022State->g != 0) {
            
            pFromU2022State->g = 0;
            *p++ = UCNV_SI;
        }
        *p++ = subchar[0];
        break;
    case 'k':
        if(myConverterData->version == 0) {
            if(length == 1) {
                if((UBool)args->converter->fromUnicodeStatus) {
                    
                    args->converter->fromUnicodeStatus = 0;
                    *p++ = UCNV_SI;
                }
                *p++ = subchar[0];
            } else  {
                if(!(UBool)args->converter->fromUnicodeStatus) {
                    
                    args->converter->fromUnicodeStatus = 1;
                    *p++ = UCNV_SO;
                }
                *p++ = subchar[0];
                *p++ = subchar[1];
            }
            break;
        } else {
            
            uint8_t *currentSubChars = myConverterData->currentConverter->subChars;
            int8_t currentSubCharLen = myConverterData->currentConverter->subCharLen;

            
            myConverterData->currentConverter->subChars = (uint8_t *)subchar;
            myConverterData->currentConverter->subCharLen = (int8_t)length;

            
            args->converter = myConverterData->currentConverter;
            myConverterData->currentConverter->fromUChar32 = cnv->fromUChar32;
            ucnv_cbFromUWriteSub(args, 0, err);
            cnv->fromUChar32 = myConverterData->currentConverter->fromUChar32;
            args->converter = cnv;

            
            myConverterData->currentConverter->subChars = currentSubChars;
            myConverterData->currentConverter->subCharLen = currentSubCharLen;

            if(*err == U_BUFFER_OVERFLOW_ERROR) {
                if(myConverterData->currentConverter->charErrorBufferLength > 0) {
                    uprv_memcpy(
                        cnv->charErrorBuffer,
                        myConverterData->currentConverter->charErrorBuffer,
                        myConverterData->currentConverter->charErrorBufferLength);
                }
                cnv->charErrorBufferLength = myConverterData->currentConverter->charErrorBufferLength;
                myConverterData->currentConverter->charErrorBufferLength = 0;
            }
            return;
        }
    default:
        
        break;
    }
    ucnv_cbFromUWriteBytes(args,
                           buffer, (int32_t)(p - buffer),
                           offsetIndex, err);
}












struct cloneStruct
{
    UConverter cnv;
    UConverter currentConverter;
    UAlignedMemory deadSpace;
    UConverterDataISO2022 mydata;
};


static UConverter *
_ISO_2022_SafeClone(
            const UConverter *cnv,
            void *stackBuffer,
            int32_t *pBufferSize,
            UErrorCode *status)
{
    struct cloneStruct * localClone;
    UConverterDataISO2022 *cnvData;
    int32_t i, size;

    if (*pBufferSize == 0) { 
        *pBufferSize = (int32_t)sizeof(struct cloneStruct);
        return NULL;
    }

    cnvData = (UConverterDataISO2022 *)cnv->extraInfo;
    localClone = (struct cloneStruct *)stackBuffer;

    

    uprv_memcpy(&localClone->mydata, cnvData, sizeof(UConverterDataISO2022));
    localClone->cnv.extraInfo = &localClone->mydata; 
    localClone->cnv.isExtraLocal = TRUE;

    

    if(cnvData->currentConverter != NULL) {
        size = (int32_t)(sizeof(UConverter) + sizeof(UAlignedMemory)); 
        localClone->mydata.currentConverter =
            ucnv_safeClone(cnvData->currentConverter,
                            &localClone->currentConverter,
                            &size, status);
        if(U_FAILURE(*status)) {
            return NULL;
        }
    }

    for(i=0; i<UCNV_2022_MAX_CONVERTERS; ++i) {
        if(cnvData->myConverterArray[i] != NULL) {
            ucnv_incrementRefCount(cnvData->myConverterArray[i]);
        }
    }

    return &localClone->cnv;
}

static void
_ISO_2022_GetUnicodeSet(const UConverter *cnv,
                    const USetAdder *sa,
                    UConverterUnicodeSet which,
                    UErrorCode *pErrorCode)
{
    int32_t i;
    UConverterDataISO2022* cnvData;

    if (U_FAILURE(*pErrorCode)) {
        return;
    }
#ifdef U_ENABLE_GENERIC_ISO_2022
    if (cnv->sharedData == &_ISO2022Data) {
        
        sa->addRange(sa->set, 0, 0xd7FF);
        sa->addRange(sa->set, 0xE000, 0x10FFFF);
        return;
    }
#endif

    cnvData = (UConverterDataISO2022*)cnv->extraInfo;

    
    switch(cnvData->locale[0]){
    case 'j':
        
        sa->add(sa->set, 0xa5);
        sa->add(sa->set, 0x203e);
        if(jpCharsetMasks[cnvData->version]&CSM(ISO8859_1)) {
            
            sa->addRange(sa->set, 0, 0xff);
        } else {
            
            sa->addRange(sa->set, 0, 0x7f);
        }
        if(cnvData->version==3 || cnvData->version==4 || which==UCNV_ROUNDTRIP_AND_FALLBACK_SET) {
            












            
            sa->addRange(sa->set, HWKANA_START, HWKANA_END);
        }
        break;
    case 'c':
    case 'z':
        
        sa->addRange(sa->set, 0, 0x7f);
        break;
    case 'k':
        
        cnvData->currentConverter->sharedData->impl->getUnicodeSet(
                cnvData->currentConverter, sa, which, pErrorCode);
        
        break;
    default:
        break;
    }

#if 0  
            if( (cnvData->locale[0]=='c' || cnvData->locale[0]=='z') &&
                cnvData->version==0 && i==CNS_11643
            ) {
                
                ucnv_MBCSGetUnicodeSetForBytes(
                        cnvData->myConverterArray[i],
                        sa, UCNV_ROUNDTRIP_SET,
                        0, 0x81, 0x82,
                        pErrorCode);
            }
#endif

    for (i=0; i<UCNV_2022_MAX_CONVERTERS; i++) {
        UConverterSetFilter filter;
        if(cnvData->myConverterArray[i]!=NULL) {
            if( (cnvData->locale[0]=='c' || cnvData->locale[0]=='z') &&
                cnvData->version==0 && i==CNS_11643
            ) {
                






                filter=UCNV_SET_FILTER_2022_CN;
            } else if(cnvData->locale[0]=='j' && i==JISX208) {
                



                filter=UCNV_SET_FILTER_SJIS;
            } else if(i==KSC5601) {
                



                filter=UCNV_SET_FILTER_GR94DBCS;
            } else {
                filter=UCNV_SET_FILTER_NONE;
            }
            ucnv_MBCSGetFilteredUnicodeSetForUnicode(cnvData->myConverterArray[i], sa, which, filter, pErrorCode);
        }
    }

    




    sa->remove(sa->set, 0x0e);
    sa->remove(sa->set, 0x0f);
    sa->remove(sa->set, 0x1b);

    
    sa->removeRange(sa->set, 0x80, 0x9f);
}

static const UConverterImpl _ISO2022Impl={
    UCNV_ISO_2022,

    NULL,
    NULL,

    _ISO2022Open,
    _ISO2022Close,
    _ISO2022Reset,

#ifdef U_ENABLE_GENERIC_ISO_2022
    T_UConverter_toUnicode_ISO_2022_OFFSETS_LOGIC,
    T_UConverter_toUnicode_ISO_2022_OFFSETS_LOGIC,
    ucnv_fromUnicode_UTF8,
    ucnv_fromUnicode_UTF8_OFFSETS_LOGIC,
#else
    NULL,
    NULL,
    NULL,
    NULL,
#endif
    NULL,

    NULL,
    _ISO2022getName,
    _ISO_2022_WriteSub,
    _ISO_2022_SafeClone,
    _ISO_2022_GetUnicodeSet,

    NULL,
    NULL
};
static const UConverterStaticData _ISO2022StaticData={
    sizeof(UConverterStaticData),
    "ISO_2022",
    2022,
    UCNV_IBM,
    UCNV_ISO_2022,
    1,
    3, 
    { 0x1a, 0, 0, 0 },
    1,
    FALSE,
    FALSE,
    0,
    0,
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } 
};
const UConverterSharedData _ISO2022Data={
    sizeof(UConverterSharedData),
    ~((uint32_t) 0),
    NULL,
    NULL,
    &_ISO2022StaticData,
    FALSE,
    &_ISO2022Impl,
    0, UCNV_MBCS_TABLE_INITIALIZER
};


static const UConverterImpl _ISO2022JPImpl={
    UCNV_ISO_2022,

    NULL,
    NULL,

    _ISO2022Open,
    _ISO2022Close,
    _ISO2022Reset,

    UConverter_toUnicode_ISO_2022_JP_OFFSETS_LOGIC,
    UConverter_toUnicode_ISO_2022_JP_OFFSETS_LOGIC,
    UConverter_fromUnicode_ISO_2022_JP_OFFSETS_LOGIC,
    UConverter_fromUnicode_ISO_2022_JP_OFFSETS_LOGIC,
    NULL,

    NULL,
    _ISO2022getName,
    _ISO_2022_WriteSub,
    _ISO_2022_SafeClone,
    _ISO_2022_GetUnicodeSet,

    NULL,
    NULL
};
static const UConverterStaticData _ISO2022JPStaticData={
    sizeof(UConverterStaticData),
    "ISO_2022_JP",
    0,
    UCNV_IBM,
    UCNV_ISO_2022,
    1,
    6, 
    { 0x1a, 0, 0, 0 },
    1,
    FALSE,
    FALSE,
    0,
    0,
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } 
};

namespace {

const UConverterSharedData _ISO2022JPData={
    sizeof(UConverterSharedData),
    ~((uint32_t) 0),
    NULL,
    NULL,
    &_ISO2022JPStaticData,
    FALSE,
    &_ISO2022JPImpl,
    0, UCNV_MBCS_TABLE_INITIALIZER
};

}  


static const UConverterImpl _ISO2022KRImpl={
    UCNV_ISO_2022,

    NULL,
    NULL,

    _ISO2022Open,
    _ISO2022Close,
    _ISO2022Reset,

    UConverter_toUnicode_ISO_2022_KR_OFFSETS_LOGIC,
    UConverter_toUnicode_ISO_2022_KR_OFFSETS_LOGIC,
    UConverter_fromUnicode_ISO_2022_KR_OFFSETS_LOGIC,
    UConverter_fromUnicode_ISO_2022_KR_OFFSETS_LOGIC,
    NULL,

    NULL,
    _ISO2022getName,
    _ISO_2022_WriteSub,
    _ISO_2022_SafeClone,
    _ISO_2022_GetUnicodeSet,

    NULL,
    NULL
};
static const UConverterStaticData _ISO2022KRStaticData={
    sizeof(UConverterStaticData),
    "ISO_2022_KR",
    0,
    UCNV_IBM,
    UCNV_ISO_2022,
    1,
    3, 
    { 0x1a, 0, 0, 0 },
    1,
    FALSE,
    FALSE,
    0,
    0,
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } 
};

namespace {

const UConverterSharedData _ISO2022KRData={
    sizeof(UConverterSharedData),
    ~((uint32_t) 0),
    NULL,
    NULL,
    &_ISO2022KRStaticData,
    FALSE,
    &_ISO2022KRImpl,
    0, UCNV_MBCS_TABLE_INITIALIZER
};

}  


static const UConverterImpl _ISO2022CNImpl={

    UCNV_ISO_2022,

    NULL,
    NULL,

    _ISO2022Open,
    _ISO2022Close,
    _ISO2022Reset,

    UConverter_toUnicode_ISO_2022_CN_OFFSETS_LOGIC,
    UConverter_toUnicode_ISO_2022_CN_OFFSETS_LOGIC,
    UConverter_fromUnicode_ISO_2022_CN_OFFSETS_LOGIC,
    UConverter_fromUnicode_ISO_2022_CN_OFFSETS_LOGIC,
    NULL,

    NULL,
    _ISO2022getName,
    _ISO_2022_WriteSub,
    _ISO_2022_SafeClone,
    _ISO_2022_GetUnicodeSet,

    NULL,
    NULL
};
static const UConverterStaticData _ISO2022CNStaticData={
    sizeof(UConverterStaticData),
    "ISO_2022_CN",
    0,
    UCNV_IBM,
    UCNV_ISO_2022,
    1,
    8, 
    { 0x1a, 0, 0, 0 },
    1,
    FALSE,
    FALSE,
    0,
    0,
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } 
};

namespace {

const UConverterSharedData _ISO2022CNData={
    sizeof(UConverterSharedData),
    ~((uint32_t) 0),
    NULL,
    NULL,
    &_ISO2022CNStaticData,
    FALSE,
    &_ISO2022CNImpl,
    0, UCNV_MBCS_TABLE_INITIALIZER
};

}  

#endif 

















#ifndef __UTEXT_H__
#define __UTEXT_H__























































































































#include "unicode/utypes.h"
#include "unicode/uchar.h"
#if U_SHOW_CPLUSPLUS_API
#include "unicode/localpointer.h"
#include "unicode/rep.h"
#include "unicode/unistr.h"
#include "unicode/chariter.h"
#endif


U_CDECL_BEGIN

struct UText;
typedef struct UText UText; 





























U_STABLE UText * U_EXPORT2
utext_close(UText *ut);

#if U_SHOW_CPLUSPLUS_API

U_NAMESPACE_BEGIN










U_DEFINE_LOCAL_OPEN_POINTER(LocalUTextPointer, UText, utext_close);

U_NAMESPACE_END

#endif






















U_STABLE UText * U_EXPORT2
utext_openUTF8(UText *ut, const char *s, int64_t length, UErrorCode *status);
















U_STABLE UText * U_EXPORT2
utext_openUChars(UText *ut, const UChar *s, int64_t length, UErrorCode *status);


#if U_SHOW_CPLUSPLUS_API












U_STABLE UText * U_EXPORT2
utext_openUnicodeString(UText *ut, icu::UnicodeString *s, UErrorCode *status);














U_STABLE UText * U_EXPORT2
utext_openConstUnicodeString(UText *ut, const icu::UnicodeString *s, UErrorCode *status);














U_STABLE UText * U_EXPORT2
utext_openReplaceable(UText *ut, icu::Replaceable *rep, UErrorCode *status);













U_STABLE UText * U_EXPORT2
utext_openCharacterIterator(UText *ut, icu::CharacterIterator *ci, UErrorCode *status);

#endif



























































U_STABLE UText * U_EXPORT2
utext_clone(UText *dest, const UText *src, UBool deep, UBool readOnly, UErrorCode *status);













U_STABLE UBool U_EXPORT2
utext_equals(const UText *a, const UText *b);



















U_STABLE int64_t U_EXPORT2
utext_nativeLength(UText *ut);














U_STABLE UBool U_EXPORT2
utext_isLengthExpensive(const UText *ut);


























U_STABLE UChar32 U_EXPORT2
utext_char32At(UText *ut, int64_t nativeIndex);












U_STABLE UChar32 U_EXPORT2
utext_current32(UText *ut);




















U_STABLE UChar32 U_EXPORT2
utext_next32(UText *ut);



















U_STABLE UChar32 U_EXPORT2
utext_previous32(UText *ut);




















U_STABLE UChar32 U_EXPORT2
utext_next32From(UText *ut, int64_t nativeIndex);


















U_STABLE UChar32 U_EXPORT2
utext_previous32From(UText *ut, int64_t nativeIndex);













U_STABLE int64_t U_EXPORT2
utext_getNativeIndex(const UText *ut);
























U_STABLE void U_EXPORT2
utext_setNativeIndex(UText *ut, int64_t nativeIndex);

















U_STABLE UBool U_EXPORT2
utext_moveIndex32(UText *ut, int32_t delta);























U_STABLE int64_t U_EXPORT2
utext_getPreviousNativeIndex(UText *ut); 




































U_STABLE int32_t U_EXPORT2
utext_extract(UText *ut,
             int64_t nativeStart, int64_t nativeLimit,
             UChar *dest, int32_t destCapacity,
             UErrorCode *status);


















#ifndef U_HIDE_INTERNAL_API









#define UTEXT_CURRENT32(ut)  \
    ((ut)->chunkOffset < (ut)->chunkLength && ((ut)->chunkContents)[(ut)->chunkOffset]<0xd800 ? \
    ((ut)->chunkContents)[((ut)->chunkOffset)] : utext_current32(ut))
#endif  












#define UTEXT_NEXT32(ut)  \
    ((ut)->chunkOffset < (ut)->chunkLength && ((ut)->chunkContents)[(ut)->chunkOffset]<0xd800 ? \
    ((ut)->chunkContents)[((ut)->chunkOffset)++] : utext_next32(ut))











#define UTEXT_PREVIOUS32(ut)  \
    ((ut)->chunkOffset > 0 && \
     (ut)->chunkContents[(ut)->chunkOffset-1] < 0xd800 ? \
          (ut)->chunkContents[--((ut)->chunkOffset)]  :  utext_previous32(ut))













#define UTEXT_GETNATIVEINDEX(ut)                       \
    ((ut)->chunkOffset <= (ut)->nativeIndexingLimit?   \
        (ut)->chunkNativeStart+(ut)->chunkOffset :     \
        (ut)->pFuncs->mapOffsetToNative(ut))    












#define UTEXT_SETNATIVEINDEX(ut, ix)                       \
    { int64_t __offset = (ix) - (ut)->chunkNativeStart; \
      if (__offset>=0 && __offset<=(int64_t)(ut)->nativeIndexingLimit) { \
          (ut)->chunkOffset=(int32_t)__offset; \
      } else { \
          utext_setNativeIndex((ut), (ix)); } }






























U_STABLE UBool U_EXPORT2
utext_isWritable(const UText *ut);










U_STABLE UBool U_EXPORT2
utext_hasMetaData(const UText *ut);





























U_STABLE int32_t U_EXPORT2
utext_replace(UText *ut,
             int64_t nativeStart, int64_t nativeLimit,
             const UChar *replacementText, int32_t replacementLength,
             UErrorCode *status);



































U_STABLE void U_EXPORT2
utext_copy(UText *ut,
          int64_t nativeStart, int64_t nativeLimit,
          int64_t destIndex,
          UBool move,
          UErrorCode *status);























U_STABLE void U_EXPORT2
utext_freeze(UText *ut);








enum {
    



    UTEXT_PROVIDER_LENGTH_IS_EXPENSIVE = 1,
    





    UTEXT_PROVIDER_STABLE_CHUNKS = 2,
    





    UTEXT_PROVIDER_WRITABLE = 3,
    



 
    UTEXT_PROVIDER_HAS_META_DATA = 4,
    






     UTEXT_PROVIDER_OWNS_TEXT = 5
};






































typedef UText * U_CALLCONV
UTextClone(UText *dest, const UText *src, UBool deep, UErrorCode *status);










typedef int64_t U_CALLCONV
UTextNativeLength(UText *ut);


























typedef UBool U_CALLCONV
UTextAccess(UText *ut, int64_t nativeIndex, UBool forward);




























typedef int32_t U_CALLCONV
UTextExtract(UText *ut,
             int64_t nativeStart, int64_t nativeLimit,
             UChar *dest, int32_t destCapacity,
             UErrorCode *status);






























typedef int32_t U_CALLCONV
UTextReplace(UText *ut,
             int64_t nativeStart, int64_t nativeLimit,
             const UChar *replacementText, int32_t replacmentLength,
             UErrorCode *status);





























typedef void U_CALLCONV
UTextCopy(UText *ut,
          int64_t nativeStart, int64_t nativeLimit,
          int64_t nativeDest,
          UBool move,
          UErrorCode *status);














typedef int64_t U_CALLCONV
UTextMapOffsetToNative(const UText *ut);
















typedef int32_t U_CALLCONV
UTextMapNativeIndexToUTF16(const UText *ut, int64_t nativeIndex);



















typedef void U_CALLCONV
UTextClose(UText *ut);











struct UTextFuncs {
    













    int32_t       tableSize;

    




    int32_t       reserved1,  reserved2,  reserved3;


    





    UTextClone *clone;

    






    UTextNativeLength *nativeLength;

    





    UTextAccess *access;

    





    UTextExtract *extract;

    





    UTextReplace *replace;

    





    UTextCopy *copy;

    





    UTextMapOffsetToNative *mapOffsetToNative;

    





    UTextMapNativeIndexToUTF16 *mapNativeIndexToUTF16;

    





    UTextClose  *close;

    



    UTextClose  *spare1;
    
    



    UTextClose  *spare2;

    



    UTextClose  *spare3;

};




typedef struct UTextFuncs UTextFuncs;

 










struct UText {
    











    uint32_t       magic;


    




    int32_t        flags;


    




    int32_t         providerProperties;

    





    int32_t         sizeOfStruct;
    
    
    

    




    int64_t         chunkNativeLimit;

    



    int32_t        extraSize;

    






    int32_t         nativeIndexingLimit;

    
    
    



    int64_t         chunkNativeStart;

    




    int32_t         chunkOffset;

    



    int32_t         chunkLength;

    
    

    





    const UChar    *chunkContents;

     



    const UTextFuncs     *pFuncs;

    




    void          *pExtra;

    





    const void   *context;

    

    




    const void     *p; 
    




    const void     *q;
     




    const void     *r;

    




    void           *privP;


    
    

    




    int64_t         a;

    




    int32_t         b;

    




    int32_t         c;

    


    




    int64_t         privA;
    




    int32_t         privB;
    




    int32_t         privC;
};


















U_STABLE UText * U_EXPORT2
utext_setup(UText *ut, int32_t extraSpace, UErrorCode *status);

#ifndef U_HIDE_INTERNAL_API





enum {
    UTEXT_MAGIC = 0x345ad82c
};
#endif  








#define UTEXT_INITIALIZER {                                        \
                  UTEXT_MAGIC,          /* magic                */ \
                  0,                    /* flags                */ \
                  0,                    /* providerProps        */ \
                  sizeof(UText),        /* sizeOfStruct         */ \
                  0,                    /* chunkNativeLimit     */ \
                  0,                    /* extraSize            */ \
                  0,                    /* nativeIndexingLimit  */ \
                  0,                    /* chunkNativeStart     */ \
                  0,                    /* chunkOffset          */ \
                  0,                    /* chunkLength          */ \
                  NULL,                 /* chunkContents        */ \
                  NULL,                 /* pFuncs               */ \
                  NULL,                 /* pExtra               */ \
                  NULL,                 /* context              */ \
                  NULL, NULL, NULL,     /* p, q, r              */ \
                  NULL,                 /* privP                */ \
                  0, 0, 0,              /* a, b, c              */ \
                  0, 0, 0               /* privA,B,C,           */ \
                  }


U_CDECL_END



#endif

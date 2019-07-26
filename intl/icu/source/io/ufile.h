

















#ifndef UFILE_H
#define UFILE_H

#include "unicode/utypes.h"
#include "unicode/ucnv.h"
#include "unicode/utrans.h"
#include "locbund.h"


#define UFILE_CHARBUFFER_SIZE 1024


#define UFILE_UCHARBUFFER_SIZE 1024



#if !UCONFIG_NO_TRANSLITERATION

typedef struct {
    UChar  *buffer;             
    int32_t capacity;           
    int32_t pos;                
    int32_t length;             
    UTransliterator *translit;
} UFILETranslitBuffer;

#endif

typedef struct u_localized_string {
    UChar       *fPos;          
    const UChar *fLimit;        
    UChar       *fBuffer;       

#if !UCONFIG_NO_FORMATTING
    ULocaleBundle  fBundle; 
#endif
} u_localized_string;

struct UFILE {
#if !UCONFIG_NO_TRANSLITERATION
    UFILETranslitBuffer *fTranslit;
#endif

    FILE        *fFile;         

    UConverter  *fConverter;    

    u_localized_string str;     

    UChar       fUCBuffer[UFILE_UCHARBUFFER_SIZE];

    UBool       fOwnFile;       

    int32_t     fFileno;        
};




U_CFUNC int32_t U_EXPORT2
u_file_write_flush( const UChar     *chars, 
        int32_t     count, 
        UFILE       *f,
        UBool       flushIO,
        UBool       flushTranslit);





void
ufile_fill_uchar_buffer(UFILE *f);







U_CFUNC UBool U_EXPORT2
ufile_getch(UFILE *f, UChar *ch);







U_CFUNC UBool U_EXPORT2
ufile_getch32(UFILE *f, UChar32 *ch);





void 
ufile_close_translit(UFILE *f);





void 
ufile_flush_translit(UFILE *f);





void 
ufile_flush_io(UFILE *f);


#endif

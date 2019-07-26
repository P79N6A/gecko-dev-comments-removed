


















#include "unicode/ustdio.h"
#include "unicode/putil.h"
#include "cmemory.h"
#include "cstring.h"
#include "ufile.h"
#include "ufmt_cmn.h"
#include "unicode/ucnv.h"
#include "unicode/ustring.h"

#include <string.h>

#define DELIM_LF 0x000A
#define DELIM_VT 0x000B
#define DELIM_FF 0x000C
#define DELIM_CR 0x000D
#define DELIM_NEL 0x0085
#define DELIM_LS 0x2028
#define DELIM_PS 0x2029


#if U_PLATFORM_USES_ONLY_WIN32_API
static const UChar DELIMITERS [] = { DELIM_CR, DELIM_LF, 0x0000 };
static const uint32_t DELIMITERS_LEN = 2;

#else
static const UChar DELIMITERS [] = { DELIM_LF, 0x0000 };
static const uint32_t DELIMITERS_LEN = 1;
#endif

#define IS_FIRST_STRING_DELIMITER(c1) \
 (UBool)((DELIM_LF <= (c1) && (c1) <= DELIM_CR) \
        || (c1) == DELIM_NEL \
        || (c1) == DELIM_LS \
        || (c1) == DELIM_PS)
#define CAN_HAVE_COMBINED_STRING_DELIMITER(c1) (UBool)((c1) == DELIM_CR)
#define IS_COMBINED_STRING_DELIMITER(c1, c2) \
 (UBool)((c1) == DELIM_CR && (c2) == DELIM_LF)


#if !UCONFIG_NO_TRANSLITERATION

U_CAPI UTransliterator* U_EXPORT2
u_fsettransliterator(UFILE *file, UFileDirection direction,
                     UTransliterator *adopt, UErrorCode *status)
{
    UTransliterator *old = NULL;

    if(U_FAILURE(*status))
    {
        return adopt;
    }

    if(!file)
    {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return adopt;
    }

    if(direction & U_READ)
    {
        
        *status = U_UNSUPPORTED_ERROR;
        return adopt;
    }

    if(adopt == NULL) 
    {
        if(file->fTranslit != NULL)
        {
            
            old = file->fTranslit->translit;
            uprv_free(file->fTranslit->buffer);
            file->fTranslit->buffer=NULL;
            uprv_free(file->fTranslit);
            file->fTranslit=NULL;
        }
    }
    else
    {
        if(file->fTranslit == NULL)
        {
            file->fTranslit = (UFILETranslitBuffer*) uprv_malloc(sizeof(UFILETranslitBuffer));
            if(!file->fTranslit)
            {
                *status = U_MEMORY_ALLOCATION_ERROR;
                return adopt;
            }
            file->fTranslit->capacity = 0;
            file->fTranslit->length = 0;
            file->fTranslit->pos = 0;
            file->fTranslit->buffer = NULL;
        }
        else
        {
            old = file->fTranslit->translit;
            ufile_flush_translit(file);
        }

        file->fTranslit->translit = adopt;
    }

    return old;
}

static const UChar * u_file_translit(UFILE *f, const UChar *src, int32_t *count, UBool flush)
{
    int32_t newlen;
    int32_t junkCount = 0;
    int32_t textLength;
    int32_t textLimit;
    UTransPosition pos;
    UErrorCode status = U_ZERO_ERROR;

    if(count == NULL)
    {
        count = &junkCount;
    }

    if ((!f)||(!f->fTranslit)||(!f->fTranslit->translit))
    {
        
        return src;
    }

    
    if(f->fTranslit->length > f->fTranslit->pos)
    {
        memmove(f->fTranslit->buffer, f->fTranslit->buffer + f->fTranslit->pos,
            (f->fTranslit->length - f->fTranslit->pos)*sizeof(UChar));
    }
    f->fTranslit->length -= f->fTranslit->pos; 
    f->fTranslit->pos = 0;

    
    newlen = (*count + f->fTranslit->length) * 4;

    if(newlen > f->fTranslit->capacity)
    {
        if(f->fTranslit->buffer == NULL)
        {
            f->fTranslit->buffer = (UChar*)uprv_malloc(newlen * sizeof(UChar));
        }
        else
        {
            f->fTranslit->buffer = (UChar*)uprv_realloc(f->fTranslit->buffer, newlen * sizeof(UChar));
        }
        
        if (f->fTranslit->buffer == NULL) {
        	return NULL;
        }
        f->fTranslit->capacity = newlen;
    }

    
    u_strncpy(f->fTranslit->buffer + f->fTranslit->length,
        src,
        *count);
    f->fTranslit->length += *count;

    
    if(flush == FALSE)
    {
        textLength = f->fTranslit->length;
        pos.contextStart = 0;
        pos.contextLimit = textLength;
        pos.start        = 0;
        pos.limit        = textLength;

        utrans_transIncrementalUChars(f->fTranslit->translit,
            f->fTranslit->buffer, 
            &textLength,
            f->fTranslit->capacity,
            &pos,
            &status);

        
        
        *count            = pos.start;
        f->fTranslit->pos = pos.start;
        f->fTranslit->length = pos.limit;

        return f->fTranslit->buffer;
    }
    else
    {
        textLength = f->fTranslit->length;
        textLimit = f->fTranslit->length;

        utrans_transUChars(f->fTranslit->translit,
            f->fTranslit->buffer,
            &textLength,
            f->fTranslit->capacity,
            0,
            &textLimit,
            &status);

        
        *count = textLimit;

        
        f->fTranslit->pos = 0;
        f->fTranslit->length = 0;

        return f->fTranslit->buffer;
    }
}

#endif

void
ufile_flush_translit(UFILE *f)
{
#if !UCONFIG_NO_TRANSLITERATION
    if((!f)||(!f->fTranslit))
        return;
#endif

    u_file_write_flush(NULL, 0, f, FALSE, TRUE);
}


void
ufile_flush_io(UFILE *f)
{
  if((!f) || (!f->fFile)) {
    return; 
  }

  u_file_write_flush(NULL, 0, f, TRUE, FALSE);
}


void
ufile_close_translit(UFILE *f)
{
#if !UCONFIG_NO_TRANSLITERATION
    if((!f)||(!f->fTranslit))
        return;
#endif

    ufile_flush_translit(f);

#if !UCONFIG_NO_TRANSLITERATION
    if(f->fTranslit->translit)
        utrans_close(f->fTranslit->translit);

    if(f->fTranslit->buffer)
    {
        uprv_free(f->fTranslit->buffer);
    }

    uprv_free(f->fTranslit);
    f->fTranslit = NULL;
#endif
}




U_CAPI int32_t U_EXPORT2 
u_fputs(const UChar    *s,
        UFILE        *f)
{
    int32_t count = u_file_write(s, u_strlen(s), f);
    count += u_file_write(DELIMITERS, DELIMITERS_LEN, f);
    return count;
}

U_CAPI UChar32 U_EXPORT2 
u_fputc(UChar32      uc,
        UFILE        *f)
{
    UChar buf[2];
    int32_t idx = 0;
    UBool isError = FALSE;

    U16_APPEND(buf, idx, sizeof(buf)/sizeof(*buf), uc, isError);
    if (isError) {
        return U_EOF;
    }
    return u_file_write(buf, idx, f) == idx ? uc : U_EOF;
}


U_CFUNC int32_t U_EXPORT2
u_file_write_flush(const UChar *chars,
                   int32_t     count,
                   UFILE       *f,
                   UBool       flushIO,
                   UBool       flushTranslit)
{
    
    UErrorCode  status       = U_ZERO_ERROR;
    const UChar *mySource    = chars;
    const UChar *mySourceBegin; 
    const UChar *mySourceEnd;
    char        charBuffer[UFILE_CHARBUFFER_SIZE];
    char        *myTarget   = charBuffer;
    int32_t     written      = 0;
    int32_t     numConverted = 0;

    if (count < 0) {
        count = u_strlen(chars);
    }

#if !UCONFIG_NO_TRANSLITERATION
    if((f->fTranslit) && (f->fTranslit->translit))
    {
        
        mySource = u_file_translit(f, chars, &count, flushTranslit);
    }
#endif

    
    if (!f->fFile) {
        int32_t charsLeft = (int32_t)(f->str.fLimit - f->str.fPos);
        if (flushIO && charsLeft > count) {
            count++;
        }
        written = ufmt_min(count, charsLeft);
        u_strncpy(f->str.fPos, mySource, written);
        f->str.fPos += written;
        return written;
    }

    mySourceEnd = mySource + count;

    
    do {
        mySourceBegin = mySource; 
        status     = U_ZERO_ERROR;
        if(f->fConverter != NULL) { 
            ucnv_fromUnicode(f->fConverter,
                &myTarget,
                charBuffer + UFILE_CHARBUFFER_SIZE,
                &mySource,
                mySourceEnd,
                NULL,
                flushIO,
                &status);
        } else { 
            int32_t convertChars = (int32_t) (mySourceEnd - mySource); 
            if (convertChars > UFILE_CHARBUFFER_SIZE) { 
                convertChars = UFILE_CHARBUFFER_SIZE; 
                status = U_BUFFER_OVERFLOW_ERROR; 
            } 
            u_UCharsToChars(mySource, myTarget, convertChars); 
            mySource += convertChars; 
            myTarget += convertChars; 
        }
        numConverted = (int32_t)(myTarget - charBuffer);

        if (numConverted > 0) {
            
            fwrite(charBuffer,
                sizeof(char),
                numConverted,
                f->fFile);

            written     += (int32_t) (mySource - mySourceBegin);
        }
        myTarget     = charBuffer;
    }
    while(status == U_BUFFER_OVERFLOW_ERROR);

    
    return written;
}

U_CAPI int32_t U_EXPORT2 
u_file_write(    const UChar     *chars,
             int32_t        count,
             UFILE         *f)
{
    return u_file_write_flush(chars,count,f,FALSE,FALSE);
}



void
ufile_fill_uchar_buffer(UFILE *f)
{
    UErrorCode  status;
    const char  *mySource;
    const char  *mySourceEnd;
    UChar       *myTarget;
    int32_t     bufferSize;
    int32_t     maxCPBytes;
    int32_t     bytesRead;
    int32_t     availLength;
    int32_t     dataSize;
    char        charBuffer[UFILE_CHARBUFFER_SIZE];
    u_localized_string *str;

    if (f->fFile == NULL) {
        
        return;
    }

    str = &f->str;
    dataSize = (int32_t)(str->fLimit - str->fPos);
    if (f->fFileno == 0 && dataSize > 0) {
        
        return;
    }

    
    if(dataSize != 0) {
        uprv_memmove(f->fUCBuffer, str->fPos, dataSize * sizeof(UChar)); 
    }


    
    availLength = UFILE_UCHARBUFFER_SIZE - dataSize;

    
    
    maxCPBytes = availLength / (f->fConverter!=NULL?(2*ucnv_getMinCharSize(f->fConverter)):1);

    
    if (f->fFileno == 0) {
        
        char *retStr = fgets(charBuffer, ufmt_min(maxCPBytes, UFILE_CHARBUFFER_SIZE), f->fFile);
        bytesRead = (int32_t)(retStr ? uprv_strlen(charBuffer) : 0);
    }
    else {
        
        bytesRead = (int32_t)fread(charBuffer,
            sizeof(char),
            ufmt_min(maxCPBytes, UFILE_CHARBUFFER_SIZE),
            f->fFile);
    }

    
    status      = U_ZERO_ERROR;
    mySource    = charBuffer;
    mySourceEnd = charBuffer + bytesRead;
    myTarget    = f->fUCBuffer + dataSize;
    bufferSize  = UFILE_UCHARBUFFER_SIZE;

    if(f->fConverter != NULL) { 
        
        ucnv_toUnicode(f->fConverter,
            &myTarget,
            f->fUCBuffer + bufferSize,
            &mySource,
            mySourceEnd,
            NULL,
            (UBool)(feof(f->fFile) != 0),
            &status);

    } else { 
        u_charsToUChars(mySource, myTarget, bytesRead);
        myTarget += bytesRead;
    }

    
    str->fPos    = str->fBuffer;
    str->fLimit  = myTarget;
}

U_CAPI UChar* U_EXPORT2 
u_fgets(UChar        *s,
        int32_t       n,
        UFILE        *f)
{
    int32_t dataSize;
    int32_t count;
    UChar *alias;
    const UChar *limit;
    UChar *sItr;
    UChar currDelim = 0;
    u_localized_string *str;

    if (n <= 0) {
        
        return NULL;
    }

    
    str = &f->str;
    if (str->fPos >= str->fLimit) {
        ufile_fill_uchar_buffer(f);
    }

    
    --n;

    
    dataSize = (int32_t)(str->fLimit - str->fPos);

    
    if (dataSize == 0)
        return NULL;

    
    count = 0;
    sItr = s;
    currDelim = 0;
    while (dataSize > 0 && count < n) {
        alias = str->fPos;

        
        if (dataSize < (n - count)) {
            limit = str->fLimit;
        }
        else {
            limit = alias + (n - count);
        }

        if (!currDelim) {
            
            while (alias < limit && !IS_FIRST_STRING_DELIMITER(*alias)) {
                count++;
                *(sItr++) = *(alias++);
            }
            
            if (alias < limit && IS_FIRST_STRING_DELIMITER(*alias)) {
                if (CAN_HAVE_COMBINED_STRING_DELIMITER(*alias)) {
                    currDelim = *alias;
                }
                else {
                    currDelim = 1;  



                }
                count++;
                *(sItr++) = *(alias++);
            }
        }
        
        if (alias < limit) {
            if (currDelim && IS_COMBINED_STRING_DELIMITER(currDelim, *alias)) {
                count++;
                *(sItr++) = *(alias++);
            }
            currDelim = 1;  



        }

        
        str->fPos = alias;

        
        if (currDelim == 1) {
            
            break;
        }

        
        ufile_fill_uchar_buffer(f);

        
        dataSize = (int32_t)(str->fLimit - str->fPos);
    }

    
    *sItr = 0x0000;
    return s;
}

U_CFUNC UBool U_EXPORT2
ufile_getch(UFILE *f, UChar *ch)
{
    UBool isValidChar = FALSE;

    *ch = U_EOF;
    
    if(f->str.fPos < f->str.fLimit){
        *ch = *(f->str.fPos)++;
        isValidChar = TRUE;
    }
    else {
        
        if(f->str.fPos >= f->str.fLimit) {
            ufile_fill_uchar_buffer(f);
        }
        if(f->str.fPos < f->str.fLimit) {
            *ch = *(f->str.fPos)++;
            isValidChar = TRUE;
        }
    }
    return isValidChar;
}

U_CAPI UChar U_EXPORT2 
u_fgetc(UFILE        *f)
{
    UChar ch;
    ufile_getch(f, &ch);
    return ch;
}

U_CFUNC UBool U_EXPORT2
ufile_getch32(UFILE *f, UChar32 *c32)
{
    UBool isValidChar = FALSE;
    u_localized_string *str;

    *c32 = U_EOF;

    
    str = &f->str;
    if (f && str->fPos + 1 >= str->fLimit) {
        ufile_fill_uchar_buffer(f);
    }

    
    if (str->fPos < str->fLimit) {
        *c32 = *(str->fPos)++;
        if (U_IS_LEAD(*c32)) {
            if (str->fPos < str->fLimit) {
                UChar c16 = *(str->fPos)++;
                *c32 = U16_GET_SUPPLEMENTARY(*c32, c16);
                isValidChar = TRUE;
            }
            else {
                *c32 = U_EOF;
            }
        }
        else {
            isValidChar = TRUE;
        }
    }

    return isValidChar;
}

U_CAPI UChar32 U_EXPORT2 
u_fgetcx(UFILE        *f)
{
    UChar32 ch;
    ufile_getch32(f, &ch);
    return ch;
}

U_CAPI UChar32 U_EXPORT2 
u_fungetc(UChar32        ch,
    UFILE        *f)
{
    u_localized_string *str;

    str = &f->str;

    
    if (str->fPos == str->fBuffer
        || (U_IS_LEAD(ch) && (str->fPos - 1) == str->fBuffer))
    {
        ch = U_EOF;
    }
    else {
        
        
        if (U_IS_LEAD(ch)) {
            if (*--(str->fPos) != U16_TRAIL(ch)
                || *--(str->fPos) != U16_LEAD(ch))
            {
                ch = U_EOF;
            }
        }
        else if (*--(str->fPos) != ch) {
            ch = U_EOF;
        }
    }
    return ch;
}

U_CAPI int32_t U_EXPORT2 
u_file_read(    UChar        *chars,
    int32_t        count,
    UFILE         *f)
{
    int32_t dataSize;
    int32_t read = 0;
    u_localized_string *str = &f->str;

    do {

        
        dataSize = (int32_t)(str->fLimit - str->fPos);
        if (dataSize <= 0) {
            
            ufile_fill_uchar_buffer(f);
            dataSize = (int32_t)(str->fLimit - str->fPos);
        }

        
        if (dataSize > (count - read)) {
            dataSize = count - read;
        }

        
        memcpy(chars + read, str->fPos, dataSize * sizeof(UChar));

        
        read += dataSize;

        
        str->fPos += dataSize;
    }
    while (dataSize != 0 && read < count);

    return read;
}

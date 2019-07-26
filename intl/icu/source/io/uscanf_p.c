

















#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "unicode/unum.h"
#include "unicode/udat.h"
#include "unicode/uset.h"
#include "uscanf.h"
#include "ufmt_cmn.h"
#include "ufile.h"
#include "locbund.h"

#include "cmemory.h"
#include "ustr_cnv.h"


#define FLAG_ASTERISK 0x002A
#define FLAG_PAREN 0x0028

#define ISFLAG(s)    (s) == FLAG_ASTERISK || \
            (s) == FLAG_PAREN


#define SPEC_DOLLARSIGN 0x0024


#define DIGIT_ZERO 0x0030
#define DIGIT_ONE 0x0031
#define DIGIT_TWO 0x0032
#define DIGIT_THREE 0x0033
#define DIGIT_FOUR 0x0034
#define DIGIT_FIVE 0x0035
#define DIGIT_SIX 0x0036
#define DIGIT_SEVEN 0x0037
#define DIGIT_EIGHT 0x0038
#define DIGIT_NINE 0x0039

#define ISDIGIT(s)    (s) == DIGIT_ZERO || \
            (s) == DIGIT_ONE || \
            (s) == DIGIT_TWO || \
            (s) == DIGIT_THREE || \
            (s) == DIGIT_FOUR || \
            (s) == DIGIT_FIVE || \
            (s) == DIGIT_SIX || \
            (s) == DIGIT_SEVEN || \
            (s) == DIGIT_EIGHT || \
            (s) == DIGIT_NINE


#define MOD_H 0x0068
#define MOD_LOWERL 0x006C
#define MOD_L 0x004C

#define ISMOD(s)    (s) == MOD_H || \
            (s) == MOD_LOWERL || \
            (s) == MOD_L




typedef struct u_scanf_spec_info {
    int32_t fWidth;         

    UChar   fSpec;          

    UChar   fPadChar;       

    UBool   fSkipArg;       
    UBool   fIsLongDouble;  
    UBool   fIsShort;       
    UBool   fIsLong;        
    UBool   fIsLongLong;    
    UBool   fIsString;      
} u_scanf_spec_info;





typedef struct u_scanf_spec {
    u_scanf_spec_info    fInfo;        
    int32_t        fArgPos;    
} u_scanf_spec;








static int32_t
u_scanf_parse_spec (const UChar     *fmt,
            u_scanf_spec    *spec)
{
    const UChar *s = fmt;
    const UChar *backup;
    u_scanf_spec_info *info = &(spec->fInfo);

    
    spec->fArgPos             = -1;

    info->fWidth        = -1;
    info->fSpec         = 0x0000;
    info->fPadChar      = 0x0020;
    info->fSkipArg      = FALSE;
    info->fIsLongDouble = FALSE;
    info->fIsShort      = FALSE;
    info->fIsLong       = FALSE;
    info->fIsLongLong   = FALSE;
    info->fIsString     = TRUE;


    
    s++;

    
    if(ISDIGIT(*s)) {

        
        backup = s;

        
        if(ISDIGIT(*s)) {
            spec->fArgPos = (int) (*s++ - DIGIT_ZERO);

            while(ISDIGIT(*s)) {
                spec->fArgPos *= 10;
                spec->fArgPos += (int) (*s++ - DIGIT_ZERO);
            }
        }

        
        if(*s != SPEC_DOLLARSIGN) {
            spec->fArgPos = -1;
            s = backup;
        }
        
        else
            s++;
    }

    
    while(ISFLAG(*s)) {
        switch(*s++) {

            
        case FLAG_ASTERISK:
            info->fSkipArg = TRUE;
            break;

            
        case FLAG_PAREN:

            
            info->fPadChar = (UChar)ufmt_digitvalue(*s++);
            info->fPadChar = (UChar)((info->fPadChar * 16) + ufmt_digitvalue(*s++));
            info->fPadChar = (UChar)((info->fPadChar * 16) + ufmt_digitvalue(*s++));
            info->fPadChar = (UChar)((info->fPadChar * 16) + ufmt_digitvalue(*s++));

            
            s++;

            break;
        }
    }

    
    if(ISDIGIT(*s)){
        info->fWidth = (int) (*s++ - DIGIT_ZERO);

        while(ISDIGIT(*s)) {
            info->fWidth *= 10;
            info->fWidth += (int) (*s++ - DIGIT_ZERO);
        }
    }

    
    if(ISMOD(*s)) {
        switch(*s++) {

            
        case MOD_H:
            info->fIsShort = TRUE;
            break;

            
        case MOD_LOWERL:
            if(*s == MOD_LOWERL) {
                info->fIsLongLong = TRUE;
                
                s++;
            }
            else
                info->fIsLong = TRUE;
            break;

            
        case MOD_L:
            info->fIsLongDouble = TRUE;
            break;
        }
    }

    
    info->fSpec = *s++;

    
    return (int32_t)(s - fmt);
}

#define UP_PERCENT 0x0025






#define UFMT_SIMPLE_PERCENT {ufmt_simple_percent, u_scanf_simple_percent_handler}

#define UFMT_STRING         {ufmt_string, u_scanf_string_handler}

#define UFMT_CHAR           {ufmt_string, u_scanf_char_handler}

#define UFMT_INT            {ufmt_int, u_scanf_integer_handler}

#define UFMT_UINT           {ufmt_int, u_scanf_uinteger_handler}

#define UFMT_OCTAL          {ufmt_int, u_scanf_octal_handler}

#define UFMT_HEX            {ufmt_int, u_scanf_hex_handler}

#define UFMT_DOUBLE         {ufmt_double, u_scanf_double_handler}

#define UFMT_SCIENTIFIC     {ufmt_double, u_scanf_scientific_handler}

#define UFMT_SCIDBL         {ufmt_double, u_scanf_scidbl_handler}

#define UFMT_COUNT          {ufmt_count, u_scanf_count_handler}

#define UFMT_SCANSET        {ufmt_string, u_scanf_scanset_handler}





#define UFMT_POINTER        {ufmt_pointer, u_scanf_pointer_handler}

#define UFMT_SPELLOUT       {ufmt_double, u_scanf_spellout_handler}

#define UFMT_PERCENT        {ufmt_double, u_scanf_percent_handler}

#define UFMT_UCHAR          {ufmt_uchar, u_scanf_uchar_handler}

#define UFMT_USTRING        {ufmt_ustring, u_scanf_ustring_handler}


#define UFMT_EMPTY {ufmt_empty, NULL}

















typedef int32_t (*u_scanf_handler) (UFILE   *stream,
                   u_scanf_spec_info  *info,
                   ufmt_args                *args,
                   const UChar              *fmt,
                   int32_t                  *fmtConsumed,
                   int32_t                  *argConverted);

typedef struct u_scanf_info {
    ufmt_type_info info;
    u_scanf_handler handler;
} u_scanf_info;

#define USCANF_NUM_FMT_HANDLERS 108
#define USCANF_SYMBOL_BUFFER_SIZE 8


#define USCANF_BASE_FMT_HANDLERS 0x20


static int32_t
u_scanf_skip_leading_ws(UFILE   *input,
                        UChar   pad)
{
    UChar   c;
    int32_t count = 0;
    UBool isNotEOF;

    
    while( (isNotEOF = ufile_getch(input, &c)) && (c == pad || u_isWhitespace(c)) )
    {
        count++;
    }

    
    if(isNotEOF)
        u_fungetc(c, input);

    return count;
}


static int32_t
u_scanf_skip_leading_positive_sign(UFILE   *input,
                                   UNumberFormat *format,
                                   UErrorCode *status)
{
    UChar   c;
    int32_t count = 0;
    UBool isNotEOF;
    UChar plusSymbol[USCANF_SYMBOL_BUFFER_SIZE];
    int32_t symbolLen;
    UErrorCode localStatus = U_ZERO_ERROR;

    if (U_SUCCESS(*status)) {
        symbolLen = unum_getSymbol(format,
            UNUM_PLUS_SIGN_SYMBOL,
            plusSymbol,
            sizeof(plusSymbol)/sizeof(*plusSymbol),
            &localStatus);

        if (U_SUCCESS(localStatus)) {
            
            while( (isNotEOF = ufile_getch(input, &c)) && (count < symbolLen && c == plusSymbol[count]) )
            {
                count++;
            }

            
            if(isNotEOF) {
                u_fungetc(c, input);
            }
        }
    }

    return count;
}

static int32_t 
u_scanf_simple_percent_handler(UFILE        *input,
                               u_scanf_spec_info *info,
                               ufmt_args    *args,
                               const UChar  *fmt,
                               int32_t      *fmtConsumed,
                               int32_t      *argConverted)
{
    
    *argConverted = 0;
    if(u_fgetc(input) != 0x0025) {
        *argConverted = -1;
    }
    return 1;
}

static int32_t
u_scanf_count_handler(UFILE         *input,
                      u_scanf_spec_info *info,
                      ufmt_args     *args,
                      const UChar   *fmt,
                      int32_t       *fmtConsumed,
                      int32_t       *argConverted)
{
    
    
    if (!info->fSkipArg) {
        if (info->fIsShort)
            *(int16_t*)(args[0].ptrValue) = (int16_t)(UINT16_MAX & info->fWidth);
        else if (info->fIsLongLong)
            *(int64_t*)(args[0].ptrValue) = info->fWidth;
        else
            *(int32_t*)(args[0].ptrValue) = (int32_t)(UINT32_MAX & info->fWidth);
    }
    *argConverted = 0;

    
    return 0;
}

static int32_t
u_scanf_double_handler(UFILE        *input,
                       u_scanf_spec_info *info,
                       ufmt_args    *args,
                       const UChar  *fmt,
                       int32_t      *fmtConsumed,
                       int32_t      *argConverted)
{
    int32_t         len;
    double          num;
    UNumberFormat   *format;
    int32_t         parsePos    = 0;
    int32_t         skipped;
    UErrorCode      status      = U_ZERO_ERROR;


    
    skipped = u_scanf_skip_leading_ws(input, info->fPadChar);

    
    ufile_fill_uchar_buffer(input);

    
    len = (int32_t)(input->str.fLimit - input->str.fPos);

    
    if(info->fWidth != -1)
        len = ufmt_min(len, info->fWidth);

    
    format = u_locbund_getNumberFormat(&input->str.fBundle, UNUM_DECIMAL);

    
    if(format == 0)
        return 0;

    
    skipped += u_scanf_skip_leading_positive_sign(input, format, &status);

    
    num = unum_parseDouble(format, input->str.fPos, len, &parsePos, &status);

    if (!info->fSkipArg) {
        if (info->fIsLong)
            *(double*)(args[0].ptrValue) = num;
        else if (info->fIsLongDouble)
            *(long double*)(args[0].ptrValue) = num;
        else
            *(float*)(args[0].ptrValue) = (float)num;
    }

    
    


    
    input->str.fPos += parsePos;

    
    *argConverted = !info->fSkipArg;
    return parsePos + skipped;
}

#define UPRINTF_SYMBOL_BUFFER_SIZE 8

static int32_t
u_scanf_scientific_handler(UFILE        *input,
                           u_scanf_spec_info *info,
                           ufmt_args    *args,
                           const UChar  *fmt,
                           int32_t      *fmtConsumed,
                           int32_t      *argConverted)
{
    int32_t         len;
    double          num;
    UNumberFormat   *format;
    int32_t         parsePos    = 0;
    int32_t         skipped;
    UErrorCode      status      = U_ZERO_ERROR;
    UChar srcExpBuf[UPRINTF_SYMBOL_BUFFER_SIZE];
    int32_t srcLen, expLen;
    UChar expBuf[UPRINTF_SYMBOL_BUFFER_SIZE];


    
    skipped = u_scanf_skip_leading_ws(input, info->fPadChar);

    
    ufile_fill_uchar_buffer(input);

    
    len = (int32_t)(input->str.fLimit - input->str.fPos);

    
    if(info->fWidth != -1)
        len = ufmt_min(len, info->fWidth);

    
    format = u_locbund_getNumberFormat(&input->str.fBundle, UNUM_SCIENTIFIC);

    
    if(format == 0)
        return 0;

    

    srcLen = unum_getSymbol(format,
        UNUM_EXPONENTIAL_SYMBOL,
        srcExpBuf,
        sizeof(srcExpBuf),
        &status);

    
    if (info->fSpec == (UChar)0x65 ) {
        expLen = u_strToLower(expBuf, (int32_t)sizeof(expBuf),
            srcExpBuf, srcLen,
            input->str.fBundle.fLocale,
            &status);
    }
    else {
        expLen = u_strToUpper(expBuf, (int32_t)sizeof(expBuf),
            srcExpBuf, srcLen,
            input->str.fBundle.fLocale,
            &status);
    }

    unum_setSymbol(format,
        UNUM_EXPONENTIAL_SYMBOL,
        expBuf,
        expLen,
        &status);




    
    skipped += u_scanf_skip_leading_positive_sign(input, format, &status);

    
    num = unum_parseDouble(format, input->str.fPos, len, &parsePos, &status);

    if (!info->fSkipArg) {
        if (info->fIsLong)
            *(double*)(args[0].ptrValue) = num;
        else if (info->fIsLongDouble)
            *(long double*)(args[0].ptrValue) = num;
        else
            *(float*)(args[0].ptrValue) = (float)num;
    }

    
    


    
    input->str.fPos += parsePos;

    
    *argConverted = !info->fSkipArg;
    return parsePos + skipped;
}

static int32_t
u_scanf_scidbl_handler(UFILE        *input,
                       u_scanf_spec_info *info,
                       ufmt_args    *args,
                       const UChar  *fmt,
                       int32_t      *fmtConsumed,
                       int32_t      *argConverted)
{
    int32_t       len;
    double        num;
    UNumberFormat *scientificFormat, *genericFormat;
    
    double        scientificResult, genericResult;
    int32_t       scientificParsePos = 0, genericParsePos = 0, parsePos = 0;
    int32_t       skipped;
    UErrorCode    scientificStatus = U_ZERO_ERROR;
    UErrorCode    genericStatus = U_ZERO_ERROR;


    
    
    
    


    
    skipped = u_scanf_skip_leading_ws(input, info->fPadChar);

    
    ufile_fill_uchar_buffer(input);

    
    len = (int32_t)(input->str.fLimit - input->str.fPos);

    
    if(info->fWidth != -1)
        len = ufmt_min(len, info->fWidth);

    
    scientificFormat = u_locbund_getNumberFormat(&input->str.fBundle, UNUM_SCIENTIFIC);
    genericFormat = u_locbund_getNumberFormat(&input->str.fBundle, UNUM_DECIMAL);

    
    if(scientificFormat == 0 || genericFormat == 0)
        return 0;

    
    skipped += u_scanf_skip_leading_positive_sign(input, genericFormat, &genericStatus);

    

    scientificResult = unum_parseDouble(scientificFormat, input->str.fPos, len,
        &scientificParsePos, &scientificStatus);

    genericResult = unum_parseDouble(genericFormat, input->str.fPos, len,
        &genericParsePos, &genericStatus);

    
    if(scientificParsePos > genericParsePos) {
        
        num = scientificResult;
        
        parsePos += scientificParsePos;
    }
    else {
        
        num = genericResult;
        
        parsePos += genericParsePos;
    }
    input->str.fPos += parsePos;

    if (!info->fSkipArg) {
        if (info->fIsLong)
            *(double*)(args[0].ptrValue) = num;
        else if (info->fIsLongDouble)
            *(long double*)(args[0].ptrValue) = num;
        else
            *(float*)(args[0].ptrValue) = (float)num;
    }

    
    


    
    *argConverted = !info->fSkipArg;
    return parsePos + skipped;
}

static int32_t
u_scanf_integer_handler(UFILE       *input,
                        u_scanf_spec_info *info,
                        ufmt_args   *args,
                        const UChar *fmt,
                        int32_t     *fmtConsumed,
                        int32_t     *argConverted)
{
    int32_t         len;
    void            *num        = (void*) (args[0].ptrValue);
    UNumberFormat   *format;
    int32_t         parsePos    = 0;
    int32_t         skipped;
    UErrorCode      status      = U_ZERO_ERROR;
    int64_t         result;


    
    skipped = u_scanf_skip_leading_ws(input, info->fPadChar);

    
    ufile_fill_uchar_buffer(input);

    
    len = (int32_t)(input->str.fLimit - input->str.fPos);

    
    if(info->fWidth != -1)
        len = ufmt_min(len, info->fWidth);

    
    format = u_locbund_getNumberFormat(&input->str.fBundle, UNUM_DECIMAL);

    
    if(format == 0)
        return 0;

    
    skipped += u_scanf_skip_leading_positive_sign(input, format, &status);

    
    result = unum_parseInt64(format, input->str.fPos, len, &parsePos, &status);

    
    if (!info->fSkipArg) {
        if (info->fIsShort)
            *(int16_t*)num = (int16_t)(UINT16_MAX & result);
        else if (info->fIsLongLong)
            *(int64_t*)num = result;
        else
            *(int32_t*)num = (int32_t)(UINT32_MAX & result);
    }

    
    input->str.fPos += parsePos;

    
    *argConverted = !info->fSkipArg;
    return parsePos + skipped;
}

static int32_t
u_scanf_uinteger_handler(UFILE          *input,
                         u_scanf_spec_info *info,
                         ufmt_args      *args,
                         const UChar    *fmt,
                         int32_t        *fmtConsumed,
                         int32_t        *argConverted)
{
    
    return u_scanf_integer_handler(input, info, args, fmt, fmtConsumed, argConverted);
}

static int32_t
u_scanf_percent_handler(UFILE       *input,
                        u_scanf_spec_info *info,
                        ufmt_args   *args,
                        const UChar *fmt,
                        int32_t     *fmtConsumed,
                        int32_t     *argConverted)
{
    int32_t         len;
    double          num;
    UNumberFormat   *format;
    int32_t         parsePos    = 0;
    UErrorCode      status      = U_ZERO_ERROR;


    
    u_scanf_skip_leading_ws(input, info->fPadChar);

    
    ufile_fill_uchar_buffer(input);

    
    len = (int32_t)(input->str.fLimit - input->str.fPos);

    
    if(info->fWidth != -1)
        len = ufmt_min(len, info->fWidth);

    
    format = u_locbund_getNumberFormat(&input->str.fBundle, UNUM_PERCENT);

    
    if(format == 0)
        return 0;

    
    u_scanf_skip_leading_positive_sign(input, format, &status);

    
    num = unum_parseDouble(format, input->str.fPos, len, &parsePos, &status);

    if (!info->fSkipArg) {
        *(double*)(args[0].ptrValue) = num;
    }

    
    


    
    input->str.fPos += parsePos;

    
    *argConverted = !info->fSkipArg;
    return parsePos;
}

static int32_t
u_scanf_string_handler(UFILE        *input,
                       u_scanf_spec_info *info,
                       ufmt_args    *args,
                       const UChar  *fmt,
                       int32_t      *fmtConsumed,
                       int32_t      *argConverted)
{
    const UChar *source;
    UConverter  *conv;
    char        *arg    = (char*)(args[0].ptrValue);
    char        *alias  = arg;
    char        *limit;
    UErrorCode  status  = U_ZERO_ERROR;
    int32_t     count;
    int32_t     skipped = 0;
    UChar       c;
    UBool       isNotEOF = FALSE;

    
    if (info->fIsString) {
        skipped = u_scanf_skip_leading_ws(input, info->fPadChar);
    }

    
    count = 0;

    
    conv = u_getDefaultConverter(&status);

    if(U_FAILURE(status))
        return -1;

    while( (info->fWidth == -1 || count < info->fWidth) 
        && (isNotEOF = ufile_getch(input, &c))
        && (!info->fIsString || (c != info->fPadChar && !u_isWhitespace(c))))
    {

        if (!info->fSkipArg) {
            
            source = &c;
            
            if (info->fWidth > 0) {
                limit = alias + info->fWidth - count;
            }
            else {
                limit = alias + ucnv_getMaxCharSize(conv);
            }

            
            ucnv_fromUnicode(conv, &alias, limit, &source, source + 1,
                NULL, TRUE, &status);

            if(U_FAILURE(status)) {
                
                u_releaseDefaultConverter(conv);
                return -1;
            }
        }

        
        ++count;
    }

    
    if (!info->fSkipArg) {
        if ((info->fWidth == -1 || count < info->fWidth) && isNotEOF)
            u_fungetc(c, input);

        
        if (info->fIsString) {
            *alias = 0x00;
        }
    }

    
    u_releaseDefaultConverter(conv);

    
    *argConverted = !info->fSkipArg;
    return count + skipped;
}

static int32_t
u_scanf_char_handler(UFILE          *input,
                     u_scanf_spec_info *info,
                     ufmt_args      *args,
                     const UChar    *fmt,
                     int32_t        *fmtConsumed,
                     int32_t        *argConverted)
{
    if (info->fWidth < 0) {
        info->fWidth = 1;
    }
    info->fIsString = FALSE;
    return u_scanf_string_handler(input, info, args, fmt, fmtConsumed, argConverted);
}

static int32_t
u_scanf_ustring_handler(UFILE       *input,
                        u_scanf_spec_info *info,
                        ufmt_args   *args,
                        const UChar *fmt,
                        int32_t     *fmtConsumed,
                        int32_t     *argConverted)
{
    UChar   *arg     = (UChar*)(args[0].ptrValue);
    UChar   *alias     = arg;
    int32_t count;
    int32_t skipped = 0;
    UChar   c;
    UBool   isNotEOF = FALSE;

    
    if (info->fIsString) {
        skipped = u_scanf_skip_leading_ws(input, info->fPadChar);
    }

    
    count = 0;

    while( (info->fWidth == -1 || count < info->fWidth)
        && (isNotEOF = ufile_getch(input, &c))
        && (!info->fIsString || (c != info->fPadChar && !u_isWhitespace(c))))
    {

        
        if (!info->fSkipArg) {
            *alias++ = c;
        }

        
        ++count;
    }

    
    if (!info->fSkipArg) {
        if((info->fWidth == -1 || count < info->fWidth) && isNotEOF) {
            u_fungetc(c, input);
        }

        
        if (info->fIsString) {
            *alias = 0x0000;
        }
    }

    
    *argConverted = !info->fSkipArg;
    return count + skipped;
}

static int32_t
u_scanf_uchar_handler(UFILE         *input,
                      u_scanf_spec_info *info,
                      ufmt_args     *args,
                      const UChar   *fmt,
                      int32_t       *fmtConsumed,
                      int32_t       *argConverted)
{
    if (info->fWidth < 0) {
        info->fWidth = 1;
    }
    info->fIsString = FALSE;
    return u_scanf_ustring_handler(input, info, args, fmt, fmtConsumed, argConverted);
}

static int32_t
u_scanf_spellout_handler(UFILE          *input,
                         u_scanf_spec_info *info,
                         ufmt_args      *args,
                         const UChar    *fmt,
                         int32_t        *fmtConsumed,
                         int32_t        *argConverted)
{
    int32_t         len;
    double          num;
    UNumberFormat   *format;
    int32_t         parsePos    = 0;
    int32_t         skipped;
    UErrorCode      status      = U_ZERO_ERROR;


    
    skipped = u_scanf_skip_leading_ws(input, info->fPadChar);

    
    ufile_fill_uchar_buffer(input);

    
    len = (int32_t)(input->str.fLimit - input->str.fPos);

    
    if(info->fWidth != -1)
        len = ufmt_min(len, info->fWidth);

    
    format = u_locbund_getNumberFormat(&input->str.fBundle, UNUM_SPELLOUT);

    
    if(format == 0)
        return 0;

    
    
    

    
    num = unum_parseDouble(format, input->str.fPos, len, &parsePos, &status);

    if (!info->fSkipArg) {
        *(double*)(args[0].ptrValue) = num;
    }

    
    


    
    input->str.fPos += parsePos;

    
    *argConverted = !info->fSkipArg;
    return parsePos + skipped;
}

static int32_t
u_scanf_hex_handler(UFILE       *input,
                    u_scanf_spec_info *info,
                    ufmt_args   *args,
                    const UChar *fmt,
                    int32_t     *fmtConsumed,
                    int32_t     *argConverted)
{
    int32_t     len;
    int32_t     skipped;
    void        *num    = (void*) (args[0].ptrValue);
    int64_t     result;

    
    skipped = u_scanf_skip_leading_ws(input, info->fPadChar);

    
    ufile_fill_uchar_buffer(input);

    
    len = (int32_t)(input->str.fLimit - input->str.fPos);

    
    if(info->fWidth != -1)
        len = ufmt_min(len, info->fWidth);

    
    if( *(input->str.fPos) == 0x0030 &&
        (*(input->str.fPos + 1) == 0x0078 || *(input->str.fPos + 1) == 0x0058) ) {

        
        input->str.fPos += 2;
        len -= 2;
    }

    
    result = ufmt_uto64(input->str.fPos, &len, 16);

    
    input->str.fPos += len;

    
    if (!info->fSkipArg) {
        if (info->fIsShort)
            *(int16_t*)num = (int16_t)(UINT16_MAX & result);
        else if (info->fIsLongLong)
            *(int64_t*)num = result;
        else
            *(int32_t*)num = (int32_t)(UINT32_MAX & result);
    }

    
    *argConverted = !info->fSkipArg;
    return len + skipped;
}

static int32_t
u_scanf_octal_handler(UFILE         *input,
                      u_scanf_spec_info *info,
                      ufmt_args     *args,
                      const UChar   *fmt,
                      int32_t       *fmtConsumed,
                      int32_t       *argConverted)
{
    int32_t     len;
    int32_t     skipped;
    void        *num         = (void*) (args[0].ptrValue);
    int64_t     result;

    
    skipped = u_scanf_skip_leading_ws(input, info->fPadChar);

    
    ufile_fill_uchar_buffer(input);

    
    len = (int32_t)(input->str.fLimit - input->str.fPos);

    
    if(info->fWidth != -1)
        len = ufmt_min(len, info->fWidth);

    
    result = ufmt_uto64(input->str.fPos, &len, 8);

    
    input->str.fPos += len;

    
    if (!info->fSkipArg) {
        if (info->fIsShort)
            *(int16_t*)num = (int16_t)(UINT16_MAX & result);
        else if (info->fIsLongLong)
            *(int64_t*)num = result;
        else
            *(int32_t*)num = (int32_t)(UINT32_MAX & result);
    }

    
    *argConverted = !info->fSkipArg;
    return len + skipped;
}

static int32_t
u_scanf_pointer_handler(UFILE       *input,
                        u_scanf_spec_info *info,
                        ufmt_args   *args,
                        const UChar *fmt,
                        int32_t     *fmtConsumed,
                        int32_t     *argConverted)
{
    int32_t len;
    int32_t skipped;
    void    *result;
    void    **p     = (void**)(args[0].ptrValue);


    
    skipped = u_scanf_skip_leading_ws(input, info->fPadChar);

    
    ufile_fill_uchar_buffer(input);

    
    len = (int32_t)(input->str.fLimit - input->str.fPos);

    
    if(info->fWidth != -1) {
        len = ufmt_min(len, info->fWidth);
    }

    
    if (len > (int32_t)(sizeof(void*)*2)) {
        len = (int32_t)(sizeof(void*)*2);
    }

    
    result = ufmt_utop(input->str.fPos, &len);

    if (!info->fSkipArg) {
        *p = result;
    }

    
    input->str.fPos += len;

    
    *argConverted = !info->fSkipArg;
    return len + skipped;
}

static int32_t
u_scanf_scanset_handler(UFILE       *input,
                        u_scanf_spec_info *info,
                        ufmt_args   *args,
                        const UChar *fmt,
                        int32_t     *fmtConsumed,
                        int32_t     *argConverted)
{
    USet        *scanset;
    UErrorCode  status = U_ZERO_ERROR;
    int32_t     chLeft = INT32_MAX;
    UChar32     c;
    UChar       *alias = (UChar*) (args[0].ptrValue);
    UBool       isNotEOF = FALSE;
    UBool       readCharacter = FALSE;

    
    scanset = uset_open(0, -1);

    
    fmt--;

    
    if(info->fWidth >= 0) {
        chLeft = info->fWidth;
    }

    
    *fmtConsumed = uset_applyPattern(scanset, fmt, -1, 0, &status);

    
    if (U_SUCCESS(status)) {
        c=0;

        
        while(chLeft > 0) {
            if ((isNotEOF = ufile_getch32(input, &c)) && uset_contains(scanset, c)) {
                readCharacter = TRUE;
                if (!info->fSkipArg) {
                    int32_t idx = 0;
                    UBool isError = FALSE;

                    U16_APPEND(alias, idx, chLeft, c, isError);
                    if (isError) {
                        break;
                    }
                    alias += idx;
                }
                chLeft -= (1 + U_IS_SUPPLEMENTARY(c));
            }
            else {
                
                break;
            }
        }

        
        if(isNotEOF && chLeft > 0) {
            u_fungetc(c, input);
        }
    }

    uset_close(scanset);

    
    if(!readCharacter)
        return -1;
    
    else if (!info->fSkipArg) {
        *alias = 0x00;
    }

    
    *argConverted = !info->fSkipArg;
    return (info->fWidth >= 0 ? info->fWidth : INT32_MAX) - chLeft;
}





static const u_scanf_info g_u_scanf_infos[USCANF_NUM_FMT_HANDLERS] = {

    UFMT_EMPTY,         UFMT_EMPTY,         UFMT_EMPTY,         UFMT_EMPTY,
    UFMT_EMPTY,         UFMT_SIMPLE_PERCENT,UFMT_EMPTY,         UFMT_EMPTY,
    UFMT_EMPTY,         UFMT_EMPTY,         UFMT_EMPTY,         UFMT_EMPTY,
    UFMT_EMPTY,         UFMT_EMPTY,         UFMT_EMPTY,         UFMT_EMPTY,


    UFMT_EMPTY,         UFMT_EMPTY,         UFMT_EMPTY,         UFMT_EMPTY,
    UFMT_EMPTY,         UFMT_EMPTY,         UFMT_EMPTY,         UFMT_EMPTY,
    UFMT_EMPTY,         UFMT_EMPTY,         UFMT_EMPTY,         UFMT_EMPTY,
    UFMT_EMPTY,         UFMT_EMPTY,         UFMT_EMPTY,         UFMT_EMPTY,


    UFMT_EMPTY,         UFMT_EMPTY,         UFMT_EMPTY,         UFMT_UCHAR,
    UFMT_EMPTY,         UFMT_SCIENTIFIC,    UFMT_EMPTY,         UFMT_SCIDBL,
#ifdef U_USE_OBSOLETE_IO_FORMATTING
    UFMT_EMPTY,         UFMT_EMPTY,         UFMT_EMPTY,         UFMT_UCHAR,
#else
    UFMT_EMPTY,         UFMT_EMPTY,         UFMT_EMPTY,         UFMT_EMPTY,
#endif
    UFMT_EMPTY,         UFMT_EMPTY,         UFMT_EMPTY,         UFMT_EMPTY,


    UFMT_PERCENT,       UFMT_EMPTY,         UFMT_EMPTY,         UFMT_USTRING,
#ifdef U_USE_OBSOLETE_IO_FORMATTING
    UFMT_EMPTY,         UFMT_USTRING,UFMT_SPELLOUT,      UFMT_EMPTY,
#else
    UFMT_EMPTY,         UFMT_EMPTY,         UFMT_SPELLOUT,      UFMT_EMPTY,
#endif
    UFMT_HEX,           UFMT_EMPTY,         UFMT_EMPTY,         UFMT_SCANSET,
    UFMT_EMPTY,         UFMT_EMPTY,         UFMT_EMPTY,         UFMT_EMPTY,


    UFMT_EMPTY,         UFMT_EMPTY,         UFMT_EMPTY,         UFMT_CHAR,
    UFMT_INT,           UFMT_SCIENTIFIC,    UFMT_DOUBLE,        UFMT_SCIDBL,
    UFMT_EMPTY,         UFMT_INT,           UFMT_EMPTY,         UFMT_EMPTY,
    UFMT_EMPTY,         UFMT_EMPTY,         UFMT_COUNT,         UFMT_OCTAL,


    UFMT_POINTER,       UFMT_EMPTY,         UFMT_EMPTY,         UFMT_STRING,
    UFMT_EMPTY,         UFMT_UINT,          UFMT_EMPTY,         UFMT_EMPTY,
    UFMT_HEX,           UFMT_EMPTY,         UFMT_EMPTY,         UFMT_EMPTY,
    UFMT_EMPTY,         UFMT_EMPTY,         UFMT_EMPTY,         UFMT_EMPTY,
};

U_CFUNC int32_t
u_scanf_parse(UFILE     *f,
            const UChar *patternSpecification,
            va_list     ap)
{
    const UChar     *alias;
    int32_t         count, converted, argConsumed, cpConsumed;
    uint16_t        handlerNum;

    ufmt_args       args;
    u_scanf_spec    spec;
    ufmt_type_info  info;
    u_scanf_handler handler;

    
    alias = patternSpecification;

    
    argConsumed = 0;
    converted = 0;
    cpConsumed = 0;

    
    for(;;) {

        
        while(*alias != UP_PERCENT && *alias != 0x0000 && u_fgetc(f) == *alias) {
            alias++;
        }

        
        if(*alias != UP_PERCENT || *alias == 0x0000)
            break;

        
        count = u_scanf_parse_spec(alias, &spec);

        
        alias += count;

        handlerNum = (uint16_t)(spec.fInfo.fSpec - USCANF_BASE_FMT_HANDLERS);
        if (handlerNum < USCANF_NUM_FMT_HANDLERS) {
            
            
            info = g_u_scanf_infos[ handlerNum ].info;
            if (info != ufmt_count && u_feof(f)) {
                break;
            }
            else if(spec.fInfo.fSkipArg) {
                args.ptrValue = NULL;
            }
            else {
                switch(info) {
                case ufmt_count:
                    
                    spec.fInfo.fWidth = cpConsumed;
                    
                case ufmt_char:
                case ufmt_uchar:
                case ufmt_int:
                case ufmt_string:
                case ufmt_ustring:
                case ufmt_pointer:
                case ufmt_float:
                case ufmt_double:
                    args.ptrValue = va_arg(ap, void*);
                    break;

                default:
                    
                    args.ptrValue = NULL;
                    break;
                }
            }

            
            handler = g_u_scanf_infos[ handlerNum ].handler;
            if(handler != 0) {

                
                count = 1;

                cpConsumed += (*handler)(f, &spec.fInfo, &args, alias, &count, &argConsumed);

                
                if(argConsumed < 0) {
                    converted = -1;
                    break;
                }

                
                converted += argConsumed;

                
                alias += count-1;
            }
            
        }
        

        
    }

    
    return converted;
}

#endif 

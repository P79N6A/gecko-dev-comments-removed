


















#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/ustring.h"
#include "unicode/utf16.h"
#include "uprintf.h"
#include "ufmt_cmn.h"
#include "cmemory.h"
#include "putilimp.h"





#define UFMT_SIMPLE_PERCENT {ufmt_simple_percent, u_printf_simple_percent_handler}

#define UFMT_STRING         {ufmt_string, u_printf_string_handler}

#define UFMT_CHAR           {ufmt_char, u_printf_char_handler}

#define UFMT_INT            {ufmt_int, u_printf_integer_handler}

#define UFMT_UINT           {ufmt_int, u_printf_uinteger_handler}

#define UFMT_OCTAL          {ufmt_int, u_printf_octal_handler}

#define UFMT_HEX            {ufmt_int, u_printf_hex_handler}

#define UFMT_DOUBLE         {ufmt_double, u_printf_double_handler}

#define UFMT_SCIENTIFIC     {ufmt_double, u_printf_scientific_handler}

#define UFMT_SCIDBL         {ufmt_double, u_printf_scidbl_handler}

#define UFMT_COUNT          {ufmt_count, u_printf_count_handler}





#define UFMT_POINTER        {ufmt_pointer, u_printf_pointer_handler}

#define UFMT_SPELLOUT       {ufmt_double, u_printf_spellout_handler}

#define UFMT_PERCENT        {ufmt_double, u_printf_percent_handler}

#define UFMT_UCHAR          {ufmt_uchar, u_printf_uchar_handler}

#define UFMT_USTRING        {ufmt_ustring, u_printf_ustring_handler}


#define UFMT_EMPTY {ufmt_empty, NULL}











typedef int32_t U_EXPORT2
u_printf_handler(const u_printf_stream_handler  *handler,

                 void                           *context,
                 ULocaleBundle                  *formatBundle,
                 const u_printf_spec_info       *info,
                 const ufmt_args                *args);

typedef struct u_printf_info {
    ufmt_type_info info;
    u_printf_handler *handler;
} u_printf_info;




typedef struct u_printf_spec {
  u_printf_spec_info    fInfo;        
  int32_t        fWidthPos;     
  int32_t        fPrecisionPos;    
  int32_t        fArgPos;    
} u_printf_spec;

#define UPRINTF_NUM_FMT_HANDLERS 108


#define UPRINTF_BASE_FMT_HANDLERS 0x20


#define UPRINTF_BUFFER_SIZE 1024
#define UPRINTF_SYMBOL_BUFFER_SIZE 8

static const UChar gNullStr[] = {0x28, 0x6E, 0x75, 0x6C, 0x6C, 0x29, 0}; 
static const UChar gSpaceStr[] = {0x20, 0}; 



static void
u_printf_set_sign(UNumberFormat        *format,
                   const u_printf_spec_info     *info,
                   UChar *prefixBuffer,
                   int32_t *prefixBufLen,
                   UErrorCode *status)
{
    if(info->fShowSign) {
        *prefixBufLen = unum_getTextAttribute(format,
                                              UNUM_POSITIVE_PREFIX,
                                              prefixBuffer,
                                              *prefixBufLen,
                                              status);
        if (info->fSpace) {
            
            
            unum_setTextAttribute(format, UNUM_POSITIVE_PREFIX, gSpaceStr, 1, status);
        }
        else {
            UChar plusSymbol[UPRINTF_SYMBOL_BUFFER_SIZE];
            int32_t symbolLen;

            symbolLen = unum_getSymbol(format,
                UNUM_PLUS_SIGN_SYMBOL,
                plusSymbol,
                sizeof(plusSymbol)/sizeof(*plusSymbol),
                status);
            unum_setTextAttribute(format,
                UNUM_POSITIVE_PREFIX,
                plusSymbol,
                symbolLen,
                status);
        }
    }
    else {
        *prefixBufLen = 0;
    }
}

static void
u_printf_reset_sign(UNumberFormat        *format,
                   const u_printf_spec_info     *info,
                   UChar *prefixBuffer,
                   int32_t *prefixBufLen,
                   UErrorCode *status)
{
    if(info->fShowSign) {
        unum_setTextAttribute(format,
                              UNUM_POSITIVE_PREFIX,
                              prefixBuffer,
                              *prefixBufLen,
                              status);
    }
}



static int32_t
u_printf_simple_percent_handler(const u_printf_stream_handler  *handler,
                                void                           *context,
                                ULocaleBundle                  *formatBundle,
                                const u_printf_spec_info       *info,
                                const ufmt_args                *args)
{
    static const UChar PERCENT[] = { UP_PERCENT };

    
    return handler->write(context, PERCENT, 1);
}


static int32_t
u_printf_string_handler(const u_printf_stream_handler  *handler,
                        void                           *context,
                        ULocaleBundle                  *formatBundle,
                        const u_printf_spec_info       *info,
                        const ufmt_args                *args)
{
    UChar *s;
    UChar buffer[UFMT_DEFAULT_BUFFER_SIZE];
    int32_t len, written;
    int32_t argSize;
    const char *arg = (const char*)(args[0].ptrValue);

    
    if (arg) {
        argSize = (int32_t)strlen(arg) + 1;
        if (argSize >= MAX_UCHAR_BUFFER_SIZE(buffer)) {
            s = ufmt_defaultCPToUnicode(arg, argSize,
                    (UChar *)uprv_malloc(MAX_UCHAR_BUFFER_NEEDED(argSize)),
                    MAX_UCHAR_BUFFER_NEEDED(argSize));
            if(s == NULL) {
                return 0;
            }
        }
        else {
            s = ufmt_defaultCPToUnicode(arg, argSize, buffer,
                    sizeof(buffer)/sizeof(UChar));
        }
    }
    else {
        s = (UChar *)gNullStr;
    }
    len = u_strlen(s);

    
    
    if (info->fPrecision != -1 && info->fPrecision < len) {
        len = info->fPrecision;
    }

    written = handler->pad_and_justify(context, info, s, len);

    
    if (gNullStr != s && buffer != s) {
        uprv_free(s);
    }

    return written;
}

static int32_t
u_printf_char_handler(const u_printf_stream_handler  *handler,
                      void                           *context,
                      ULocaleBundle                  *formatBundle,
                      const u_printf_spec_info       *info,
                      const ufmt_args                *args)
{
    UChar s[U16_MAX_LENGTH+1];
    int32_t len = 1, written;
    unsigned char arg = (unsigned char)(args[0].int64Value);

    
    ufmt_defaultCPToUnicode((const char *)&arg, 2, s, sizeof(s)/sizeof(UChar));

    
    if (arg != 0) {
        len = u_strlen(s);
    }

    
    
    

    written = handler->pad_and_justify(context, info, s, len);

    return written;
}

static int32_t
u_printf_double_handler(const u_printf_stream_handler  *handler,
                        void                           *context,
                        ULocaleBundle                  *formatBundle,
                        const u_printf_spec_info       *info,
                        const ufmt_args                *args)
{
    double        num         = (double) (args[0].doubleValue);
    UNumberFormat  *format;
    UChar          result[UPRINTF_BUFFER_SIZE];
    UChar          prefixBuffer[UPRINTF_BUFFER_SIZE];
    int32_t        prefixBufferLen = sizeof(prefixBuffer);
    int32_t        minDecimalDigits;
    int32_t        maxDecimalDigits;
    int32_t        resultLen;
    UErrorCode     status        = U_ZERO_ERROR;

    prefixBuffer[0] = 0;

    
    


    
    format = u_locbund_getNumberFormat(formatBundle, UNUM_DECIMAL);

    
    if(format == 0)
        return 0;

    
    minDecimalDigits = unum_getAttribute(format, UNUM_MIN_FRACTION_DIGITS);
    maxDecimalDigits = unum_getAttribute(format, UNUM_MAX_FRACTION_DIGITS);

    
    if(info->fPrecision != -1) {
        
        unum_setAttribute(format, UNUM_FRACTION_DIGITS, info->fPrecision);
    }
    else if(info->fAlt) {
        
        
        unum_setAttribute(format, UNUM_FRACTION_DIGITS, 6);
    }
    else {
        
        unum_setAttribute(format, UNUM_FRACTION_DIGITS, 6);
    }

    
    if (info->fShowSign) {
        u_printf_set_sign(format, info, prefixBuffer, &prefixBufferLen, &status);
    }

    
    resultLen = unum_formatDouble(format, num, result, UPRINTF_BUFFER_SIZE, 0, &status);

    if (U_FAILURE(status)) {
        resultLen = 0;
    }

    
    
    unum_setAttribute(format, UNUM_MIN_FRACTION_DIGITS, minDecimalDigits);
    unum_setAttribute(format, UNUM_MAX_FRACTION_DIGITS, maxDecimalDigits);

    if (info->fShowSign) {
        
        UErrorCode localStatus = U_ZERO_ERROR;
        u_printf_reset_sign(format, info, prefixBuffer, &prefixBufferLen, &localStatus);
    }

    return handler->pad_and_justify(context, info, result, resultLen);
}


static int32_t
u_printf_integer_handler(const u_printf_stream_handler  *handler,
                         void                           *context,
                         ULocaleBundle                  *formatBundle,
                         const u_printf_spec_info       *info,
                         const ufmt_args                *args)
{
    int64_t         num        = args[0].int64Value;
    UNumberFormat   *format;
    UChar           result[UPRINTF_BUFFER_SIZE];
    UChar           prefixBuffer[UPRINTF_BUFFER_SIZE];
    int32_t         prefixBufferLen = sizeof(prefixBuffer);
    int32_t         minDigits     = -1;
    int32_t         resultLen;
    UErrorCode      status        = U_ZERO_ERROR;

    prefixBuffer[0] = 0;

    
    if (info->fIsShort)
        num = (int16_t)num;
    else if (!info->fIsLongLong)
        num = (int32_t)num;

    
    format = u_locbund_getNumberFormat(formatBundle, UNUM_DECIMAL);

    
    if(format == 0)
        return 0;

    

    
    if(info->fPrecision != -1) {
        
        minDigits = unum_getAttribute(format, UNUM_MIN_INTEGER_DIGITS);
        unum_setAttribute(format, UNUM_MIN_INTEGER_DIGITS, info->fPrecision);
    }

    
    if(info->fShowSign) {
        u_printf_set_sign(format, info, prefixBuffer, &prefixBufferLen, &status);
    }

    
    resultLen = unum_formatInt64(format, num, result, UPRINTF_BUFFER_SIZE, 0, &status);

    if (U_FAILURE(status)) {
        resultLen = 0;
    }

    
    if (minDigits != -1) {
        unum_setAttribute(format, UNUM_MIN_INTEGER_DIGITS, minDigits);
    }

    if (info->fShowSign) {
        
        UErrorCode localStatus = U_ZERO_ERROR;
        u_printf_reset_sign(format, info, prefixBuffer, &prefixBufferLen, &localStatus);
    }

    return handler->pad_and_justify(context, info, result, resultLen);
}

static int32_t
u_printf_hex_handler(const u_printf_stream_handler  *handler,
                     void                           *context,
                     ULocaleBundle                  *formatBundle,
                     const u_printf_spec_info       *info,
                     const ufmt_args                *args)
{
    int64_t         num        = args[0].int64Value;
    UChar           result[UPRINTF_BUFFER_SIZE];
    int32_t         len        = UPRINTF_BUFFER_SIZE;


    
    if (info->fIsShort)
        num &= UINT16_MAX;
    else if (!info->fIsLongLong)
        num &= UINT32_MAX;

    
    ufmt_64tou(result, &len, num, 16,
        (UBool)(info->fSpec == 0x0078),
        (info->fPrecision == -1 && info->fZero) ? info->fWidth : info->fPrecision);

    
    if(num != 0 && info->fAlt && len < UPRINTF_BUFFER_SIZE - 2) {
        
        memmove(result + 2, result, len * sizeof(UChar));
        result[0] = 0x0030;
        result[1] = info->fSpec;
        len += 2;
    }

    return handler->pad_and_justify(context, info, result, len);
}

static int32_t
u_printf_octal_handler(const u_printf_stream_handler  *handler,
                       void                           *context,
                       ULocaleBundle                  *formatBundle,
                       const u_printf_spec_info       *info,
                       const ufmt_args                *args)
{
    int64_t         num        = args[0].int64Value;
    UChar           result[UPRINTF_BUFFER_SIZE];
    int32_t         len        = UPRINTF_BUFFER_SIZE;


    
    if (info->fIsShort)
        num &= UINT16_MAX;
    else if (!info->fIsLongLong)
        num &= UINT32_MAX;

    
    ufmt_64tou(result, &len, num, 8,
        FALSE, 
        info->fPrecision == -1 && info->fZero ? info->fWidth : info->fPrecision);

    
    if(info->fAlt && result[0] != 0x0030 && len < UPRINTF_BUFFER_SIZE - 1) {
        
        memmove(result + 1, result, len * sizeof(UChar));
        result[0] = 0x0030;
        len += 1;
    }

    return handler->pad_and_justify(context, info, result, len);
}

static int32_t
u_printf_uinteger_handler(const u_printf_stream_handler *handler,
                          void                          *context,
                          ULocaleBundle                 *formatBundle,
                          const u_printf_spec_info      *info,
                          const ufmt_args               *args)
{
    int64_t         num        = args[0].int64Value;
    UNumberFormat   *format;
    UChar           result[UPRINTF_BUFFER_SIZE];
    int32_t         minDigits     = -1;
    int32_t         resultLen;
    UErrorCode      status        = U_ZERO_ERROR;

    
    if (info->fIsShort)
        num &= UINT16_MAX;
    else if (!info->fIsLongLong)
        num &= UINT32_MAX;

    
    format = u_locbund_getNumberFormat(formatBundle, UNUM_DECIMAL);

    
    if(format == 0)
        return 0;

    

    
    if(info->fPrecision != -1) {
        
        minDigits = unum_getAttribute(format, UNUM_MIN_INTEGER_DIGITS);
        unum_setAttribute(format, UNUM_MIN_INTEGER_DIGITS, info->fPrecision);
    }

    

    
    resultLen = unum_formatInt64(format, num, result, UPRINTF_BUFFER_SIZE, 0, &status);

    if (U_FAILURE(status)) {
        resultLen = 0;
    }

    
    if (minDigits != -1) {
        unum_setAttribute(format, UNUM_MIN_INTEGER_DIGITS, minDigits);
    }

    return handler->pad_and_justify(context, info, result, resultLen);
}

static int32_t
u_printf_pointer_handler(const u_printf_stream_handler  *handler,
                         void                           *context,
                         ULocaleBundle                  *formatBundle,
                         const u_printf_spec_info       *info,
                         const ufmt_args                *args)
{
    UChar           result[UPRINTF_BUFFER_SIZE];
    int32_t         len  = UPRINTF_BUFFER_SIZE;

    
    ufmt_ptou(result, &len, args[0].ptrValue, TRUE);

    return handler->pad_and_justify(context, info, result, len);
}

static int32_t
u_printf_scientific_handler(const u_printf_stream_handler  *handler,
                            void                           *context,
                            ULocaleBundle                  *formatBundle,
                            const u_printf_spec_info       *info,
                            const ufmt_args                *args)
{
    double          num         = (double) (args[0].doubleValue);
    UNumberFormat   *format;
    UChar           result[UPRINTF_BUFFER_SIZE];
    UChar           prefixBuffer[UPRINTF_BUFFER_SIZE];
    int32_t         prefixBufferLen = sizeof(prefixBuffer);
    int32_t         minDecimalDigits;
    int32_t         maxDecimalDigits;
    UErrorCode      status        = U_ZERO_ERROR;
    UChar srcExpBuf[UPRINTF_SYMBOL_BUFFER_SIZE];
    int32_t srcLen, expLen;
    int32_t resultLen;
    UChar expBuf[UPRINTF_SYMBOL_BUFFER_SIZE];

    prefixBuffer[0] = 0;

    
    


    
    format = u_locbund_getNumberFormat(formatBundle, UNUM_SCIENTIFIC);

    
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
            formatBundle->fLocale,
            &status);
    }
    else {
        expLen = u_strToUpper(expBuf, (int32_t)sizeof(expBuf),
            srcExpBuf, srcLen,
            formatBundle->fLocale,
            &status);
    }

    unum_setSymbol(format,
        UNUM_EXPONENTIAL_SYMBOL,
        expBuf,
        expLen,
        &status);

    
    minDecimalDigits = unum_getAttribute(format, UNUM_MIN_FRACTION_DIGITS);
    maxDecimalDigits = unum_getAttribute(format, UNUM_MAX_FRACTION_DIGITS);

    
    if(info->fPrecision != -1) {
        
        if (info->fOrigSpec == (UChar)0x65  || info->fOrigSpec == (UChar)0x45 ) {
            unum_setAttribute(format, UNUM_FRACTION_DIGITS, info->fPrecision);
        }
        else {
            unum_setAttribute(format, UNUM_MIN_FRACTION_DIGITS, 1);
            unum_setAttribute(format, UNUM_MAX_FRACTION_DIGITS, info->fPrecision);
        }
    }
    else if(info->fAlt) {
        
        
        unum_setAttribute(format, UNUM_FRACTION_DIGITS, 6);
    }
    else {
        
        unum_setAttribute(format, UNUM_FRACTION_DIGITS, 6);
    }

    
    if (info->fShowSign) {
        u_printf_set_sign(format, info, prefixBuffer, &prefixBufferLen, &status);
    }

    
    resultLen = unum_formatDouble(format, num, result, UPRINTF_BUFFER_SIZE, 0, &status);

    if (U_FAILURE(status)) {
        resultLen = 0;
    }

    
    
    unum_setAttribute(format, UNUM_MIN_FRACTION_DIGITS, minDecimalDigits);
    unum_setAttribute(format, UNUM_MAX_FRACTION_DIGITS, maxDecimalDigits);

    

    





    if (info->fShowSign) {
        
        UErrorCode localStatus = U_ZERO_ERROR;
        u_printf_reset_sign(format, info, prefixBuffer, &prefixBufferLen, &localStatus);
    }

    return handler->pad_and_justify(context, info, result, resultLen);
}

static int32_t
u_printf_percent_handler(const u_printf_stream_handler  *handler,
                         void                           *context,
                         ULocaleBundle                  *formatBundle,
                         const u_printf_spec_info       *info,
                         const ufmt_args                *args)
{
    double          num         = (double) (args[0].doubleValue);
    UNumberFormat   *format;
    UChar           result[UPRINTF_BUFFER_SIZE];
    UChar           prefixBuffer[UPRINTF_BUFFER_SIZE];
    int32_t         prefixBufferLen = sizeof(prefixBuffer);
    int32_t         minDecimalDigits;
    int32_t         maxDecimalDigits;
    int32_t         resultLen;
    UErrorCode      status        = U_ZERO_ERROR;

    prefixBuffer[0] = 0;

    
    


    
    format = u_locbund_getNumberFormat(formatBundle, UNUM_PERCENT);

    
    if(format == 0)
        return 0;

    
    minDecimalDigits = unum_getAttribute(format, UNUM_MIN_FRACTION_DIGITS);
    maxDecimalDigits = unum_getAttribute(format, UNUM_MAX_FRACTION_DIGITS);

    
    if(info->fPrecision != -1) {
        
        unum_setAttribute(format, UNUM_FRACTION_DIGITS, info->fPrecision);
    }
    else if(info->fAlt) {
        
        
        unum_setAttribute(format, UNUM_FRACTION_DIGITS, 6);
    }
    else {
        
        unum_setAttribute(format, UNUM_FRACTION_DIGITS, 6);
    }

    
    if (info->fShowSign) {
        u_printf_set_sign(format, info, prefixBuffer, &prefixBufferLen, &status);
    }

    
    resultLen = unum_formatDouble(format, num, result, UPRINTF_BUFFER_SIZE, 0, &status);

    if (U_FAILURE(status)) {
        resultLen = 0;
    }

    
    
    unum_setAttribute(format, UNUM_MIN_FRACTION_DIGITS, minDecimalDigits);
    unum_setAttribute(format, UNUM_MAX_FRACTION_DIGITS, maxDecimalDigits);

    if (info->fShowSign) {
        
        UErrorCode localStatus = U_ZERO_ERROR;
        u_printf_reset_sign(format, info, prefixBuffer, &prefixBufferLen, &localStatus);
    }

    return handler->pad_and_justify(context, info, result, resultLen);
}

static int32_t
u_printf_ustring_handler(const u_printf_stream_handler  *handler,
                         void                           *context,
                         ULocaleBundle                  *formatBundle,
                         const u_printf_spec_info       *info,
                         const ufmt_args                *args)
{
    int32_t len, written;
    const UChar *arg = (const UChar*)(args[0].ptrValue);

    
    if (arg == NULL) {
        arg = gNullStr;
    }
    len = u_strlen(arg);

    
    
    if (info->fPrecision != -1 && info->fPrecision < len) {
        len = info->fPrecision;
    }

    
    written = handler->pad_and_justify(context, info, arg, len);

    return written;
}

static int32_t
u_printf_uchar_handler(const u_printf_stream_handler  *handler,
                       void                           *context,
                       ULocaleBundle                  *formatBundle,
                       const u_printf_spec_info       *info,
                       const ufmt_args                *args)
{
    int32_t written = 0;
    UChar arg = (UChar)(args[0].int64Value);

    
    
    

    
    written = handler->pad_and_justify(context, info, &arg, 1);

    return written;
}

static int32_t
u_printf_scidbl_handler(const u_printf_stream_handler  *handler,
                        void                           *context,
                        ULocaleBundle                  *formatBundle,
                        const u_printf_spec_info       *info,
                        const ufmt_args                *args)
{
    u_printf_spec_info scidbl_info;
    double      num = args[0].doubleValue;
    int32_t     retVal;
    UNumberFormat *format;
    int32_t maxSigDecimalDigits, significantDigits;

    memcpy(&scidbl_info, info, sizeof(u_printf_spec_info));

    
    if (scidbl_info.fPrecision == -1 && num == uprv_trunc(num))
    {
        
        scidbl_info.fSpec = 0x0066;
        scidbl_info.fPrecision = 0;
        
        retVal = u_printf_double_handler(handler, context, formatBundle, &scidbl_info, args);
    }
    else if(num < 0.0001 || (scidbl_info.fPrecision < 1 && 1000000.0 <= num)
        || (scidbl_info.fPrecision != -1 && num > uprv_pow10(scidbl_info.fPrecision)))
    {
        
        scidbl_info.fSpec = scidbl_info.fSpec - 2;
        if (scidbl_info.fPrecision == -1) {
            scidbl_info.fPrecision = 5;
        }
        
        retVal = u_printf_scientific_handler(handler, context, formatBundle, &scidbl_info, args);
    }
    else {
        format = u_locbund_getNumberFormat(formatBundle, UNUM_DECIMAL);
        
        if (format == NULL) {
            return 0;
        }
        maxSigDecimalDigits = unum_getAttribute(format, UNUM_MAX_SIGNIFICANT_DIGITS);
        significantDigits = scidbl_info.fPrecision;

        
        scidbl_info.fSpec = 0x0066;
        if (significantDigits == -1) {
            significantDigits = 6;
        }
        unum_setAttribute(format, UNUM_SIGNIFICANT_DIGITS_USED, TRUE);
        unum_setAttribute(format, UNUM_MAX_SIGNIFICANT_DIGITS, significantDigits);
        
        retVal = u_printf_double_handler(handler, context, formatBundle, &scidbl_info, args);
        unum_setAttribute(format, UNUM_MAX_SIGNIFICANT_DIGITS, maxSigDecimalDigits);
        unum_setAttribute(format, UNUM_SIGNIFICANT_DIGITS_USED, FALSE);
    }
    return retVal;
}

static int32_t
u_printf_count_handler(const u_printf_stream_handler  *handler,
                       void                           *context,
                       ULocaleBundle                  *formatBundle,
                       const u_printf_spec_info       *info,
                       const ufmt_args                *args)
{
    int32_t *count = (int32_t*)(args[0].ptrValue);

    
    
    *count = info->fWidth;

    return 0;
}

static int32_t
u_printf_spellout_handler(const u_printf_stream_handler *handler,
                          void                          *context,
                          ULocaleBundle                 *formatBundle,
                          const u_printf_spec_info      *info,
                          const ufmt_args               *args)
{
    double          num         = (double) (args[0].doubleValue);
    UNumberFormat   *format;
    UChar           result[UPRINTF_BUFFER_SIZE];
    UChar           prefixBuffer[UPRINTF_BUFFER_SIZE];
    int32_t         prefixBufferLen = sizeof(prefixBuffer);
    int32_t         minDecimalDigits;
    int32_t         maxDecimalDigits;
    int32_t         resultLen;
    UErrorCode      status        = U_ZERO_ERROR;

    prefixBuffer[0] = 0;

    
    


    
    format = u_locbund_getNumberFormat(formatBundle, UNUM_SPELLOUT);

    
    if(format == 0)
        return 0;

    
    minDecimalDigits = unum_getAttribute(format, UNUM_MIN_FRACTION_DIGITS);
    maxDecimalDigits = unum_getAttribute(format, UNUM_MAX_FRACTION_DIGITS);

    
    if(info->fPrecision != -1) {
        
        unum_setAttribute(format, UNUM_FRACTION_DIGITS, info->fPrecision);
    }
    else if(info->fAlt) {
        
        
        unum_setAttribute(format, UNUM_FRACTION_DIGITS, 6);
    }
    else {
        
        unum_setAttribute(format, UNUM_FRACTION_DIGITS, 6);
    }

    
    if (info->fShowSign) {
        u_printf_set_sign(format, info, prefixBuffer, &prefixBufferLen, &status);
    }

    
    resultLen = unum_formatDouble(format, num, result, UPRINTF_BUFFER_SIZE, 0, &status);

    if (U_FAILURE(status)) {
        resultLen = 0;
    }

    
    
    unum_setAttribute(format, UNUM_MIN_FRACTION_DIGITS, minDecimalDigits);
    unum_setAttribute(format, UNUM_MAX_FRACTION_DIGITS, maxDecimalDigits);

    if (info->fShowSign) {
        
        UErrorCode localStatus = U_ZERO_ERROR;
        u_printf_reset_sign(format, info, prefixBuffer, &prefixBufferLen, &localStatus);
    }

    return handler->pad_and_justify(context, info, result, resultLen);
}





static const u_printf_info g_u_printf_infos[UPRINTF_NUM_FMT_HANDLERS] = {

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
    UFMT_HEX,           UFMT_EMPTY,         UFMT_EMPTY,         UFMT_EMPTY,
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


#define FLAG_MINUS 0x002D
#define FLAG_PLUS 0x002B
#define FLAG_SPACE 0x0020
#define FLAG_POUND 0x0023
#define FLAG_ZERO  0x0030
#define FLAG_PAREN 0x0028

#define ISFLAG(s)    (s) == FLAG_MINUS || \
            (s) == FLAG_PLUS || \
            (s) == FLAG_SPACE || \
            (s) == FLAG_POUND || \
            (s) == FLAG_ZERO || \
            (s) == FLAG_PAREN


#define SPEC_ASTERISK 0x002A
#define SPEC_DOLLARSIGN 0x0024
#define SPEC_PERIOD 0x002E
#define SPEC_PERCENT 0x0025


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

static ufmt_args* parseArguments(const UChar *alias, va_list ap, UErrorCode *status) {
    ufmt_args *arglist = NULL;
    ufmt_type_info *typelist = NULL;
    UBool *islonglong = NULL;
    int32_t size = 0;
    int32_t pos = 0;
    UChar type;
    uint16_t handlerNum;
    const UChar *aliasStart = alias;

    
    for(;;) {
        
        while(*alias != UP_PERCENT && *alias != 0x0000) {
            alias++;
        }

        if(*alias == 0x0000) {
            break;
        }

        alias++;

        
        if(ISDIGIT(*alias)) {

            
            if(ISDIGIT(*alias)) {
                pos = (int) (*alias++ - DIGIT_ZERO);

                while(ISDIGIT(*alias)) {
                    pos *= 10;
                    pos += (int) (*alias++ - DIGIT_ZERO);
                }
            }

            
            if(*alias != SPEC_DOLLARSIGN) {
                return NULL;
            }
        } else {
            return NULL;
        }

        if (pos > size) {
            size = pos;
        }
    }

    
    typelist = (ufmt_type_info*)uprv_malloc(sizeof(ufmt_type_info) * size);
    islonglong = (UBool*)uprv_malloc(sizeof(UBool) * size);
    arglist = (ufmt_args*)uprv_malloc(sizeof(ufmt_args) * size);

    
    if (!typelist || !islonglong || !arglist) {
        if (typelist) {
            uprv_free(typelist);
        }

        if (islonglong) {
            uprv_free(islonglong);
        }

        if (arglist) {
            uprv_free(arglist);
        }

        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }

    
    alias = aliasStart;

    for(;;) {
        
        while(*alias != UP_PERCENT && *alias != 0x0000) {
            alias++;
        }

        if(*alias == 0x0000) {
            break;
        }

        alias++;

        
        if(ISDIGIT(*alias)) {
            pos = (int) (*alias++ - DIGIT_ZERO);

            while(ISDIGIT(*alias)) {
                pos *= 10;
                pos += (int) (*alias++ - DIGIT_ZERO);
            }
        }
        
        pos--;

        
        while (ISMOD(*alias) || ISFLAG(*alias) || ISDIGIT(*alias) || 
            *alias == SPEC_ASTERISK || *alias == SPEC_PERIOD || *alias == SPEC_DOLLARSIGN) {
                islonglong[pos] = FALSE;
                if (ISMOD(*alias)) {
                    alias++;
                    if (*alias == MOD_LOWERL) {
                        islonglong[pos] = TRUE;
                    } 
                } 
                alias++;
        }
        type = *alias;

        
        handlerNum = (uint16_t)(type - UPRINTF_BASE_FMT_HANDLERS);
        if (handlerNum < UPRINTF_NUM_FMT_HANDLERS) {
            typelist[pos] = g_u_printf_infos[ handlerNum ].info;
        } else {
            typelist[pos] = ufmt_empty;
        }
    }

    
    for (pos = 0; pos < size; pos++) {
        switch (typelist[pos]) {
        case ufmt_string:
        case ufmt_ustring:
        case ufmt_pointer:
            arglist[pos].ptrValue = va_arg(ap, void*);
            break;
        case ufmt_char:
        case ufmt_uchar:
        case ufmt_int:
            if (islonglong[pos]) {
                arglist[pos].int64Value = va_arg(ap, int64_t);
            }
            else {
                arglist[pos].int64Value = va_arg(ap, int32_t);
            }
            break;
        case ufmt_float:
            arglist[pos].floatValue = (float) va_arg(ap, double);
            break;
        case ufmt_double:
            arglist[pos].doubleValue = va_arg(ap, double);
            break;
        default:
            
            arglist[pos].ptrValue = NULL;
            break;
        }
    }

    uprv_free(typelist);
    uprv_free(islonglong);

    return arglist;
}


U_CFUNC int32_t
u_printf_parse(const u_printf_stream_handler *streamHandler,
               const UChar     *fmt,
               void            *context,
               u_localized_print_string *locStringContext,
               ULocaleBundle   *formatBundle,
               int32_t         *written,
               va_list         ap)
{
    uint16_t         handlerNum;
    ufmt_args        args;
    ufmt_type_info   argType;
    u_printf_handler *handler;
    u_printf_spec    spec;
    u_printf_spec_info *info = &(spec.fInfo);

    const UChar *alias = fmt;
    const UChar *backup;
    const UChar *lastAlias;
    const UChar *orgAlias = fmt;
    
    ufmt_args *arglist = NULL; 
    UErrorCode status = U_ZERO_ERROR;
    if (!locStringContext || locStringContext->available >= 0) {
        
        arglist = parseArguments(orgAlias, ap, &status);

        
        if (U_FAILURE(status)) {
            return -1;
        }
    }
    
    
    while(!locStringContext || locStringContext->available >= 0) {

        
        lastAlias = alias;
        while(*alias != UP_PERCENT && *alias != 0x0000) {
            alias++;
        }

        
        if(alias > lastAlias) {
            *written += (streamHandler->write)(context, lastAlias, (int32_t)(alias - lastAlias));
        }

        
        if(*alias == 0x0000) {
            break;
        }

        
        spec.fWidthPos     = -1;
        spec.fPrecisionPos = -1;
        spec.fArgPos       = -1;

        uprv_memset(info, 0, sizeof(*info));
        info->fPrecision    = -1;
        info->fWidth        = -1;
        info->fPadChar      = 0x0020;

        
        alias++;

        
        if(ISDIGIT(*alias)) {

            
            backup = alias;

            
            if(ISDIGIT(*alias)) {
                spec.fArgPos = (int) (*alias++ - DIGIT_ZERO);

                while(ISDIGIT(*alias)) {
                    spec.fArgPos *= 10;
                    spec.fArgPos += (int) (*alias++ - DIGIT_ZERO);
                }
            }

            
            if(*alias != SPEC_DOLLARSIGN) {
                spec.fArgPos = -1;
                alias = backup;
            }
            
            else
                alias++;
        }

        
        while(ISFLAG(*alias)) {
            switch(*alias++) {

                
            case FLAG_MINUS:
                info->fLeft = TRUE;
                break;

                
            case FLAG_PLUS:
                info->fShowSign = TRUE;
                break;

                
            case FLAG_SPACE:
                info->fShowSign = TRUE;
                info->fSpace = TRUE;
                break;

                
            case FLAG_POUND:
                info->fAlt = TRUE;
                break;

                
            case FLAG_ZERO:
                info->fZero = TRUE;
                info->fPadChar = 0x0030;
                break;

                
            case FLAG_PAREN:

                
                
                info->fPadChar = (UChar)ufmt_digitvalue(*alias++);
                info->fPadChar = (UChar)((info->fPadChar * 16) + ufmt_digitvalue(*alias++));
                info->fPadChar = (UChar)((info->fPadChar * 16) + ufmt_digitvalue(*alias++));
                info->fPadChar = (UChar)((info->fPadChar * 16) + ufmt_digitvalue(*alias++));

                
                alias++;

                break;
            }
        }

        

        
        if(*alias == SPEC_ASTERISK) {

            info->fWidth = -2;

            
            alias++;

            
            backup = alias;

            
            if(ISDIGIT(*alias)) {
                spec.fWidthPos = (int) (*alias++ - DIGIT_ZERO);

                while(ISDIGIT(*alias)) {
                    spec.fWidthPos *= 10;
                    spec.fWidthPos += (int) (*alias++ - DIGIT_ZERO);
                }
            }

            
            if(*alias != SPEC_DOLLARSIGN) {
                spec.fWidthPos = -1;
                alias = backup;
            }
            
            else
                alias++;
        }
        
        else if(ISDIGIT(*alias)){
            info->fWidth = (int) (*alias++ - DIGIT_ZERO);

            while(ISDIGIT(*alias)) {
                info->fWidth *= 10;
                info->fWidth += (int) (*alias++ - DIGIT_ZERO);
            }
        }

        

        if(*alias == SPEC_PERIOD) {

            
            alias++;

            
            if(*alias == SPEC_ASTERISK) {

                info->fPrecision = -2;

                
                alias++;

                
                backup = alias;

                
                if(ISDIGIT(*alias)) {
                    spec.fPrecisionPos = (int) (*alias++ - DIGIT_ZERO);

                    while(ISDIGIT(*alias)) {
                        spec.fPrecisionPos *= 10;
                        spec.fPrecisionPos += (int) (*alias++ - DIGIT_ZERO);
                    }

                    
                    if(*alias != SPEC_DOLLARSIGN) {
                        spec.fPrecisionPos = -1;
                        alias = backup;
                    }
                    else {
                        
                        alias++;
                    }
                }
            }
            
            else if(ISDIGIT(*alias)){
                info->fPrecision = (int) (*alias++ - DIGIT_ZERO);

                while(ISDIGIT(*alias)) {
                    info->fPrecision *= 10;
                    info->fPrecision += (int) (*alias++ - DIGIT_ZERO);
                }
            }
        }

        
        if(ISMOD(*alias)) {
            switch(*alias++) {

                
            case MOD_H:
                info->fIsShort = TRUE;
                break;

                
            case MOD_LOWERL:
                if(*alias == MOD_LOWERL) {
                    info->fIsLongLong = TRUE;
                    
                    alias++;
                }
                else
                    info->fIsLong = TRUE;
                break;

                
            case MOD_L:
                info->fIsLongDouble = TRUE;
                break;
            }
        }

        
        info->fSpec = *alias++;
        info->fOrigSpec = info->fSpec;

        

        
        if(spec.fInfo.fWidth == -2) {
            if(spec.fWidthPos == -1) {
                
                info->fWidth = va_arg(ap, int32_t);
            }
            

            
            if(info->fWidth < 0) {
                info->fWidth *= -1; 
                info->fLeft = TRUE;
            }
        }

        
        if(info->fPrecision == -2) {
            if(spec.fPrecisionPos == -1) {
                
                info->fPrecision = va_arg(ap, int32_t);
            }
            

            
            if(info->fPrecision < 0)
                info->fPrecision = 0;
        }

        handlerNum = (uint16_t)(info->fSpec - UPRINTF_BASE_FMT_HANDLERS);
        if (handlerNum < UPRINTF_NUM_FMT_HANDLERS) {
            
            argType = g_u_printf_infos[ handlerNum ].info;

            
            if (spec.fArgPos > 0) {
                
                spec.fArgPos--;
                switch(argType) {
                case ufmt_count:
                    
                    info->fWidth = *written;
                    
                case ufmt_string:
                case ufmt_ustring:
                case ufmt_pointer:
                    args.ptrValue = arglist[spec.fArgPos].ptrValue;
                    break;
                case ufmt_char:
                case ufmt_uchar:
                case ufmt_int:
                    args.int64Value = arglist[spec.fArgPos].int64Value;
                    break;
                case ufmt_float:
                    args.floatValue = arglist[spec.fArgPos].floatValue;
                    break;
                case ufmt_double:
                    args.doubleValue = arglist[spec.fArgPos].doubleValue;
                    break;
                default:
                    
                    args.ptrValue = NULL;
                    break;
                }
            } else { 
                switch(argType) {
                case ufmt_count:
                    
                    info->fWidth = *written;
                    
                case ufmt_string:
                case ufmt_ustring:
                case ufmt_pointer:
                    args.ptrValue = va_arg(ap, void*);
                    break;
                case ufmt_char:
                case ufmt_uchar:
                case ufmt_int:
                    if (info->fIsLongLong) {
                        args.int64Value = va_arg(ap, int64_t);
                    }
                    else {
                        args.int64Value = va_arg(ap, int32_t);
                    }
                    break;
                case ufmt_float:
                    args.floatValue = (float) va_arg(ap, double);
                    break;
                case ufmt_double:
                    args.doubleValue = va_arg(ap, double);
                    break;
                default:
                    
                    args.ptrValue = NULL;
                    break;
                }
            }

            
            handler = g_u_printf_infos[ handlerNum ].handler;
            if(handler != 0) {
                *written += (*handler)(streamHandler, context, formatBundle, info, &args);
            }
            else {
                
                *written += (streamHandler->write)(context, fmt, (int32_t)(alias - lastAlias));
            }
        }
        else {
            
            *written += (streamHandler->write)(context, fmt, (int32_t)(alias - lastAlias));
        }
    }
    
    if (arglist != NULL) {
        uprv_free(arglist);
    }
    
    return (int32_t)(alias - fmt);
}

#endif 

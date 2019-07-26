

















#ifndef UPRINTF_H
#define UPRINTF_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/ustdio.h"
#include "ufmt_cmn.h"
#include "locbund.h"




typedef struct u_printf_spec_info {
  int32_t    fPrecision;    
  int32_t    fWidth;        

  UChar     fOrigSpec;      
  UChar     fSpec;          
  UChar     fPadChar;       

  UBool     fAlt;           
  UBool     fSpace;         
  UBool     fLeft;          
  UBool     fShowSign;      
  UBool     fZero;          

  UBool     fIsLongDouble;  
  UBool     fIsShort;       
  UBool     fIsLong;        
  UBool     fIsLongLong;    
} u_printf_spec_info;

typedef int32_t U_EXPORT2
u_printf_write_stream(void          *context,
                      const UChar   *str,
                      int32_t       count);

typedef int32_t U_EXPORT2
u_printf_pad_and_justify_stream(void                        *context,
                                const u_printf_spec_info    *info,
                                const UChar                 *result,
                                int32_t                     resultLen);

typedef struct u_printf_stream_handler {
    u_printf_write_stream *write;
    u_printf_pad_and_justify_stream *pad_and_justify;
} u_printf_stream_handler;


typedef struct u_localized_print_string {
    UChar     *str;     
    int32_t   available;
    int32_t   len;      

    ULocaleBundle  fBundle;     
} u_localized_print_string;

#define UP_PERCENT 0x0025











U_CFUNC int32_t
u_printf_parse(const u_printf_stream_handler *streamHandler,
               const UChar     *fmt,
               void            *context,
               u_localized_print_string *locStringContext,
               ULocaleBundle   *formatBundle,
               int32_t         *written,
               va_list         ap);

#endif 

#endif

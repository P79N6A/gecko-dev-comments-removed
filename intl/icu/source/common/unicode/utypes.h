






























#ifndef UTYPES_H
#define UTYPES_H


#include "unicode/umachine.h"
#include "unicode/uversion.h"
#include "unicode/uconfig.h"
#include <float.h>

#if !U_NO_DEFAULT_INCLUDE_UTF_HEADERS
#   include "unicode/utf.h"
#endif
















#ifdef __cplusplus
#   ifndef U_SHOW_CPLUSPLUS_API
#       define U_SHOW_CPLUSPLUS_API 1
#   endif
#else
#   undef U_SHOW_CPLUSPLUS_API
#   define U_SHOW_CPLUSPLUS_API 0
#endif













#if !U_DEFAULT_SHOW_DRAFT && !defined(U_SHOW_DRAFT_API)
#define U_HIDE_DRAFT_API 1
#endif
#if !U_DEFAULT_SHOW_DRAFT && !defined(U_SHOW_INTERNAL_API)
#define U_HIDE_INTERNAL_API 1
#endif























#if U_CHARSET_FAMILY
#   if U_IS_BIG_ENDIAN
   
#     define U_ICUDATA_TYPE_LETTER "e"
#     define U_ICUDATA_TYPE_LITLETTER e
#   else
#     error "Don't know what to do with little endian EBCDIC!"
#     define U_ICUDATA_TYPE_LETTER "x"
#     define U_ICUDATA_TYPE_LITLETTER x
#   endif
#else
#   if U_IS_BIG_ENDIAN
      
#     define U_ICUDATA_TYPE_LETTER "b"
#     define U_ICUDATA_TYPE_LITLETTER b
#   else
      
#     define U_ICUDATA_TYPE_LETTER "l"
#     define U_ICUDATA_TYPE_LITLETTER l
#   endif
#endif






#define U_ICUDATA_NAME    "icudt" U_ICU_VERSION_SHORT U_ICUDATA_TYPE_LETTER
#ifndef U_HIDE_INTERNAL_API
#define U_USRDATA_NAME    "usrdt" U_ICU_VERSION_SHORT U_ICUDATA_TYPE_LETTER  /**< @internal */
#define U_USE_USRDATA     1  /**< @internal */
#endif  












#define U_ICUDATA_ENTRY_POINT  U_DEF2_ICUDATA_ENTRY_POINT(U_ICU_VERSION_MAJOR_NUM,U_LIB_SUFFIX_C_NAME)

#ifndef U_HIDE_INTERNAL_API




#define U_DEF2_ICUDATA_ENTRY_POINT(major,suff) U_DEF_ICUDATA_ENTRY_POINT(major,suff)





#ifndef U_DEF_ICUDATA_ENTRY_POINT

#ifndef U_LIB_SUFFIX_C_NAME
#define U_DEF_ICUDATA_ENTRY_POINT(major, suff) icudt##major##_dat
#else
#define U_DEF_ICUDATA_ENTRY_POINT(major, suff) icudt##suff ## major##_dat
#endif
#endif
#endif  






#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif












typedef double UDate;


#define U_MILLIS_PER_SECOND        (1000)

#define U_MILLIS_PER_MINUTE       (60000)

#define U_MILLIS_PER_HOUR       (3600000)

#define U_MILLIS_PER_DAY       (86400000)




 
#define U_DATE_MAX DBL_MAX




 
#define U_DATE_MIN -U_DATE_MAX




































































#if defined(U_COMBINED_IMPLEMENTATION)
#define U_DATA_API     U_EXPORT
#define U_COMMON_API   U_EXPORT
#define U_I18N_API     U_EXPORT
#define U_LAYOUT_API   U_EXPORT
#define U_LAYOUTEX_API U_EXPORT
#define U_IO_API       U_EXPORT
#define U_TOOLUTIL_API U_EXPORT
#elif defined(U_STATIC_IMPLEMENTATION)
#define U_DATA_API
#define U_COMMON_API
#define U_I18N_API
#define U_LAYOUT_API
#define U_LAYOUTEX_API
#define U_IO_API
#define U_TOOLUTIL_API
#elif defined(U_COMMON_IMPLEMENTATION)
#define U_DATA_API     U_IMPORT
#define U_COMMON_API   U_EXPORT
#define U_I18N_API     U_IMPORT
#define U_LAYOUT_API   U_IMPORT
#define U_LAYOUTEX_API U_IMPORT
#define U_IO_API       U_IMPORT
#define U_TOOLUTIL_API U_IMPORT
#elif defined(U_I18N_IMPLEMENTATION)
#define U_DATA_API     U_IMPORT
#define U_COMMON_API   U_IMPORT
#define U_I18N_API     U_EXPORT
#define U_LAYOUT_API   U_IMPORT
#define U_LAYOUTEX_API U_IMPORT
#define U_IO_API       U_IMPORT
#define U_TOOLUTIL_API U_IMPORT
#elif defined(U_LAYOUT_IMPLEMENTATION)
#define U_DATA_API     U_IMPORT
#define U_COMMON_API   U_IMPORT
#define U_I18N_API     U_IMPORT
#define U_LAYOUT_API   U_EXPORT
#define U_LAYOUTEX_API U_IMPORT
#define U_IO_API       U_IMPORT
#define U_TOOLUTIL_API U_IMPORT
#elif defined(U_LAYOUTEX_IMPLEMENTATION)
#define U_DATA_API     U_IMPORT
#define U_COMMON_API   U_IMPORT
#define U_I18N_API     U_IMPORT
#define U_LAYOUT_API   U_IMPORT
#define U_LAYOUTEX_API U_EXPORT
#define U_IO_API       U_IMPORT
#define U_TOOLUTIL_API U_IMPORT
#elif defined(U_IO_IMPLEMENTATION)
#define U_DATA_API     U_IMPORT
#define U_COMMON_API   U_IMPORT
#define U_I18N_API     U_IMPORT
#define U_LAYOUT_API   U_IMPORT
#define U_LAYOUTEX_API U_IMPORT
#define U_IO_API       U_EXPORT
#define U_TOOLUTIL_API U_IMPORT
#elif defined(U_TOOLUTIL_IMPLEMENTATION)
#define U_DATA_API     U_IMPORT
#define U_COMMON_API   U_IMPORT
#define U_I18N_API     U_IMPORT
#define U_LAYOUT_API   U_IMPORT
#define U_LAYOUTEX_API U_IMPORT
#define U_IO_API       U_IMPORT
#define U_TOOLUTIL_API U_EXPORT
#else
#define U_DATA_API     U_IMPORT
#define U_COMMON_API   U_IMPORT
#define U_I18N_API     U_IMPORT
#define U_LAYOUT_API   U_IMPORT
#define U_LAYOUTEX_API U_IMPORT
#define U_IO_API       U_IMPORT
#define U_TOOLUTIL_API U_IMPORT
#endif






#ifdef __cplusplus
#define U_STANDARD_CPP_NAMESPACE        ::
#else
#define U_STANDARD_CPP_NAMESPACE
#endif



























#if defined(__cplusplus) && U_DEBUG && U_OVERRIDE_CXX_ALLOCATION && (_MSC_VER>=1200) && !defined(U_STATIC_IMPLEMENTATION) && (defined(U_COMMON_IMPLEMENTATION) || defined(U_I18N_IMPLEMENTATION) || defined(U_IO_IMPLEMENTATION) || defined(U_LAYOUT_IMPLEMENTATION) || defined(U_LAYOUTEX_IMPLEMENTATION))

#ifndef U_HIDE_INTERNAL_API





inline void *
operator new(size_t ) {
    char *q=NULL;
    *q=5; 
    return q;
}

#ifdef _Ret_bytecap_

_Ret_bytecap_(_Size)
#endif





inline void *
operator new[](size_t ) {
    char *q=NULL;
    *q=5; 
    return q;
}






inline void
operator delete(void * ) {
    char *q=NULL;
    *q=5; 
}






inline void
operator delete[](void * ) {
    char *q=NULL;
    *q=5; 
}

#endif 
#endif



















typedef enum UErrorCode {
    




    U_USING_FALLBACK_WARNING  = -128,   

    U_ERROR_WARNING_START     = -128,   

    U_USING_DEFAULT_WARNING   = -127,   

    U_SAFECLONE_ALLOCATED_WARNING = -126, 

    U_STATE_OLD_WARNING       = -125,   

    U_STRING_NOT_TERMINATED_WARNING = -124,

    U_SORT_KEY_TOO_SHORT_WARNING = -123, 

    U_AMBIGUOUS_ALIAS_WARNING = -122,   

    U_DIFFERENT_UCA_VERSION = -121,     
    
    U_PLUGIN_CHANGED_LEVEL_WARNING = -120, 

    U_ERROR_WARNING_LIMIT,              


    U_ZERO_ERROR              =  0,     

    U_ILLEGAL_ARGUMENT_ERROR  =  1,     
    U_MISSING_RESOURCE_ERROR  =  2,     
    U_INVALID_FORMAT_ERROR    =  3,     
    U_FILE_ACCESS_ERROR       =  4,     
    U_INTERNAL_PROGRAM_ERROR  =  5,     
    U_MESSAGE_PARSE_ERROR     =  6,     
    U_MEMORY_ALLOCATION_ERROR =  7,     
    U_INDEX_OUTOFBOUNDS_ERROR =  8,     
    U_PARSE_ERROR             =  9,     
    U_INVALID_CHAR_FOUND      = 10,     
    U_TRUNCATED_CHAR_FOUND    = 11,     
    U_ILLEGAL_CHAR_FOUND      = 12,     
    U_INVALID_TABLE_FORMAT    = 13,     
    U_INVALID_TABLE_FILE      = 14,     
    U_BUFFER_OVERFLOW_ERROR   = 15,     
    U_UNSUPPORTED_ERROR       = 16,     
    U_RESOURCE_TYPE_MISMATCH  = 17,     
    U_ILLEGAL_ESCAPE_SEQUENCE = 18,     
    U_UNSUPPORTED_ESCAPE_SEQUENCE = 19, 
    U_NO_SPACE_AVAILABLE      = 20,     
    U_CE_NOT_FOUND_ERROR      = 21,     
    U_PRIMARY_TOO_LONG_ERROR  = 22,     
    U_STATE_TOO_OLD_ERROR     = 23,     
    U_TOO_MANY_ALIASES_ERROR  = 24,     

    U_ENUM_OUT_OF_SYNC_ERROR  = 25,     
    U_INVARIANT_CONVERSION_ERROR = 26,  
    U_INVALID_STATE_ERROR     = 27,     
    U_COLLATOR_VERSION_MISMATCH = 28,   
    U_USELESS_COLLATOR_ERROR  = 29,     
    U_NO_WRITE_PERMISSION     = 30,     

    U_STANDARD_ERROR_LIMIT,             
    


    U_BAD_VARIABLE_DEFINITION=0x10000,
    U_PARSE_ERROR_START = 0x10000,    
    U_MALFORMED_RULE,                 
    U_MALFORMED_SET,                  
    U_MALFORMED_SYMBOL_REFERENCE,     
    U_MALFORMED_UNICODE_ESCAPE,       
    U_MALFORMED_VARIABLE_DEFINITION,  
    U_MALFORMED_VARIABLE_REFERENCE,   
    U_MISMATCHED_SEGMENT_DELIMITERS,  
    U_MISPLACED_ANCHOR_START,         
    U_MISPLACED_CURSOR_OFFSET,        
    U_MISPLACED_QUANTIFIER,           
    U_MISSING_OPERATOR,               
    U_MISSING_SEGMENT_CLOSE,          
    U_MULTIPLE_ANTE_CONTEXTS,         
    U_MULTIPLE_CURSORS,               
    U_MULTIPLE_POST_CONTEXTS,         
    U_TRAILING_BACKSLASH,             
    U_UNDEFINED_SEGMENT_REFERENCE,    
    U_UNDEFINED_VARIABLE,             
    U_UNQUOTED_SPECIAL,               
    U_UNTERMINATED_QUOTE,             
    U_RULE_MASK_ERROR,                
    U_MISPLACED_COMPOUND_FILTER,      
    U_MULTIPLE_COMPOUND_FILTERS,      
    U_INVALID_RBT_SYNTAX,             
    U_INVALID_PROPERTY_PATTERN,       
    U_MALFORMED_PRAGMA,               
    U_UNCLOSED_SEGMENT,               
    U_ILLEGAL_CHAR_IN_SEGMENT,        
    U_VARIABLE_RANGE_EXHAUSTED,       
    U_VARIABLE_RANGE_OVERLAP,         
    U_ILLEGAL_CHARACTER,              
    U_INTERNAL_TRANSLITERATOR_ERROR,  
    U_INVALID_ID,                     
    U_INVALID_FUNCTION,               
    U_PARSE_ERROR_LIMIT,              

    


    U_UNEXPECTED_TOKEN=0x10100,       
    U_FMT_PARSE_ERROR_START=0x10100,  
    U_MULTIPLE_DECIMAL_SEPARATORS,    
    U_MULTIPLE_DECIMAL_SEPERATORS = U_MULTIPLE_DECIMAL_SEPARATORS, 
    U_MULTIPLE_EXPONENTIAL_SYMBOLS,   
    U_MALFORMED_EXPONENTIAL_PATTERN,  
    U_MULTIPLE_PERCENT_SYMBOLS,       
    U_MULTIPLE_PERMILL_SYMBOLS,       
    U_MULTIPLE_PAD_SPECIFIERS,        
    U_PATTERN_SYNTAX_ERROR,           
    U_ILLEGAL_PAD_POSITION,           
    U_UNMATCHED_BRACES,               
    U_UNSUPPORTED_PROPERTY,           
    U_UNSUPPORTED_ATTRIBUTE,          
    U_ARGUMENT_TYPE_MISMATCH,         
    U_DUPLICATE_KEYWORD,              
    U_UNDEFINED_KEYWORD,              
    U_DEFAULT_KEYWORD_MISSING,        
    U_DECIMAL_NUMBER_SYNTAX_ERROR,    
    U_FORMAT_INEXACT_ERROR,           
    U_FMT_PARSE_ERROR_LIMIT,          

    


    U_BRK_INTERNAL_ERROR=0x10200,          
    U_BRK_ERROR_START=0x10200,             
    U_BRK_HEX_DIGITS_EXPECTED,             
    U_BRK_SEMICOLON_EXPECTED,              
    U_BRK_RULE_SYNTAX,                     
    U_BRK_UNCLOSED_SET,                    
    U_BRK_ASSIGN_ERROR,                    
    U_BRK_VARIABLE_REDFINITION,            
    U_BRK_MISMATCHED_PAREN,                
    U_BRK_NEW_LINE_IN_QUOTED_STRING,       
    U_BRK_UNDEFINED_VARIABLE,              
    U_BRK_INIT_ERROR,                      
    U_BRK_RULE_EMPTY_SET,                  
    U_BRK_UNRECOGNIZED_OPTION,             
    U_BRK_MALFORMED_RULE_TAG,              
    U_BRK_ERROR_LIMIT,                     

    


    U_REGEX_INTERNAL_ERROR=0x10300,       
    U_REGEX_ERROR_START=0x10300,          
    U_REGEX_RULE_SYNTAX,                  
    U_REGEX_INVALID_STATE,                
    U_REGEX_BAD_ESCAPE_SEQUENCE,          
    U_REGEX_PROPERTY_SYNTAX,              
    U_REGEX_UNIMPLEMENTED,                
    U_REGEX_MISMATCHED_PAREN,             
    U_REGEX_NUMBER_TOO_BIG,               
    U_REGEX_BAD_INTERVAL,                 
    U_REGEX_MAX_LT_MIN,                   
    U_REGEX_INVALID_BACK_REF,             
    U_REGEX_INVALID_FLAG,                 
    U_REGEX_LOOK_BEHIND_LIMIT,            
    U_REGEX_SET_CONTAINS_STRING,          
#ifndef U_HIDE_DEPRECATED_API
    U_REGEX_OCTAL_TOO_BIG,                
#endif  
    U_REGEX_MISSING_CLOSE_BRACKET=U_REGEX_SET_CONTAINS_STRING+2, 
    U_REGEX_INVALID_RANGE,                
    U_REGEX_STACK_OVERFLOW,               
    U_REGEX_TIME_OUT,                     
    U_REGEX_STOPPED_BY_CALLER,            
#ifndef U_HIDE_DRAFT_API
    U_REGEX_PATTERN_TOO_BIG,              
    U_REGEX_INVALID_CAPTURE_GROUP_NAME,   
#endif  
    U_REGEX_ERROR_LIMIT=U_REGEX_STOPPED_BY_CALLER+3, 

    


    U_IDNA_PROHIBITED_ERROR=0x10400,
    U_IDNA_ERROR_START=0x10400,
    U_IDNA_UNASSIGNED_ERROR,
    U_IDNA_CHECK_BIDI_ERROR,
    U_IDNA_STD3_ASCII_RULES_ERROR,
    U_IDNA_ACE_PREFIX_ERROR,
    U_IDNA_VERIFICATION_ERROR,
    U_IDNA_LABEL_TOO_LONG_ERROR,
    U_IDNA_ZERO_LENGTH_LABEL_ERROR,
    U_IDNA_DOMAIN_NAME_TOO_LONG_ERROR,
    U_IDNA_ERROR_LIMIT,
    


    U_STRINGPREP_PROHIBITED_ERROR = U_IDNA_PROHIBITED_ERROR,
    U_STRINGPREP_UNASSIGNED_ERROR = U_IDNA_UNASSIGNED_ERROR,
    U_STRINGPREP_CHECK_BIDI_ERROR = U_IDNA_CHECK_BIDI_ERROR,
    
    


    U_PLUGIN_ERROR_START=0x10500,         
    U_PLUGIN_TOO_HIGH=0x10500,            
    U_PLUGIN_DIDNT_SET_LEVEL,             
    U_PLUGIN_ERROR_LIMIT,                 

    U_ERROR_LIMIT=U_PLUGIN_ERROR_LIMIT      
} UErrorCode;




#ifdef __cplusplus
    



    static
    inline UBool U_SUCCESS(UErrorCode code) { return (UBool)(code<=U_ZERO_ERROR); }
    



    static
    inline UBool U_FAILURE(UErrorCode code) { return (UBool)(code>U_ZERO_ERROR); }
#else
    



#   define U_SUCCESS(x) ((x)<=U_ZERO_ERROR)
    



#   define U_FAILURE(x) ((x)>U_ZERO_ERROR)
#endif







U_STABLE const char * U_EXPORT2
u_errorName(UErrorCode code);


#endif 

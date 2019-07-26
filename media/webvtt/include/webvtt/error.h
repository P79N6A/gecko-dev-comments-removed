


























#ifndef __WEBVTT_ERROR_H__
# define __WEBVTT_ERROR_H__
# include "util.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

  enum
  webvtt_error_t
  {
    
    WEBVTT_ALLOCATION_FAILED = 0,
    



    WEBVTT_MALFORMED_TAG,
    
    WEBVTT_EXPECTED_EOL,
    
    WEBVTT_EXPECTED_WHITESPACE,
    



    WEBVTT_UNEXPECTED_WHITESPACE,
    
    WEBVTT_LONG_COMMENT,
    
    WEBVTT_ID_TRUNCATED,
    
    WEBVTT_MALFORMED_TIMESTAMP,
    
    WEBVTT_EXPECTED_TIMESTAMP,
    
    WEBVTT_MISSING_CUETIME_SEPARATOR,
    

    WEBVTT_EXPECTED_CUETIME_SEPARATOR,
    
    WEBVTT_MISSING_CUESETTING_DELIMITER,
    
    WEBVTT_INVALID_CUESETTING_DELIMITER,
    
    WEBVTT_INVALID_ENDTIME,
    
    WEBVTT_INVALID_CUESETTING,
    
    WEBVTT_UNFINISHED_CUETIMES,
    
    WEBVTT_MISSING_CUESETTING_KEYWORD,
    
    WEBVTT_VERTICAL_ALREADY_SET,
    
    WEBVTT_VERTICAL_BAD_VALUE,
    
    WEBVTT_LINE_ALREADY_SET,
    
    WEBVTT_LINE_BAD_VALUE,
    
    WEBVTT_POSITION_ALREADY_SET,
    
    WEBVTT_POSITION_BAD_VALUE,
    
    WEBVTT_SIZE_ALREADY_SET,
    
    WEBVTT_SIZE_BAD_VALUE,
    
    WEBVTT_ALIGN_ALREADY_SET,
    
    WEBVTT_ALIGN_BAD_VALUE,
    
    WEBVTT_CUE_CONTAINS_SEPARATOR,
    
    WEBVTT_CUE_INCOMPLETE,
  };
  typedef enum webvtt_error_t webvtt_error;

  WEBVTT_EXPORT const char *webvtt_strerror( webvtt_error );

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif

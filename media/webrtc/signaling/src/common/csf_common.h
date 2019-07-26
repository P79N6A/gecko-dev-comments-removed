






































#ifndef _CSF_COMMON_E58E5677_950A_424c_B6C2_CA180092E6A2_H
#define _CSF_COMMON_E58E5677_950A_424c_B6C2_CA180092E6A2_H

#include <assert.h>
#include <memory>
#include <vector>
#include <stdlib.h>























#ifdef WIN32
    #if !defined(_countof)
        #if !defined(__cplusplus)
            #define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
        #else
            extern "C++"
            {
            template <typename _CountofType, size_t _SizeOfArray>
            char (*_csf_countof_helper(_CountofType (&_Array)[_SizeOfArray]))[_SizeOfArray];
            #define _countof(_Array) sizeof(*_csf_countof_helper(_Array))
            }
        #endif
    #endif
#else
    #define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#endif


#define csf_countof(anArray) _countof(anArray)



#ifdef _WIN32
  
  #define csf_sprintf( buffer,  sizeOfBufferInCharsInclNullTerm,  format, ...)\
    _snprintf_s (buffer, sizeOfBufferInCharsInclNullTerm, _TRUNCATE, format, __VA_ARGS__)
#else
  #define csf_sprintf( buffer,  sizeOfBufferInCharsInclNullTerm,  format, ...)\
    snprintf (buffer, sizeOfBufferInCharsInclNullTerm, format, __VA_ARGS__);\
    buffer[sizeOfBufferInCharsInclNullTerm-1] = '\0'
#endif



#ifdef _WIN32
  #define csf_vsprintf( buffer,  sizeOfBufferInCharsInclNullTerm,  format,  vaList)\
    vsnprintf_s (buffer, sizeOfBufferInCharsInclNullTerm, _TRUNCATE, format, vaList);\
    buffer[sizeOfBufferInCharsInclNullTerm-1] = '\0'
#else
  #define csf_vsprintf( buffer,  sizeOfBufferInCharsInclNullTerm,  format,  vaList)\
    vsprintf (buffer, format, vaList);\
    buffer[sizeOfBufferInCharsInclNullTerm-1] = '\0'
#endif

#endif
















































#ifndef OMX_Types_h
#define OMX_Types_h

#ifdef __cplusplus
extern "C" {
#endif




#ifdef __SYMBIAN32__   
#   ifdef __OMX_EXPORTS
#       define OMX_API __declspec(dllexport)
#   else
#       ifdef _WIN32
#           define OMX_API __declspec(dllexport) 
#       else
#           define OMX_API __declspec(dllimport)
#       endif
#   endif
#else
#   ifdef _WIN32
#      ifdef __OMX_EXPORTS
#          define OMX_API __declspec(dllexport)
#      else

#define OMX_API
#      endif
#   else
#      ifdef __OMX_EXPORTS
#          define OMX_API
#      else
#          define OMX_API extern
#      endif
#   endif
#endif

#ifndef OMX_APIENTRY
#define OMX_APIENTRY 
#endif 




#ifndef OMX_IN
#define OMX_IN
#endif




#ifndef OMX_OUT
#define OMX_OUT
#endif






#ifndef OMX_INOUT
#define OMX_INOUT
#endif






#define OMX_ALL 0xFFFFFFFF






 
 


 







  




 



 
 

 


typedef unsigned char OMX_U8;


typedef signed char OMX_S8;


typedef unsigned short OMX_U16;


typedef signed short OMX_S16;


typedef unsigned long OMX_U32;


typedef signed long OMX_S32;








#ifndef OMX_SKIP64BIT
#ifdef __SYMBIAN32__

typedef unsigned long long OMX_U64;


typedef signed long long OMX_S64;

#elif defined(WIN32)

   
typedef unsigned __int64  OMX_U64;


typedef signed   __int64  OMX_S64;

#else 


typedef unsigned long long OMX_U64;


typedef signed long long OMX_S64;

#endif 
#endif






typedef enum OMX_BOOL {
    OMX_FALSE = 0,
    OMX_TRUE = !OMX_FALSE,
    OMX_BOOL_MAX = 0x7FFFFFFF
} OMX_BOOL; 
 




typedef void* OMX_PTR;






typedef char* OMX_STRING;






typedef unsigned char* OMX_BYTE;





typedef unsigned char OMX_UUIDTYPE[128];




typedef enum OMX_DIRTYPE
{
    OMX_DirInput,              
    OMX_DirOutput,             
    OMX_DirMax = 0x7FFFFFFF
} OMX_DIRTYPE;




typedef enum OMX_ENDIANTYPE
{
    OMX_EndianBig, 
    OMX_EndianLittle, 
    OMX_EndianMax = 0x7FFFFFFF
} OMX_ENDIANTYPE;





typedef enum OMX_NUMERICALDATATYPE
{
    OMX_NumericalDataSigned, 
    OMX_NumericalDataUnsigned, 
    OMX_NumercialDataMax = 0x7FFFFFFF
} OMX_NUMERICALDATATYPE;



typedef struct OMX_BU32 {
    OMX_U32 nValue; 
    OMX_U32 nMin;   
    OMX_U32 nMax;   
} OMX_BU32;



typedef struct OMX_BS32 {
    OMX_S32 nValue; 
    OMX_S32 nMin;   
    OMX_S32 nMax;   
} OMX_BS32;














#ifndef OMX_SKIP64BIT
typedef OMX_S64 OMX_TICKS;
#else
typedef struct OMX_TICKS
{
    OMX_U32 nLowPart;    
    OMX_U32 nHighPart;   
} OMX_TICKS;
#endif
#define OMX_TICKS_PER_SECOND 1000000




typedef void* OMX_HANDLETYPE;

typedef struct OMX_MARKTYPE
{
    OMX_HANDLETYPE hMarkTargetComponent;   


    OMX_PTR pMarkData;   


} OMX_MARKTYPE;





typedef void* OMX_NATIVE_DEVICETYPE;



typedef void* OMX_NATIVE_WINDOWTYPE;












typedef union OMX_VERSIONTYPE
{
    struct
    {
        OMX_U8 nVersionMajor;   
        OMX_U8 nVersionMinor;   
        OMX_U8 nRevision;       
        OMX_U8 nStep;           
    } s;
    OMX_U32 nVersion;           


} OMX_VERSIONTYPE;

#ifdef __cplusplus
}
#endif 

#endif


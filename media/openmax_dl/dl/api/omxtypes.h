






























  
#ifndef _OMXTYPES_H_
#define _OMXTYPES_H_

#include <limits.h> 

#define OMX_IN
#define OMX_OUT
#define OMX_INOUT


typedef enum {
    
    
    OMX_Sts_NoErr                    =  0,    
    OMX_Sts_Err                      = -2,        
    OMX_Sts_InvalidBitstreamValErr   = -182,      
    OMX_Sts_MemAllocErr              = -9,    
    OMX_StsACAAC_GainCtrErr    	     = -159,  
    OMX_StsACAAC_PrgNumErr           = -167,  
    OMX_StsACAAC_CoefValErr          = -163,       
    OMX_StsACAAC_MaxSfbErr           = -162,      
	OMX_StsACAAC_PlsDataErr		     = -160,  

    
    OMX_Sts_BadArgErr                = -5,    

    OMX_StsACAAC_TnsNumFiltErr       = -157,  
    OMX_StsACAAC_TnsLenErr           = -156,     
    OMX_StsACAAC_TnsOrderErr         = -155,                    
    OMX_StsACAAC_TnsCoefResErr       = -154,  
    OMX_StsACAAC_TnsCoefErr          = -153,                    
    OMX_StsACAAC_TnsDirectErr        = -152,    

    OMX_StsICJP_JPEGMarkerErr        = -183,  
                                              
    OMX_StsICJP_JPEGMarker           = -181,  
                                              
    OMX_StsIPPP_ContextMatchErr      = -17,   

    OMX_StsSP_EvenMedianMaskSizeErr  = -180,  

    OMX_Sts_MaximumEnumeration       = INT_MAX  
    
 } OMXResult;          

 

#if UCHAR_MAX == 0xff
typedef unsigned char OMX_U8;
#elif USHRT_MAX == 0xff 
typedef unsigned short int OMX_U8; 
#else
#error OMX_U8 undefined
#endif 

 

#if SCHAR_MAX == 0x7f 
typedef signed char OMX_S8;
#elif SHRT_MAX == 0x7f 
typedef signed short int OMX_S8; 
#else
#error OMX_S8 undefined
#endif
 
 

#if USHRT_MAX == 0xffff
typedef unsigned short int OMX_U16;
#elif UINT_MAX == 0xffff
typedef unsigned int OMX_U16; 
#else
#error OMX_U16 undefined
#endif



#if SHRT_MAX == 0x7fff 
typedef signed short int OMX_S16;
#elif INT_MAX == 0x7fff 
typedef signed int OMX_S16; 
#else
#error OMX_S16 undefined
#endif



#if UINT_MAX == 0xffffffff
typedef unsigned int OMX_U32;
#elif LONG_MAX == 0xffffffff
typedef unsigned long int OMX_U32; 
#else
#error OMX_U32 undefined
#endif



#if INT_MAX == 0x7fffffff
typedef signed int OMX_S32;
#elif LONG_MAX == 0x7fffffff
typedef long signed int OMX_S32; 
#else
#error OMX_S32 undefined
#endif



#if defined( _WIN32 ) || defined ( _WIN64 )
    typedef __int64 OMX_S64; 
    typedef unsigned __int64 OMX_U64; 
    #define OMX_MIN_S64			(0x8000000000000000i64)
    #define OMX_MIN_U64			(0x0000000000000000i64)
    #define OMX_MAX_S64			(0x7FFFFFFFFFFFFFFFi64)
    #define OMX_MAX_U64			(0xFFFFFFFFFFFFFFFFi64)
#else
    typedef long long OMX_S64; 
    typedef unsigned long long OMX_U64; 
    #define OMX_MIN_S64			(0x8000000000000000LL)
    #define OMX_MIN_U64			(0x0000000000000000LL)
    #define OMX_MAX_S64			(0x7FFFFFFFFFFFFFFFLL)
    #define OMX_MAX_U64			(0xFFFFFFFFFFFFFFFFLL)
#endif



typedef struct
{
  OMX_S8 Re; 
  OMX_S8 Im; 	
	
} OMX_SC8; 



typedef struct
{
  OMX_S16 Re; 
  OMX_S16 Im; 	
	
} OMX_SC16; 



typedef struct
{
  OMX_S32 Re; 
  OMX_S32 Im; 	
	
} OMX_SC32; 



typedef struct
{
  OMX_S64 Re; 
  OMX_S64 Im; 	
	
} OMX_SC64; 



typedef float OMX_F32; 



typedef double OMX_F64; 



typedef int OMX_INT; 


#define OMX_MIN_S8  	   	(-128)
#define OMX_MIN_U8  		0
#define OMX_MIN_S16		 	(-32768)
#define OMX_MIN_U16			0
#define OMX_MIN_S32			(-2147483647-1)
#define OMX_MIN_U32			0

#define OMX_MAX_S8			(127)
#define OMX_MAX_U8			(255)
#define OMX_MAX_S16			(32767)
#define OMX_MAX_U16			(0xFFFF)
#define OMX_MAX_S32			(2147483647)
#define OMX_MAX_U32			(0xFFFFFFFF)

typedef void OMXVoid;

#ifndef NULL
#define NULL ((void*)0)
#endif





typedef struct {
	OMX_INT x;      
	OMX_INT y;      
	OMX_INT width;  
	OMX_INT height; 
}OMXRect;



typedef struct 
{
 OMX_INT x; 
 OMX_INT y;	
	
} OMXPoint;



typedef struct 
{
 OMX_INT width;  
 OMX_INT height; 
	
} OMXSize;

#endif 

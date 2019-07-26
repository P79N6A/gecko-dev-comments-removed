



























#ifndef OMX_IndexExt_h
#define OMX_IndexExt_h

#ifdef __cplusplus
extern "C" {
#endif





#include <OMX_Index.h>






typedef enum OMX_INDEXEXTTYPE {

    
    OMX_IndexExtComponentStartUnused = OMX_IndexKhronosExtensions + 0x00100000,
    OMX_IndexConfigCallbackRequest,                 
    OMX_IndexConfigCommitMode,                      
    OMX_IndexConfigCommit,                          

    
    OMX_IndexExtPortStartUnused = OMX_IndexKhronosExtensions + 0x00200000,

    
    OMX_IndexExtAudioStartUnused = OMX_IndexKhronosExtensions + 0x00400000,

    
    OMX_IndexExtImageStartUnused = OMX_IndexKhronosExtensions + 0x00500000,

    
    OMX_IndexExtVideoStartUnused = OMX_IndexKhronosExtensions + 0x00600000,
    OMX_IndexParamNalStreamFormatSupported,         
    OMX_IndexParamNalStreamFormat,                  
    OMX_IndexParamNalStreamFormatSelect,            
    OMX_IndexParamVideoVp8,                         
    OMX_IndexConfigVideoVp8ReferenceFrame,          
    OMX_IndexConfigVideoVp8ReferenceFrameType,      

    
    OMX_IndexExtCommonStartUnused = OMX_IndexKhronosExtensions + 0x00700000,

    
    OMX_IndexExtOtherStartUnused = OMX_IndexKhronosExtensions + 0x00800000,

    
    OMX_IndexExtTimeStartUnused = OMX_IndexKhronosExtensions + 0x00900000,

    OMX_IndexExtMax = 0x7FFFFFFF
} OMX_INDEXEXTTYPE;

#ifdef __cplusplus
}
#endif 

#endif


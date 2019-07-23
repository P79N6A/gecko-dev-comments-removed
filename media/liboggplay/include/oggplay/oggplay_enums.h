







































 
#ifndef __OGGPLAY_ENUMS_H__
#define __OGGPLAY_ENUMS_H__




typedef enum OggPlayErrorCode {
  E_OGGPLAY_CONTINUE            = 1,
  E_OGGPLAY_OK                  = 0,    
  E_OGGPLAY_BAD_OGGPLAY         = -1,   
  E_OGGPLAY_BAD_READER          = -2,   
  E_OGGPLAY_BAD_INPUT           = -3,   
  E_OGGPLAY_NO_SUCH_CHUNK       = -4,
  E_OGGPLAY_BAD_TRACK           = -5,   
  E_OGGPLAY_TRACK_IS_SKELETON   = -6,   
  E_OGGPLAY_OGGZ_UNHAPPY        = -7,   
  E_OGGPLAY_END_OF_FILE         = -8,   
  E_OGGPLAY_TRACK_IS_OVER       = -9,   
  E_OGGPLAY_BAD_CALLBACK_INFO   = -10,  
  E_OGGPLAY_WRONG_TRACK_TYPE    = -11,  
  E_OGGPLAY_UNINITIALISED       = -12,  
  E_OGGPLAY_CALLBACK_MODE       = -13,  
  E_OGGPLAY_BUFFER_MODE         = -14,  
  E_OGGPLAY_USER_INTERRUPT      = -15,  
  E_OGGPLAY_SOCKET_ERROR        = -16,  
  E_OGGPLAY_TIMEOUT             = -17,  
  E_OGGPLAY_CANT_SEEK           = -18,  
  E_OGGPLAY_NO_KATE_SUPPORT     = -19,  
  E_OGGPLAY_NO_TIGER_SUPPORT    = -20,  
  E_OGGPLAY_OUT_OF_MEMORY       = -21,  
  E_OGGPLAY_TYPE_OVERFLOW       = -22,  
  E_OGGPLAY_TRACK_IS_UNKNOWN    = -23,  
  E_OGGPLAY_TRACK_UNINITIALISED = -24,  
  E_OGGPLAY_NOTCHICKENPAYBACK   = -777
} OggPlayErrorCode;




typedef enum OggPlayDataType {
  OGGPLAY_INACTIVE      = -1,   
  OGGPLAY_YUV_VIDEO     = 0,    
  OGGPLAY_RGBA_VIDEO    = 1,    
  OGGPLAY_SHORTS_AUDIO  = 1000, 
  OGGPLAY_FLOATS_AUDIO  = 1001, 
  OGGPLAY_CMML          = 2000, 
  OGGPLAY_KATE          = 3000, 
  OGGPLAY_TYPE_UNKNOWN  = 9000  
} OggPlayDataType;




typedef enum OggPlayStreamInfo {
  OGGPLAY_STREAM_UNINITIALISED = 0, 
  OGGPLAY_STREAM_FIRST_DATA = 1,    
  OGGPLAY_STREAM_INITIALISED = 2,   
  OGGPLAY_STREAM_LAST_DATA = 3,     
  OGGPLAY_STREAM_JUST_SEEKED = 4    
} OggPlayStreamInfo;

#endif

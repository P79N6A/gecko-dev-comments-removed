












































#ifndef OMX_Core_h
#define OMX_Core_h

#ifdef __cplusplus
extern "C" {
#endif







#include <OMX_Index.h>






typedef enum OMX_COMMANDTYPE
{
    OMX_CommandStateSet,    
    OMX_CommandFlush,       
    OMX_CommandPortDisable, 
    OMX_CommandPortEnable,  
    OMX_CommandMarkBuffer,  
    OMX_CommandKhronosExtensions = 0x6F000000,  
    OMX_CommandVendorStartUnused = 0x7F000000, 
    OMX_CommandMax = 0X7FFFFFFF
} OMX_COMMANDTYPE;
































typedef enum OMX_STATETYPE
{
    OMX_StateInvalid,      


    OMX_StateLoaded,      




    OMX_StateIdle,        


    OMX_StateExecuting,   

    OMX_StatePause,       
    OMX_StateWaitForResources, 


    OMX_StateKhronosExtensions = 0x6F000000,  
    OMX_StateVendorStartUnused = 0x7F000000, 
    OMX_StateMax = 0X7FFFFFFF
} OMX_STATETYPE;











typedef enum OMX_ERRORTYPE
{
  OMX_ErrorNone = 0,

  
  OMX_ErrorInsufficientResources = (OMX_S32) 0x80001000,

  
  OMX_ErrorUndefined = (OMX_S32) 0x80001001,

  
  OMX_ErrorInvalidComponentName = (OMX_S32) 0x80001002,

  
  OMX_ErrorComponentNotFound = (OMX_S32) 0x80001003,

  

  OMX_ErrorInvalidComponent = (OMX_S32) 0x80001004,

  
  OMX_ErrorBadParameter = (OMX_S32) 0x80001005,

  
  OMX_ErrorNotImplemented = (OMX_S32) 0x80001006,

  
  OMX_ErrorUnderflow = (OMX_S32) 0x80001007,

  
  OMX_ErrorOverflow = (OMX_S32) 0x80001008,

  
  OMX_ErrorHardware = (OMX_S32) 0x80001009,

  
  OMX_ErrorInvalidState = (OMX_S32) 0x8000100A,

  
  OMX_ErrorStreamCorrupt = (OMX_S32) 0x8000100B,

  
  OMX_ErrorPortsNotCompatible = (OMX_S32) 0x8000100C,

  

  OMX_ErrorResourcesLost = (OMX_S32) 0x8000100D,

  
  OMX_ErrorNoMore = (OMX_S32) 0x8000100E,

  
  OMX_ErrorVersionMismatch = (OMX_S32) 0x8000100F,

  
  OMX_ErrorNotReady = (OMX_S32) 0x80001010,

  
  OMX_ErrorTimeout = (OMX_S32) 0x80001011,

  
  OMX_ErrorSameState = (OMX_S32) 0x80001012,

  

  OMX_ErrorResourcesPreempted = (OMX_S32) 0x80001013, 

  



  OMX_ErrorPortUnresponsiveDuringAllocation = (OMX_S32) 0x80001014,

  



  OMX_ErrorPortUnresponsiveDuringDeallocation = (OMX_S32) 0x80001015,

  



  OMX_ErrorPortUnresponsiveDuringStop = (OMX_S32) 0x80001016,

  
  OMX_ErrorIncorrectStateTransition = (OMX_S32) 0x80001017,

  
  OMX_ErrorIncorrectStateOperation = (OMX_S32) 0x80001018, 

  
  OMX_ErrorUnsupportedSetting = (OMX_S32) 0x80001019,

  
  OMX_ErrorUnsupportedIndex = (OMX_S32) 0x8000101A,

  
  OMX_ErrorBadPortIndex = (OMX_S32) 0x8000101B,

  
  OMX_ErrorPortUnpopulated = (OMX_S32) 0x8000101C,

  
  OMX_ErrorComponentSuspended = (OMX_S32) 0x8000101D,

  
  OMX_ErrorDynamicResourcesUnavailable = (OMX_S32) 0x8000101E,

  

  OMX_ErrorMbErrorsInFrame = (OMX_S32) 0x8000101F,

  
  OMX_ErrorFormatNotDetected = (OMX_S32) 0x80001020, 

  
  OMX_ErrorContentPipeOpenFailed = (OMX_S32) 0x80001021,

  
  OMX_ErrorContentPipeCreationFailed = (OMX_S32) 0x80001022,

  
  OMX_ErrorSeperateTablesUsed = (OMX_S32) 0x80001023,

  
  OMX_ErrorTunnelingUnsupported = (OMX_S32) 0x80001024,

  OMX_ErrorKhronosExtensions = (OMX_S32)0x8F000000,  
  OMX_ErrorVendorStartUnused = (OMX_S32)0x90000000, 
  OMX_ErrorMax = 0x7FFFFFFF
} OMX_ERRORTYPE;


typedef OMX_ERRORTYPE (* OMX_COMPONENTINITTYPE)(OMX_IN  OMX_HANDLETYPE hComponent);


typedef struct OMX_COMPONENTREGISTERTYPE
{
  const char          * pName;       
  OMX_COMPONENTINITTYPE pInitialize; 
} OMX_COMPONENTREGISTERTYPE;


extern OMX_COMPONENTREGISTERTYPE OMX_ComponentRegistered[];


typedef struct OMX_PRIORITYMGMTTYPE {
 OMX_U32 nSize;             
 OMX_VERSIONTYPE nVersion;  
 OMX_U32 nGroupPriority;            
 OMX_U32 nGroupID;                  
} OMX_PRIORITYMGMTTYPE;


#define OMX_MAX_STRINGNAME_SIZE 128


typedef struct OMX_PARAM_COMPONENTROLETYPE {
    OMX_U32 nSize;              
    OMX_VERSIONTYPE nVersion;   
    OMX_U8 cRole[OMX_MAX_STRINGNAME_SIZE];  
} OMX_PARAM_COMPONENTROLETYPE;










#define OMX_BUFFERFLAG_EOS 0x00000001 


























#define OMX_BUFFERFLAG_STARTTIME 0x00000002

 




















#define OMX_BUFFERFLAG_DECODEONLY 0x00000004






#define OMX_BUFFERFLAG_DATACORRUPT 0x00000008






#define OMX_BUFFERFLAG_ENDOFFRAME 0x00000010





#define OMX_BUFFERFLAG_SYNCFRAME 0x00000020





#define OMX_BUFFERFLAG_EXTRADATA 0x00000040

















#define OMX_BUFFERFLAG_CODECCONFIG 0x00000080





#define OMX_BUFFERFLAG_TFF 0x00010000





#define OMX_BUFFERFLAG_BFF 0x00020000


typedef struct OMX_BUFFERHEADERTYPE
{
    OMX_U32 nSize;              
    OMX_VERSIONTYPE nVersion;   
    OMX_U8* pBuffer;            

    OMX_U32 nAllocLen;          
    OMX_U32 nFilledLen;         

    OMX_U32 nOffset;            

    OMX_PTR pAppPrivate;        

    OMX_PTR pPlatformPrivate;   
 
    OMX_PTR pInputPortPrivate;  

    OMX_PTR pOutputPortPrivate; 

    OMX_HANDLETYPE hMarkTargetComponent; 

    OMX_PTR pMarkData;          


    OMX_U32 nTickCount;         








 OMX_TICKS nTimeStamp;          






  OMX_U32     nFlags;           
  OMX_U32 nOutputPortIndex;     

  OMX_U32 nInputPortIndex;      

} OMX_BUFFERHEADERTYPE;







typedef enum OMX_EXTRADATATYPE
{
   OMX_ExtraDataNone = 0,                               
   OMX_ExtraDataQuantization,                   
   OMX_ExtraDataKhronosExtensions = 0x6F000000,  
   OMX_ExtraDataVendorStartUnused = 0x7F000000, 
   OMX_ExtraDataMax = 0x7FFFFFFF
} OMX_EXTRADATATYPE;


typedef struct OMX_OTHER_EXTRADATATYPE  {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;               
    OMX_U32 nPortIndex;
    OMX_EXTRADATATYPE eType;       
    OMX_U32 nDataSize;   
    OMX_U8  data[1];     
} OMX_OTHER_EXTRADATATYPE;


typedef struct OMX_PORT_PARAM_TYPE {
    OMX_U32 nSize;              
    OMX_VERSIONTYPE nVersion;   
    OMX_U32 nPorts;             
    OMX_U32 nStartPortNumber;   
} OMX_PORT_PARAM_TYPE; 


typedef enum OMX_EVENTTYPE
{
    OMX_EventCmdComplete,         
    OMX_EventError,               
    OMX_EventMark,                
    OMX_EventPortSettingsChanged, 
    OMX_EventBufferFlag,           
    OMX_EventResourcesAcquired,   


   OMX_EventComponentResumed,     
   OMX_EventDynamicResourcesAvailable, 
   OMX_EventPortFormatDetected,      
   OMX_EventKhronosExtensions = 0x6F000000,  
   OMX_EventVendorStartUnused = 0x7F000000, 
   OMX_EventMax = 0x7FFFFFFF
} OMX_EVENTTYPE;

typedef struct OMX_CALLBACKTYPE
{
    






























   OMX_ERRORTYPE (*EventHandler)(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_EVENTTYPE eEvent,
        OMX_IN OMX_U32 nData1,
        OMX_IN OMX_U32 nData2,
        OMX_IN OMX_PTR pEventData);

    





















    OMX_ERRORTYPE (*EmptyBufferDone)(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);

    























    OMX_ERRORTYPE (*FillBufferDone)(
        OMX_OUT OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_PTR pAppData,
        OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer);

} OMX_CALLBACKTYPE;





typedef enum OMX_BUFFERSUPPLIERTYPE
{
    OMX_BufferSupplyUnspecified = 0x0, 

    OMX_BufferSupplyInput,             
    OMX_BufferSupplyOutput,            
    OMX_BufferSupplyKhronosExtensions = 0x6F000000,  
    OMX_BufferSupplyVendorStartUnused = 0x7F000000, 
    OMX_BufferSupplyMax = 0x7FFFFFFF
} OMX_BUFFERSUPPLIERTYPE;





typedef struct OMX_PARAM_BUFFERSUPPLIERTYPE {
    OMX_U32 nSize; 
    OMX_VERSIONTYPE nVersion; 
    OMX_U32 nPortIndex; 
    OMX_BUFFERSUPPLIERTYPE eBufferSupplier; 
} OMX_PARAM_BUFFERSUPPLIERTYPE;






#define OMX_PORTTUNNELFLAG_READONLY 0x00000001 






   
typedef struct OMX_TUNNELSETUPTYPE
{
    OMX_U32 nTunnelFlags;             
    OMX_BUFFERSUPPLIERTYPE eSupplier; 
} OMX_TUNNELSETUPTYPE; 












































#define OMX_GetComponentVersion(                            \
        hComponent,                                         \
        pComponentName,                                     \
        pComponentVersion,                                  \
        pSpecVersion,                                       \
        pComponentUUID)                                     \
    ((OMX_COMPONENTTYPE*)hComponent)->GetComponentVersion(  \
        hComponent,                                         \
        pComponentName,                                     \
        pComponentVersion,                                  \
        pSpecVersion,                                       \
        pComponentUUID)                 /* Macro End */

















































#define OMX_SendCommand(                                    \
         hComponent,                                        \
         Cmd,                                               \
         nParam,                                            \
         pCmdData)                                          \
     ((OMX_COMPONENTTYPE*)hComponent)->SendCommand(         \
         hComponent,                                        \
         Cmd,                                               \
         nParam,                                            \
         pCmdData)                          /* Macro End */































#define OMX_GetParameter(                                   \
        hComponent,                                         \
        nParamIndex,                                        \
        pComponentParameterStructure)                        \
    ((OMX_COMPONENTTYPE*)hComponent)->GetParameter(         \
        hComponent,                                         \
        nParamIndex,                                        \
        pComponentParameterStructure)    /* Macro End */































#define OMX_SetParameter(                                   \
        hComponent,                                         \
        nParamIndex,                                        \
        pComponentParameterStructure)                        \
    ((OMX_COMPONENTTYPE*)hComponent)->SetParameter(         \
        hComponent,                                         \
        nParamIndex,                                        \
        pComponentParameterStructure)    /* Macro End */



























        
#define OMX_GetConfig(                                      \
        hComponent,                                         \
        nConfigIndex,                                       \
        pComponentConfigStructure)                           \
    ((OMX_COMPONENTTYPE*)hComponent)->GetConfig(            \
        hComponent,                                         \
        nConfigIndex,                                       \
        pComponentConfigStructure)       /* Macro End */




























#define OMX_SetConfig(                                      \
        hComponent,                                         \
        nConfigIndex,                                       \
        pComponentConfigStructure)                           \
    ((OMX_COMPONENTTYPE*)hComponent)->SetConfig(            \
        hComponent,                                         \
        nConfigIndex,                                       \
        pComponentConfigStructure)       /* Macro End */


























#define OMX_GetExtensionIndex(                              \
        hComponent,                                         \
        cParameterName,                                     \
        pIndexType)                                         \
    ((OMX_COMPONENTTYPE*)hComponent)->GetExtensionIndex(    \
        hComponent,                                         \
        cParameterName,                                     \
        pIndexType)                     /* Macro End */



















#define OMX_GetState(                                       \
        hComponent,                                         \
        pState)                                             \
    ((OMX_COMPONENTTYPE*)hComponent)->GetState(             \
        hComponent,                                         \
        pState)                         /* Macro End */





















#define OMX_UseBuffer(                                      \
           hComponent,                                      \
           ppBufferHdr,                                     \
           nPortIndex,                                      \
           pAppPrivate,                                     \
           nSizeBytes,                                      \
           pBuffer)                                         \
    ((OMX_COMPONENTTYPE*)hComponent)->UseBuffer(            \
           hComponent,                                      \
           ppBufferHdr,                                     \
           nPortIndex,                                      \
           pAppPrivate,                                     \
           nSizeBytes,                                      \
           pBuffer)




























    
#define OMX_AllocateBuffer(                                 \
        hComponent,                                         \
        ppBuffer,                                           \
        nPortIndex,                                         \
        pAppPrivate,                                        \
        nSizeBytes)                                         \
    ((OMX_COMPONENTTYPE*)hComponent)->AllocateBuffer(       \
        hComponent,                                         \
        ppBuffer,                                           \
        nPortIndex,                                         \
        pAppPrivate,                                        \
        nSizeBytes)                     /* Macro End */
























#define OMX_FreeBuffer(                                     \
        hComponent,                                         \
        nPortIndex,                                         \
        pBuffer)                                            \
    ((OMX_COMPONENTTYPE*)hComponent)->FreeBuffer(           \
        hComponent,                                         \
        nPortIndex,                                         \
        pBuffer)                        /* Macro End */
























#define OMX_EmptyThisBuffer(                                \
        hComponent,                                         \
        pBuffer)                                            \
    ((OMX_COMPONENTTYPE*)hComponent)->EmptyThisBuffer(      \
        hComponent,                                         \
        pBuffer)                        /* Macro End */
























#define OMX_FillThisBuffer(                                 \
        hComponent,                                         \
        pBuffer)                                            \
    ((OMX_COMPONENTTYPE*)hComponent)->FillThisBuffer(       \
        hComponent,                                         \
        pBuffer)                        /* Macro End */

































#define OMX_UseEGLImage(                                    \
           hComponent,                                      \
           ppBufferHdr,                                     \
           nPortIndex,                                      \
           pAppPrivate,                                     \
           eglImage)                                        \
    ((OMX_COMPONENTTYPE*)hComponent)->UseEGLImage(          \
           hComponent,                                      \
           ppBufferHdr,                                     \
           nPortIndex,                                      \
           pAppPrivate,                                     \
           eglImage)












OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_Init(void);














OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_Deinit(void);







































OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_ComponentNameEnum(
    OMX_OUT OMX_STRING cComponentName,
    OMX_IN  OMX_U32 nNameLength,
    OMX_IN  OMX_U32 nIndex);





























OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_GetHandle(
    OMX_OUT OMX_HANDLETYPE* pHandle, 
    OMX_IN  OMX_STRING cComponentName,
    OMX_IN  OMX_PTR pAppData,
    OMX_IN  OMX_CALLBACKTYPE* pCallBacks);

















OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_FreeHandle(
    OMX_IN  OMX_HANDLETYPE hComponent);



















































OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_SetupTunnel(
    OMX_IN  OMX_HANDLETYPE hOutput,
    OMX_IN  OMX_U32 nPortOutput,
    OMX_IN  OMX_HANDLETYPE hInput,
    OMX_IN  OMX_U32 nPortInput);
    

OMX_API OMX_ERRORTYPE   OMX_GetContentPipe(
    OMX_OUT OMX_HANDLETYPE *hPipe,
    OMX_IN OMX_STRING szURI);




























OMX_API OMX_ERRORTYPE OMX_GetComponentsOfRole ( 
	OMX_IN      OMX_STRING role,
    OMX_INOUT   OMX_U32 *pNumComps,
    OMX_INOUT   OMX_U8  **compNames);


























OMX_API OMX_ERRORTYPE OMX_GetRolesOfComponent ( 
	OMX_IN      OMX_STRING compName, 
    OMX_INOUT   OMX_U32 *pNumRoles,
    OMX_OUT     OMX_U8 **roles);

#ifdef __cplusplus
}
#endif 

#endif



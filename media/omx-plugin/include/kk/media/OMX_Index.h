













































#ifndef OMX_Index_h
#define OMX_Index_h

#ifdef __cplusplus
extern "C" {
#endif






#include <OMX_Types.h>

















typedef enum OMX_INDEXTYPE {

    OMX_IndexComponentStartUnused = 0x01000000,
    OMX_IndexParamPriorityMgmt,             
    OMX_IndexParamAudioInit,                
    OMX_IndexParamImageInit,                
    OMX_IndexParamVideoInit,                
    OMX_IndexParamOtherInit,                
    OMX_IndexParamNumAvailableStreams,      
    OMX_IndexParamActiveStream,             
    OMX_IndexParamSuspensionPolicy,         
    OMX_IndexParamComponentSuspended,       
    OMX_IndexConfigCapturing,                
    OMX_IndexConfigCaptureMode,              
    OMX_IndexAutoPauseAfterCapture,          
    OMX_IndexParamContentURI,               
    OMX_IndexParamCustomContentPipe,         
    OMX_IndexParamDisableResourceConcealment, 
    OMX_IndexConfigMetadataItemCount,       
    OMX_IndexConfigContainerNodeCount,      
    OMX_IndexConfigMetadataItem,            
    OMX_IndexConfigCounterNodeID,           
    OMX_IndexParamMetadataFilterType,       
    OMX_IndexParamMetadataKeyFilter,        
    OMX_IndexConfigPriorityMgmt,            
    OMX_IndexParamStandardComponentRole,    

    OMX_IndexPortStartUnused = 0x02000000,
    OMX_IndexParamPortDefinition,           
    OMX_IndexParamCompBufferSupplier,        
    OMX_IndexReservedStartUnused = 0x03000000,

    
    OMX_IndexAudioStartUnused = 0x04000000,
    OMX_IndexParamAudioPortFormat,          
    OMX_IndexParamAudioPcm,                 
    OMX_IndexParamAudioAac,                 
    OMX_IndexParamAudioRa,                  
    OMX_IndexParamAudioMp3,                 
    OMX_IndexParamAudioAdpcm,               
    OMX_IndexParamAudioG723,                
    OMX_IndexParamAudioG729,                
    OMX_IndexParamAudioAmr,                 
    OMX_IndexParamAudioWma,                 
    OMX_IndexParamAudioSbc,                 
    OMX_IndexParamAudioMidi,                
    OMX_IndexParamAudioGsm_FR,              
    OMX_IndexParamAudioMidiLoadUserSound,   
    OMX_IndexParamAudioG726,                
    OMX_IndexParamAudioGsm_EFR,             
    OMX_IndexParamAudioGsm_HR,              
    OMX_IndexParamAudioPdc_FR,              
    OMX_IndexParamAudioPdc_EFR,             
    OMX_IndexParamAudioPdc_HR,              
    OMX_IndexParamAudioTdma_FR,             
    OMX_IndexParamAudioTdma_EFR,            
    OMX_IndexParamAudioQcelp8,              
    OMX_IndexParamAudioQcelp13,             
    OMX_IndexParamAudioEvrc,                
    OMX_IndexParamAudioSmv,                 
    OMX_IndexParamAudioVorbis,              
    OMX_IndexParamAudioFlac,                

    OMX_IndexConfigAudioMidiImmediateEvent, 
    OMX_IndexConfigAudioMidiControl,        
    OMX_IndexConfigAudioMidiSoundBankProgram, 
    OMX_IndexConfigAudioMidiStatus,         
    OMX_IndexConfigAudioMidiMetaEvent,      
    OMX_IndexConfigAudioMidiMetaEventData,  
    OMX_IndexConfigAudioVolume,             
    OMX_IndexConfigAudioBalance,            
    OMX_IndexConfigAudioChannelMute,        
    OMX_IndexConfigAudioMute,               
    OMX_IndexConfigAudioLoudness,           
    OMX_IndexConfigAudioEchoCancelation,    
    OMX_IndexConfigAudioNoiseReduction,     
    OMX_IndexConfigAudioBass,               
    OMX_IndexConfigAudioTreble,             
    OMX_IndexConfigAudioStereoWidening,     
    OMX_IndexConfigAudioChorus,             
    OMX_IndexConfigAudioEqualizer,          
    OMX_IndexConfigAudioReverberation,      
    OMX_IndexConfigAudioChannelVolume,      

    
    OMX_IndexImageStartUnused = 0x05000000,
    OMX_IndexParamImagePortFormat,          
    OMX_IndexParamFlashControl,             
    OMX_IndexConfigFocusControl,            
    OMX_IndexParamQFactor,                  
    OMX_IndexParamQuantizationTable,        
    OMX_IndexParamHuffmanTable,             
    OMX_IndexConfigFlashControl,            

    
    OMX_IndexVideoStartUnused = 0x06000000,
    OMX_IndexParamVideoPortFormat,          
    OMX_IndexParamVideoQuantization,        
    OMX_IndexParamVideoFastUpdate,          
    OMX_IndexParamVideoBitrate,             
    OMX_IndexParamVideoMotionVector,        
    OMX_IndexParamVideoIntraRefresh,        
    OMX_IndexParamVideoErrorCorrection,     
    OMX_IndexParamVideoVBSMC,               
    OMX_IndexParamVideoMpeg2,               
    OMX_IndexParamVideoMpeg4,               
    OMX_IndexParamVideoWmv,                 
    OMX_IndexParamVideoRv,                  
    OMX_IndexParamVideoAvc,                 
    OMX_IndexParamVideoH263,                
    OMX_IndexParamVideoProfileLevelQuerySupported, 
    OMX_IndexParamVideoProfileLevelCurrent, 
    OMX_IndexConfigVideoBitrate,            
    OMX_IndexConfigVideoFramerate,          
    OMX_IndexConfigVideoIntraVOPRefresh,    
    OMX_IndexConfigVideoIntraMBRefresh,     
    OMX_IndexConfigVideoMBErrorReporting,   
    OMX_IndexParamVideoMacroblocksPerFrame, 
    OMX_IndexConfigVideoMacroBlockErrorMap, 
    OMX_IndexParamVideoSliceFMO,            
    OMX_IndexConfigVideoAVCIntraPeriod,     
    OMX_IndexConfigVideoNalSize,            

    
    OMX_IndexCommonStartUnused = 0x07000000,
    OMX_IndexParamCommonDeblocking,         
    OMX_IndexParamCommonSensorMode,         
    OMX_IndexParamCommonInterleave,         
    OMX_IndexConfigCommonColorFormatConversion, 
    OMX_IndexConfigCommonScale,             
    OMX_IndexConfigCommonImageFilter,       
    OMX_IndexConfigCommonColorEnhancement,  
    OMX_IndexConfigCommonColorKey,          
    OMX_IndexConfigCommonColorBlend,        
    OMX_IndexConfigCommonFrameStabilisation,
    OMX_IndexConfigCommonRotate,            
    OMX_IndexConfigCommonMirror,            
    OMX_IndexConfigCommonOutputPosition,    
    OMX_IndexConfigCommonInputCrop,         
    OMX_IndexConfigCommonOutputCrop,        
    OMX_IndexConfigCommonDigitalZoom,       
    OMX_IndexConfigCommonOpticalZoom,       
    OMX_IndexConfigCommonWhiteBalance,      
    OMX_IndexConfigCommonExposure,          
    OMX_IndexConfigCommonContrast,          
    OMX_IndexConfigCommonBrightness,        
    OMX_IndexConfigCommonBacklight,         
    OMX_IndexConfigCommonGamma,             
    OMX_IndexConfigCommonSaturation,        
    OMX_IndexConfigCommonLightness,         
    OMX_IndexConfigCommonExclusionRect,     
    OMX_IndexConfigCommonDithering,         
    OMX_IndexConfigCommonPlaneBlend,        
    OMX_IndexConfigCommonExposureValue,     
    OMX_IndexConfigCommonOutputSize,        
    OMX_IndexParamCommonExtraQuantData,     
    OMX_IndexConfigCommonFocusRegion,       
    OMX_IndexConfigCommonFocusStatus,       
    OMX_IndexConfigCommonTransitionEffect,  

    
    OMX_IndexOtherStartUnused = 0x08000000,
    OMX_IndexParamOtherPortFormat,          
    OMX_IndexConfigOtherPower,              
    OMX_IndexConfigOtherStats,              


    
    OMX_IndexTimeStartUnused = 0x09000000,
    OMX_IndexConfigTimeScale,               
    OMX_IndexConfigTimeClockState,          
    OMX_IndexConfigTimeActiveRefClock,      
    OMX_IndexConfigTimeCurrentMediaTime,    
    OMX_IndexConfigTimeCurrentWallTime,     
    OMX_IndexConfigTimeCurrentAudioReference, 
    OMX_IndexConfigTimeCurrentVideoReference, 
    OMX_IndexConfigTimeMediaTimeRequest,    
    OMX_IndexConfigTimeClientStartTime,     
    OMX_IndexConfigTimePosition,            
    OMX_IndexConfigTimeSeekMode,            


    OMX_IndexKhronosExtensions = 0x6F000000,  
    
    OMX_IndexVendorStartUnused = 0x7F000000,
    




    OMX_IndexMax = 0x7FFFFFFF

} OMX_INDEXTYPE;

#ifdef __cplusplus
}
#endif 

#endif


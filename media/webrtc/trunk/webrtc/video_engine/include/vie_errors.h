









#ifndef WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_ERRORS_H_
#define WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_ERRORS_H_

enum ViEErrors {
  
  kViENotInitialized = 12000,        
  kViEBaseVoEFailure,                
  kViEBaseChannelCreationFailed,     
  kViEBaseInvalidChannelId,          
  kViEAPIDoesNotExist,               
  kViEBaseInvalidArgument,
  kViEBaseAlreadySending,            
  kViEBaseNotSending,                
  kViEBaseReceiveOnlyChannel,        
  kViEBaseAlreadyReceiving,          
  kViEBaseObserverAlreadyRegistered,  
  kViEBaseObserverNotRegistered,     
  kViEBaseUnknownError,              

  
  kViECodecInvalidArgument  = 12100,    
  kViECodecObserverAlreadyRegistered,   
  kViECodecObserverNotRegistered,       
  kViECodecInvalidCodec,                
  kViECodecInvalidChannelId,            
  kViECodecInUse,                       
  kViECodecReceiveOnlyChannel,          
  kViECodecUnknownError,                

  
  kViERenderInvalidRenderId = 12200,  
  kViERenderAlreadyExists,            
  kViERenderInvalidFrameFormat,       
  kViERenderUnknownError,             

  
  kViECaptureDeviceAlreadyConnected = 12300,  
  kViECaptureDeviceDoesNotExist,              
  kViECaptureDeviceInvalidChannelId,          
  kViECaptureDeviceNotConnected,              
  kViECaptureDeviceNotStarted,                
  kViECaptureDeviceAlreadyStarted,            
  kViECaptureDeviceAlreadyAllocated,          
  kViECaptureDeviceMaxNoDevicesAllocated,     
  kViECaptureObserverAlreadyRegistered,       
  kViECaptureDeviceObserverNotRegistered,     
  kViECaptureDeviceUnknownError,              
  kViECaptureDeviceMacQtkitNotSupported,      

  
  kViEFileInvalidChannelId  = 12400,  
  kViEFileInvalidArgument,            
  kViEFileAlreadyRecording,           
  kViEFileVoENotSet,                  
  kViEFileNotRecording,               
  kViEFileMaxNoOfFilesOpened,         
  kViEFileNotPlaying,                 
  kViEFileObserverAlreadyRegistered,  
  kViEFileObserverNotRegistered,      
  kViEFileInputAlreadyConnected,      
  kViEFileNotConnected,               
  kViEFileVoEFailure,                 
  kViEFileInvalidRenderId,            
  kViEFileInvalidFile,                
  kViEFileInvalidCapture,             
  kViEFileSetRenderTimeoutError,      
  kViEFileSetStartImageError,         
  kViEFileUnknownError,               

  
  kViENetworkInvalidChannelId = 12500,   
  kViENetworkAlreadyReceiving,           
  kViENetworkLocalReceiverNotSet,        
  kViENetworkAlreadySending,             
  kViENetworkDestinationNotSet,          
  kViENetworkInvalidArgument,            
  kViENetworkSendCodecNotSet,            
  kViENetworkServiceTypeNotSupported,    
  kViENetworkNotSupported,               
  kViENetworkUnknownError,               

  
  kViERtpRtcpInvalidChannelId = 12600,   
  kViERtpRtcpAlreadySending,             
  kViERtpRtcpNotSending,                 
  kViERtpRtcpRtcpDisabled,               
  kViERtpRtcpObserverAlreadyRegistered,  
  kViERtpRtcpObserverNotRegistered,      
  kViERtpRtcpUnknownError,               

  
  kViEImageProcessInvalidChannelId  = 12800,  
  kViEImageProcessInvalidCaptureId,          
  kViEImageProcessFilterExists,              
  kViEImageProcessFilterDoesNotExist,        
  kViEImageProcessAlreadyEnabled,            
  kViEImageProcessAlreadyDisabled,           
  kViEImageProcessUnknownError               
};

#endif  

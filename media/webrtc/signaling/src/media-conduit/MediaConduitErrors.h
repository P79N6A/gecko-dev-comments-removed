




#ifndef MEDIA_SESSION_ERRORS_H_
#define MEDIA_SESSION_ERRORS_H_

namespace mozilla
{
enum MediaConduitErrorCode
{
kMediaConduitNoError = 0,              
kMediaConduitSessionNotInited = 10100, 
                                       
kMediaConduitMalformedArgument,        
kMediaConduitCaptureError,             
kMediaConduitInvalidSendCodec,         
kMediaConduitInvalidReceiveCodec,      
kMediaConduitCodecInUse,               
kMediaConduitInvalidRenderer,          
kMediaConduitRendererFail,             
kMediaConduitSendingAlready,           
kMediaConduitReceivingAlready,         
kMediaConduitTransportRegistrationFail,
kMediaConduitInvalidTransport,         
kMediaConduitChannelError,             
kMediaConduitSocketError,              
kMediaConduitRTPRTCPModuleError,       
kMediaConduitRTPProcessingFailed,      
kMediaConduitUnknownError,             
kMediaConduitExternalRecordingError,   
kMediaConduitRecordingError,           
kMediaConduitExternalPlayoutError,     
kMediaConduitPlayoutError,             
kMediaConduitMTUError,                 
kMediaConduitRTCPStatusError,          
kMediaConduitKeyFrameRequestError,     
kMediaConduitNACKStatusError,          
kMediaConduitTMMBRStatusError          
};

}

#endif


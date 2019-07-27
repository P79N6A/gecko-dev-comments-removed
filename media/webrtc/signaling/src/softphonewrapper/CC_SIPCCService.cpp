



#ifdef _WIN32
#include <windows.h> 
#endif

#include "CSFLog.h"

#include "CC_CallTypes.h"
#include "CC_SIPCCService.h"
#include "NullDeleter.h"
#include "CC_SIPCCDevice.h"
#include "CC_SIPCCDeviceInfo.h"
#include "CC_SIPCCFeatureInfo.h"
#include "CC_SIPCCCallServerInfo.h"
#include "CC_SIPCCCall.h"
#include "CC_SIPCCCallInfo.h"
#include "CC_SIPCCLine.h"
#include "CC_SIPCCLineInfo.h"
#include "CSFMediaProvider.h"
#include "CSFAudioTermination.h"
#include "CSFVideoTermination.h"

#include "base/platform_thread.h"
#include "base/time.h"

extern "C" {
#include "ccapi_device.h"
}
#include "debug-psipcc-types.h"
#include "VcmSIPCCBinding.h"

#include "csf_common.h"

static const char* logTag = "CC_SIPCCService";

using namespace std;

#define MAX_SUPPORTED_NUM_CALLS        100
#define MAX_SUPPORTED_NUM_LINES        100
#define MAX_SUPPORTED_NUM_FEATURES     100
#define MAX_SUPPORTED_NUM_CALL_SERVERS 100

extern "C"
{
#include "cpr_types.h"
#include "ccapi_device.h"
#include "ccapi_device_info.h"
#include "ccapi_call.h"

#include "cpr_stdio.h"
#include "config_api.h"
#include "ccapi_service.h"
#include "plat_api.h"















void configCtlFetchReq(int device_handle)
{
    CSFLogDebug(logTag, "In configCtlFetchReq");

    CSF::CC_SIPCCService * pPhone = CSF::CC_SIPCCService::_self;

    if (pPhone == nullptr)
    {
        CSFLogError( logTag, "CC_SIPCCService::_self is NULL.");
    }
    else
    {
    	CCAPI_Start_response(device_handle, pPhone->deviceName.c_str(), pPhone->sipUser.c_str(),
    						 	 pPhone->sipPassword.c_str(), pPhone->sipDomain.c_str());
    }
}

















void configFetchReq(int device_handle)
{
    CSFLogDebug( logTag, "In configFetchReq");

    configCtlFetchReq(device_handle);
}













void configParserError(void)
{
    CSFLogError( logTag, "In configParserError");
}



























void configApplyConfigNotify(cc_string_t config_version,
		cc_string_t dial_plan_version,
		cc_string_t fcp_version,
		cc_string_t cucm_result,
		cc_string_t load_id,
		cc_string_t inactive_load_id,
		cc_string_t load_server,
		cc_string_t log_server,
		cc_boolean ppid)
{
    CSFLogDebug( logTag, "In configApplyConfigNotify");
}

char * platGetIPAddr ()
{
    CSFLogDebug( logTag, "In platGetIPAddr()");

    CSF::CC_SIPCCService * pPhone = CSF::CC_SIPCCService::_self;

    if (pPhone == nullptr)
    {
        CSFLogError( logTag, "In platGetIPAddr(). CC_SIPCCService::_self is NULL.");
        return (char *) "";
    }

	return (char*) pPhone->localAddress.c_str();
}

void ccmedia_flash_once_timer_callback (void)
{
}











cc_boolean dialPlanFetchReq(int device_handle, char* dialPlanFileName)
{
    return 0;
}











cc_boolean fcpFetchReq(int device_handle, char* fcpFileName)
{
    return 0;
}

extern cc_int32_t SipDebugMessage;
extern cc_int32_t SipDebugState;
extern cc_int32_t SipDebugTask;
extern cc_int32_t SipDebugRegState;
extern cc_int32_t GSMDebug;
extern cc_int32_t FIMDebug;
extern cc_int32_t LSMDebug;
extern cc_int32_t FSMDebugSM;
extern int32_t CSMDebugSM;
extern cc_int32_t CCDebug;
extern cc_int32_t CCDebugMsg;
extern cc_int32_t AuthDebug;
extern cc_int32_t ConfigDebug;
extern cc_int32_t DpintDebug;
extern cc_int32_t KpmlDebug;
extern cc_int32_t VCMDebug;
extern cc_int32_t g_CCAppDebug;
extern cc_int32_t g_CCLogDebug;
extern cc_int32_t TNPDebug;

static cc_int32_t * _maskedLoggingEntriesArray[19] = {
  &SipDebugMessage, 
  &SipDebugState, 
  &SipDebugTask, 
  &SipDebugRegState, 
  &GSMDebug, 
  &FIMDebug, 
  &LSMDebug, 
  &FSMDebugSM, 
  &CSMDebugSM, 
  &CCDebug, 
  &CCDebugMsg, 
  &AuthDebug, 
  &ConfigDebug, 
  &DpintDebug, 
  &KpmlDebug, 
  &VCMDebug, 
  &g_CCAppDebug, 
  &g_CCLogDebug, 
  &TNPDebug, 
};
















#define HAS_21_BITS 0x1FFFFF

static int _maxBitValueMaskedLoggingEntries = csf_countof(_maskedLoggingEntriesArray);


} 

extern "C" void CCAPI_DeviceListener_onDeviceEvent(ccapi_device_event_e type, cc_device_handle_t hDevice, cc_deviceinfo_ref_t dev_info)
{
    
    CSF::CC_SIPCCService::onDeviceEvent(type, hDevice, dev_info);
}

extern "C" void CCAPI_DeviceListener_onFeatureEvent(ccapi_device_event_e type, cc_deviceinfo_ref_t dev_info, cc_featureinfo_ref_t feature_info)
{
    
    CSF::CC_SIPCCService::onFeatureEvent(type, dev_info, feature_info);
}

extern "C" void CCAPI_LineListener_onLineEvent(ccapi_line_event_e type, cc_lineid_t line, cc_lineinfo_ref_t info)
{
    
    CSF::CC_SIPCCService::onLineEvent(type, line, info);
}

extern "C" void CCAPI_CallListener_onCallEvent(ccapi_call_event_e type, cc_call_handle_t handle, cc_callinfo_ref_t info)
{
    
	CSF::CC_SIPCCService::onCallEvent(type, handle, info);
}



namespace CSF
{

CC_SIPCCService* CC_SIPCCService::_self = nullptr;

CC_SIPCCService::CC_SIPCCService()
: loggingMask(0),
  bCreated(false),
  bStarted(false),
  bUseConfig(false)
{
	
    assert(_self == nullptr);
    _self = this;
    
    
    
}

CC_SIPCCService::~CC_SIPCCService()
{
  destroy();

  _self = nullptr;
}

bool CC_SIPCCService::init(const std::string& user, const std::string& password, const std::string& domain, const std::string& device)
{
    sipUser = user;
    sipPassword = password;
    sipDomain = domain;
    deviceName = device;

    if (!(bCreated = (CCAPI_Service_create() == CC_SUCCESS)))
    {
        CSFLogError( logTag, "Call to CCAPI_Service_create() failed.");
        return false;
    }
    return true;
}

void CC_SIPCCService::destroy()
{
	stop();

    if (bCreated)
    {
        if (CCAPI_Service_destroy() == CC_FAILURE)
        {
            CSFLogError( logTag, "Call to CCAPI_Service_destroy() failed.");
        }

        bCreated = false;
    }

	deviceName = "";
	loggingMask = 0;

    CC_SIPCCDevice::reset();
    CC_SIPCCDeviceInfo::reset();
    CC_SIPCCFeatureInfo::reset();
    CC_SIPCCCallServerInfo::reset();
    CC_SIPCCLine::reset();
    CC_SIPCCLineInfo::reset();
    CC_SIPCCCall::reset();
    CC_SIPCCCallInfo::reset();

	if(audioControlWrapper != nullptr)
	{
		audioControlWrapper->setAudioControl(nullptr);
	}
	if(videoControlWrapper != nullptr)
	{
		videoControlWrapper->setVideoControl(nullptr);
	}
}

void CC_SIPCCService::setDeviceName(const std::string& deviceName)
{
	this->deviceName = deviceName;
}

void CC_SIPCCService::setLoggingMask(int mask)
{
	this->loggingMask = mask;
}

void CC_SIPCCService::setLocalAddressAndGateway(const std::string& localAddress,
                                                const std::string& defaultGW)
{
	this->localAddress = localAddress;
    this->defaultGW = defaultGW;

    CCAPI_Device_IP_Update(CCAPI_Device_getDeviceID(), localAddress.c_str(), "", 0,
                           localAddress.c_str(), "", 0);

	AudioTermination* audio = VcmSIPCCBinding::getAudioTermination();
	if(audio != nullptr)
	{
		audio->setLocalIP(localAddress.c_str());
	}
	VideoTermination* video = VcmSIPCCBinding::getVideoTermination();
	if(video != nullptr)
	{
		video->setLocalIP(localAddress.c_str());
	}
}




bool CC_SIPCCService::startService()
{
	AudioTermination * pAudio = VcmSIPCCBinding::getAudioTermination();
	VideoTermination * pVideo = VcmSIPCCBinding::getVideoTermination();

	if(pAudio != nullptr)
	{
		pAudio->setMediaPorts(16384, 32766);
		pAudio->setDSCPValue(184);
    	pAudio->setVADEnabled(false);
	}

    if (pVideo != nullptr)
    {
		pVideo->setDSCPValue(136);
    }

    bUseConfig = false;
    if (!(bStarted = (CCAPI_Service_start() == CC_SUCCESS)))
    {
        CSFLogError( logTag, "Call to CCAPI_Service_start() failed.");
        return false;
    }

    CC_DevicePtr devicePtr = CC_SIPCCDevice::createDevice ();
    if (devicePtr == nullptr)
    {
    	CSFLogWarn( logTag, "stopping because createDevice failed");
    	stop();
        return false;
    }
    CSFLogDebug( logTag, "About to imposeLoggingMask");
    applyLoggingMask(loggingMask);

    return true;
}


void CC_SIPCCService::stop()
{
    if (bStarted)
    {
    	
    	endAllActiveCalls();

        if (CCAPI_Service_stop() == CC_FAILURE)
        {
            CSFLogError( logTag, "Call to CCAPI_Service_stop() failed.");
        }

        bStarted = false;
    }
}

bool CC_SIPCCService::isStarted()
{
	return bStarted;
}




CC_DevicePtr CC_SIPCCService::getActiveDevice()
{
    return CC_SIPCCDevice::wrap(CCAPI_Device_getDeviceID()).get();
}




vector<CC_DevicePtr> CC_SIPCCService::getDevices()
{
	vector<CC_DevicePtr> devices;

	CC_SIPCCDevicePtr pDevice = CC_SIPCCDevice::wrap(CCAPI_Device_getDeviceID());
	if(pDevice != nullptr)
	{
        devices.push_back(pDevice.get());
    }

    return devices;
}




AudioControlPtr CC_SIPCCService::getAudioControl ()
{
	if(audioControlWrapper != nullptr)
	{
		return audioControlWrapper.get();
	}
	else
	{
		audioControlWrapper = AudioControlWrapperPtr(new AudioControlWrapper(VcmSIPCCBinding::getAudioControl()));
		return audioControlWrapper.get();
	}
}




VideoControlPtr CC_SIPCCService::getVideoControl ()
{
	if(videoControlWrapper != nullptr)
	{
		return videoControlWrapper.get();
	}
	else
	{
		videoControlWrapper = VideoControlWrapperPtr(new VideoControlWrapper(VcmSIPCCBinding::getVideoControl()));
		return videoControlWrapper.get();
	}
}


void CC_SIPCCService::applyLoggingMask (int newMask)
{
    if (newMask >> _maxBitValueMaskedLoggingEntries > 0)
    {
    	CSFLogWarn( logTag, "Value of 0x%x specified for mask includes at least one bit value that exceeds the maximum supported bitfield value. "
                    "Ignoring unsupported bits.", newMask);
    }

    CSFLogDebug( logTag, "Applying a sipcc log mask = %d", newMask);

    loggingMask = newMask & (HAS_21_BITS);

    for (int i=0; i<_maxBitValueMaskedLoggingEntries; i++)
    {
        *(_maskedLoggingEntriesArray[i]) = (loggingMask >> i) & 0x1;
    }
}




void CC_SIPCCService::endAllActiveCalls()
{
	CC_DevicePtr device = getActiveDevice();
	if(device != nullptr)
	{
		CC_DeviceInfoPtr deviceInfo = device->getDeviceInfo();
		vector<CC_CallPtr> calls = deviceInfo->getCalls();
		CSFLogInfo( logTag, "endAllActiveCalls(): %zu calls to be ended.", calls.size());
		for(vector<CC_CallPtr>::iterator it = calls.begin(); it != calls.end(); it++)
		{
			
			CC_CallPtr call = *it;
			CC_CallInfoPtr callInfo = call->getCallInfo();
			if(callInfo->hasCapability(CC_CallCapabilityEnum::canEndCall))
			{
				CSFLogDebug( logTag, "endAllActiveCalls(): ending call %s -> %s [%s]",
					callInfo->getCallingPartyNumber().c_str(),
                    callInfo->getCalledPartyNumber().c_str(),
					call_state_getname(callInfo->getCallState()));
				call->endCall();
			}
			else if(callInfo->hasCapability(CC_CallCapabilityEnum::canResume) && callInfo->getCallState() != REMHOLD)
			{
				CSFLogDebug( logTag, "endAllActiveCalls(): resume then ending call %s -> %s, [%s]",
					callInfo->getCallingPartyNumber().c_str(),
                    callInfo->getCalledPartyNumber().c_str(),
					call_state_getname(callInfo->getCallState()));
				call->muteAudio();
				call->resume(callInfo->getVideoDirection());
				call->endCall();
			}
		}

		if(!calls.empty())
		{
#ifdef MOZILLA_INTERNAL_API
			
			PlatformThread::Sleep(500);
#endif
		}
    }
}



void CC_SIPCCService::onDeviceEvent(ccapi_device_event_e type, cc_device_handle_t handle, cc_deviceinfo_ref_t info)
{
    if (_self == nullptr)
    {
        CSFLogError( logTag, "CC_SIPCCService::_self is NULL. Unable to notify observers of device event.");
        return;
    }

    CC_SIPCCDevicePtr devicePtr = CC_SIPCCDevice::wrap(handle);
    if (devicePtr == nullptr)
    {
        CSFLogError( logTag, "Unable to notify device observers for device handle (%u), as failed to create CC_DevicePtr", handle);
        return;
    }

    CC_SIPCCDeviceInfoPtr infoPtr = CC_SIPCCDeviceInfo::wrap(info);
    if (infoPtr == nullptr)
    {
        CSFLogError( logTag, "Unable to notify call observers for device handle (%u), as failed to create CC_DeviceInfoPtr", handle);
        return;
    }

    CSFLogInfo( logTag, "onDeviceEvent( %s, %s, [%s] )",
      device_event_getname(type),
      devicePtr->toString().c_str(),
      infoPtr->getDeviceName().c_str());
    _self->notifyDeviceEventObservers(type, devicePtr.get(), infoPtr.get());
}

void CC_SIPCCService::onFeatureEvent(ccapi_device_event_e type, cc_deviceinfo_ref_t , cc_featureinfo_ref_t feature_info)
{

    if (_self == nullptr)
     {
         CSFLogError( logTag, "CC_SIPCCService::_self is NULL. Unable to notify observers of device event.");
         return;
     }

     cc_device_handle_t hDevice = CCAPI_Device_getDeviceID();
     CC_DevicePtr devicePtr = CC_SIPCCDevice::wrap(hDevice).get();
     if (devicePtr == nullptr)
     {
         CSFLogError( logTag, "Unable to notify device observers for device handle (%u), as failed to create CC_DevicePtr", hDevice);
         return;
     }

     CC_FeatureInfoPtr infoPtr = CC_SIPCCFeatureInfo::wrap(feature_info).get();
     if (infoPtr  == nullptr)
     {
         CSFLogError( logTag, "Unable to notify call observers for feature info handle (%p), as failed to create CC_FeatureInfoPtr", feature_info);
         return;
     }

     CSFLogInfo( logTag, "onFeatureEvent( %s, %s, [%s] )",
         device_event_getname(type),
         devicePtr->toString().c_str(),
         infoPtr->getDisplayName().c_str());
     _self->notifyFeatureEventObservers(type, devicePtr, infoPtr);
}

void CC_SIPCCService::onLineEvent(ccapi_line_event_e eventType, cc_lineid_t line, cc_lineinfo_ref_t info)
{
    if (_self == nullptr)
    {
        CSFLogError( logTag, "CC_SIPCCService::_self is NULL. Unable to notify observers of line event.");
        return;
    }

    CC_LinePtr linePtr = CC_SIPCCLine::wrap(line).get();
    if (linePtr == nullptr)
    {
        CSFLogError( logTag, "Unable to notify line observers for line lineId (%u), as failed to create CC_LinePtr", line);
        return;
    }

    CC_LineInfoPtr infoPtr = CC_SIPCCLineInfo::wrap(info).get();
    if (infoPtr == nullptr)
    {
        CSFLogError( logTag, "Unable to notify line observers for line lineId (%u), as failed to create CC_LineInfoPtr", line);
        return;
    }

    CSFLogInfo( logTag, "onLineEvent(%s, %s, [%s|%s]",
        line_event_getname(eventType), linePtr->toString().c_str(),
    	infoPtr->getNumber().c_str(), (infoPtr->getRegState() ? "INS" : "OOS"));
    _self->notifyLineEventObservers(eventType, linePtr, infoPtr);
}

void CC_SIPCCService::onCallEvent(ccapi_call_event_e eventType, cc_call_handle_t handle, cc_callinfo_ref_t info)
{
    if (_self == nullptr)
    {
        CSFLogError( logTag, "CC_SIPCCService::_self is NULL. Unable to notify observers of call event.");
        return;
    }

    CC_SIPCCCallPtr callPtr = CC_SIPCCCall::wrap(handle);
    if (callPtr == nullptr)
    {
        CSFLogError( logTag, "Unable to notify call observers for call handle (%u), as failed to create CC_CallPtr", handle);
        return;
    }

    CC_SIPCCCallInfoPtr infoPtr = CC_SIPCCCallInfo::wrap(info);
    if (infoPtr == nullptr)
    {
        CSFLogError( logTag, "Unable to notify call observers for call handle (%u), as failed to create CC_CallInfoPtr", handle);
        return;
    }

    infoPtr->setMediaData(callPtr->getMediaData());

	set<CSF::CC_CallCapabilityEnum::CC_CallCapability> capSet = infoPtr->getCapabilitySet();
    CSFLogInfo( logTag, "onCallEvent(%s, %s, [%s|%s]",
        call_event_getname(eventType), callPtr->toString().c_str(),
    	call_state_getname(infoPtr->getCallState()), CC_CallCapabilityEnum::toString(capSet).c_str());
    _self->notifyCallEventObservers(eventType, callPtr.get(), infoPtr.get());

    
    
    if (infoPtr->getCallState() == ONHOOK) {
        CSFLogDebug( logTag, "Removing call info from wrapper map (handle=%u)",
                     handle);
        CC_SIPCCCall::release(handle);
    }

    
    
    CC_SIPCCCallInfo::release(info);
}

void CC_SIPCCService::addCCObserver ( CC_Observer * observer )
{
    if (observer == nullptr)
    {
        CSFLogError( logTag, "NULL value for \"observer\" passed to addCCObserver().");
        return;
    }

    ccObservers.insert(observer);
}

void CC_SIPCCService::removeCCObserver ( CC_Observer * observer )
{
    ccObservers.erase(observer);
}


void CC_SIPCCService::notifyDeviceEventObservers (ccapi_device_event_e eventType, CC_DevicePtr devicePtr, CC_DeviceInfoPtr info)
{
	set<CC_Observer*>::const_iterator it = ccObservers.begin();
	for ( ; it != ccObservers.end(); it++ )
    {
	    (*it)->onDeviceEvent(eventType, devicePtr, info);
    }
}

void CC_SIPCCService::notifyFeatureEventObservers (ccapi_device_event_e eventType, CC_DevicePtr devicePtr, CC_FeatureInfoPtr info)
{
	set<CC_Observer*>::const_iterator it = ccObservers.begin();
	for ( ; it != ccObservers.end(); it++ )
    {
	    (*it)->onFeatureEvent(eventType, devicePtr, info);
    }
}

void CC_SIPCCService::notifyLineEventObservers (ccapi_line_event_e eventType, CC_LinePtr linePtr, CC_LineInfoPtr info)
{
	set<CC_Observer*>::const_iterator it = ccObservers.begin();
	for ( ; it != ccObservers.end(); it++ )
    {
	    (*it)->onLineEvent(eventType, linePtr, info);
    }
}

void CC_SIPCCService::notifyCallEventObservers (ccapi_call_event_e eventType, CC_CallPtr callPtr, CC_CallInfoPtr info)
{
	set<CC_Observer*>::const_iterator it = ccObservers.begin();
	for ( ; it != ccObservers.end(); it++ )
    {
	    (*it)->onCallEvent(eventType, callPtr, info);
    }
}







void CC_SIPCCService::registerStream(cc_call_handle_t call, int streamId, bool isVideo)
{
    CSFLogDebug( logTag, "registerStream for call: %d strId=%d video=%s",
        call, streamId, isVideo ? "TRUE" : "FALSE");
	
    CC_SIPCCCallPtr callPtr = CC_SIPCCCall::wrap(call);
    if (callPtr != nullptr)
    {
    	callPtr->addStream(streamId, isVideo);
    }
    else
    {
        CSFLogError( logTag, "registerStream(), No call found for allocated Stream: %d, %s",
            streamId, isVideo ? "TRUE" : "FALSE");
    }
}




void CC_SIPCCService::deregisterStream(cc_call_handle_t call, int streamId)
{
	
    CC_SIPCCCallPtr callPtr = CC_SIPCCCall::wrap(call);
    if (callPtr != nullptr)
    {
    	callPtr->removeStream(streamId);
    }
    else
    {
        CSFLogError( logTag, "deregisterStream(), No call found for deallocated Stream: %d", streamId);
    }
}




void CC_SIPCCService::dtmfBurst(int digit, int direction, int duration)
{
	
	vector<CC_SIPCCCallPtr> calls;
	{
		
		cc_deviceinfo_ref_t deviceInfoRef = CCAPI_Device_getDeviceInfo(CCAPI_Device_getDeviceID());
		cc_call_handle_t handles[MAX_SUPPORTED_NUM_CALLS] = {};
		cc_uint16_t numHandles = csf_countof(handles);

		CCAPI_DeviceInfo_getCalls(deviceInfoRef, handles, &numHandles);

		for (int i=0; i<numHandles; i++)
		{
			CC_SIPCCCallPtr callPtr = CC_SIPCCCall::wrap(handles[i]);
			calls.push_back(callPtr);
		}
		CCAPI_Device_releaseDeviceInfo(deviceInfoRef);
	}

	
	

	AudioTermination * pAudio = VcmSIPCCBinding::getAudioTermination();
	bool bSent = false;
	for(vector<CC_SIPCCCallPtr>::iterator it = calls.begin(); it != calls.end() && !bSent; it++)
    {
    	CC_SIPCCCallMediaDataPtr pMediaData = (*it)->getMediaData();

		for (StreamMapType::iterator entry =  pMediaData->streamMap.begin(); entry !=  pMediaData->streamMap.end(); entry++)
	    {
			if (entry->second.isVideo == false)
			{
				
				if (pAudio->sendDtmf(entry->first, digit))
				{
					
					bSent = true;
					break;
				}
				else
				{
					CSFLogWarn( logTag, "dtmfBurst:sendDtmf returned fail");
				}
			}
	    }
    }
}




void CC_SIPCCService::sendIFrame(cc_call_handle_t call_handle)
{
	CC_SIPCCCallPtr callPtr = CC_SIPCCCall::wrap(call_handle);
    CC_SIPCCCallMediaDataPtr pMediaData=callPtr->getMediaData();
    if (pMediaData != nullptr)
    {
        for (StreamMapType::iterator entry =  pMediaData->streamMap.begin(); entry !=  pMediaData->streamMap.end(); entry++)
        {
     		if (entry->second.isVideo == true)
            {
                VcmSIPCCBinding::getVideoTermination()->sendIFrame( entry->first );
            }
        }
    }
}

bool CC_SIPCCService::isValidMediaPortRange(int mediaStartPort, int mediaEndPort)
{
    if(mediaStartPort < 1024  ||
        mediaStartPort > 65535  ||
    	mediaEndPort < 1024 ||
    	mediaEndPort > 65535 ||
    	mediaEndPort - mediaStartPort < 3)
    {
    		return false;
    }
    else
    {
    	return true;
    }
}

bool CC_SIPCCService::isValidDSCPValue(int value)
{
	
	
	
	if(value >= 0 &&
		value <= 252 &&
		value % 4 == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void CC_SIPCCService::onVideoModeChanged( bool enable )
{
}




void CC_SIPCCService::onKeyFrameRequested( int stream )

{
    CSFLogDebug(logTag, "onKeyFrameRequested for stream ");
	
	vector<CC_SIPCCCallPtr> calls;
	{
		
		cc_deviceinfo_ref_t deviceInfoRef = CCAPI_Device_getDeviceInfo(CCAPI_Device_getDeviceID());
		cc_call_handle_t handles[MAX_SUPPORTED_NUM_CALLS] = {};
		cc_uint16_t numHandles = csf_countof(handles);

		CCAPI_DeviceInfo_getCalls(deviceInfoRef, handles, &numHandles);

		for (int i=0; i<numHandles; i++)
		{
			CC_SIPCCCallPtr callPtr = CC_SIPCCCall::wrap(handles[i]);
			
			calls.push_back(callPtr);
		}
		CCAPI_Device_releaseDeviceInfo(deviceInfoRef);
	}

	
	

	bool bSent = false;
	for(vector<CC_SIPCCCallPtr>::iterator it = calls.begin(); it != calls.end() && !bSent; it++)
    {
    	CC_SIPCCCallMediaDataPtr pMediaData = (*it)->getMediaData();

		for (StreamMapType::iterator entry =  pMediaData->streamMap.begin(); entry !=  pMediaData->streamMap.end(); entry++)
	    {
			if ((entry->first==stream) && (entry->second.isVideo == true))
			{
                CSFLogDebug(logTag, "Send SIP message to originator for stream id %d", stream);
				if ((*it)->sendInfo ( "","application/media_control+xml", "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n"
						"<media_control>\n"
						"\n"
						"  <vc_primitive>\n"
						"    <to_encoder>\n"
						"      <picture_fast_update/>\n"
						"    </to_encoder>\n"
						"  </vc_primitive>\n"
						"\n"
						"</media_control>\n"))
				{
					CSFLogWarn(logTag, "sendinfo returned true");
					bSent = true;
					break;
				}
				else
				{
					CSFLogWarn(logTag, "sendinfo returned false");
				}
			}
	    }
    }
}

void CC_SIPCCService::onMediaLost( int callId )
{
}

void CC_SIPCCService::onMediaRestored( int callId )
{
}

bool CC_SIPCCService::setLocalVoipPort(int port) {
	return CCAPI_Config_set_local_voip_port(port);
}

bool CC_SIPCCService::setRemoteVoipPort(int port) {
	return CCAPI_Config_set_remote_voip_port(port);
}

bool CC_SIPCCService::setP2PMode(bool mode)  {
	return CCAPI_Config_set_p2p_mode(mode);
}

bool CC_SIPCCService::setSDPMode(bool mode)  {
	return CCAPI_Config_set_sdp_mode(mode);
}

} 

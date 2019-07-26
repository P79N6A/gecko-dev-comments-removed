






































#ifndef _USE_CPVE

#include "CC_Common.h"

#include "string.h"
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#endif

#include "WebrtcMediaProvider.h"
#include "WebrtcAudioProvider.h"
#include "WebrtcToneGenerator.h"
#include "WebrtcRingGenerator.h"
#include "voe_file.h"
#include "voe_hardware.h"
#include "voe_errors.h"
#include "voe_network.h"
#include "voe_audio_processing.h"
#include "WebrtcLogging.h"

#include "CSFLog.h"
static const char* logTag = "WebrtcAudioProvider";
using namespace std;

extern "C" void config_get_value (int id, void *buffer, int length);


namespace CSF {

#define EXTRACT_DYNAMIC_PAYLOAD_TYPE(PTYPE) ((PTYPE)>>16)
#define CLEAR_DYNAMIC_PAYLOAD_TYPE(PTYPE)   (PTYPE & 0x0000FFFF)
#define CHECK_DYNAMIC_PAYLOAD_TYPE(PTYPE)   (PTYPE & 0xFFFF0000)


#ifdef WIN32
bool IsUserAdmin();
bool IsVistaOrNewer();
#endif

class WebrtcAudioStream {
public:
	WebrtcAudioStream(int _streamId, int _channelId):
		streamId(_streamId), channelId(_channelId),
		isRxStarted(false), isTxStarted(false), isAlive(false)
		{}
	int streamId;
	int channelId;
	bool isRxStarted;
	bool isTxStarted;
	bool isAlive;
};

WebrtcAudioProvider::WebrtcAudioProvider( WebrtcMediaProvider* provider )
: provider(provider), 
  voeVoice(NULL),
  voeBase(NULL), 
  voeFile(NULL),
  voeHw(NULL), 
  voeDTMF(NULL), 
  voeNetwork(NULL), 
  voeVolumeControl(NULL), 
  voeVoiceQuality(NULL), 
  voeEncryption(NULL),
  toneGen(NULL), 
  ringGen(NULL), 
  startPort(1024), 
  endPort(65535), 
  defaultVolume(100), 
  ringerVolume(100), 
  DSCPValue(0), 
  VADEnabled(false),
  stopping(false)
{
	LOG_WEBRTC_INFO( logTag, "WebrtcAudioProvider::WebrtcAudioProvider");
}

int WebrtcAudioProvider::init()
{
	LOG_WEBRTC_INFO( logTag, "WebrtcAudioProvider::init");

	if (voeVoice != NULL) {
		LOG_WEBRTC_ERROR( logTag, "WebrtcAudioProvider::init : already initialized");
		return -1;
	}

    voeVoice = webrtc::VoiceEngine::Create();
	if (voeVoice == NULL) {
		LOG_WEBRTC_ERROR( logTag, "WebrtcAudioProvider():VoiceEngine::Create failed");
		return -1;
	}

	voeBase = webrtc::VoEBase::GetInterface( voeVoice );
	if (voeBase == NULL) {
		LOG_WEBRTC_ERROR( logTag, "WebrtcAudioProvider(): VoEBase::GetInterface failed");
		return -1;
	}

	const int VERSIONBUFLEN=1024;
	char versionbuf[VERSIONBUFLEN];
	voeBase->GetVersion(versionbuf);
	LOG_WEBRTC_INFO( logTag, "%s", versionbuf );
	


	
	
	
	


	voeDTMF = webrtc::VoEDtmf::GetInterface( voeVoice );
	voeFile = webrtc::VoEFile::GetInterface( voeVoice );
	voeHw =   webrtc::VoEHardware::GetInterface( voeVoice );
	voeNetwork = webrtc::VoENetwork::GetInterface( voeVoice );
	voeVolumeControl = webrtc::VoEVolumeControl::GetInterface( voeVoice );
	voeVoiceQuality = webrtc::VoEAudioProcessing::GetInterface( voeVoice );
    voeEncryption = webrtc::VoEEncryption::GetInterface(voeVoice);

	if ((!voeDTMF) || (!voeFile) || (!voeHw) ||(!voeNetwork) || (!voeVolumeControl) || (!voeVoiceQuality) || (!voeEncryption)) {
		LOG_WEBRTC_ERROR( logTag, "WebrtcAudioProvider(): voeVE_GetInterface failed voeDTMF=%p voeFile=%p voeHw=%p voeNetwork=%p voeVolumeControl=%p voeVoiceQuality=%p voeEncryption=%p",
		voeDTMF,voeFile,voeHw,voeNetwork,voeVolumeControl,voeVoiceQuality,voeEncryption);
		return -1;
	}

	codecSelector.init(voeVoice, false, true);
	voeBase->Init();

	localRingChannel = voeBase->CreateChannel();
	localToneChannel = voeBase->CreateChannel();

    


	webrtc::AgcConfig webphone_agc_config = {3,9,1};
	voeVoiceQuality->SetAgcConfig(webphone_agc_config); 
	voeVoiceQuality->SetAgcStatus(true, webrtc::kAgcFixedDigital);
	voeVoiceQuality->SetNsStatus(true, webrtc::kNsModerateSuppression);

	
	int nDevices, error = 0;
	
	error = voeHw->GetNumOfRecordingDevices(nDevices);	
	for(int i = 0; i < nDevices; i++) {
		char name[128];
		char guid[128];
		memset(name,0,128);
		memset(guid,0,128);
		error = voeHw->GetRecordingDeviceName(i, name, guid);
		if(error == 0) {
			recordingDevice = name;
		} else {
			LOG_WEBRTC_ERROR( logTag, "WebrtcAudioProvider: GetRecordingDeviceNamefailed: Error: %d", voeBase->LastError() );
		}	
	}
	
	
	nDevices = 0;
	error = voeHw->GetNumOfPlayoutDevices(nDevices);
	for(int i=0; i < nDevices; i++) {
		char pname[128], pguid[128];
		memset(pname,0,128);
		memset(pguid,0,128);
		if ( voeHw->GetPlayoutDeviceName( i, pname, pguid ) == 0 ) {
			playoutDevice = pname;
		} else {
				LOG_WEBRTC_ERROR( logTag, "WebrtcAudioProvider: GetlayoutDeviceNamefailed: Error: %d", voeBase->LastError() );
		}
	}

    LOG_WEBRTC_DEBUG( logTag, "local IP: \"%s\"", localIP.c_str());
    LOG_WEBRTC_DEBUG( logTag, "RecordingDeviceName: \"%s\"", recordingDevice.c_str());
    LOG_WEBRTC_DEBUG( logTag, "PlayoutDeviceName: \"%s\"", playoutDevice.c_str());
	
	return 0;
}

WebrtcAudioProvider::~WebrtcAudioProvider() {
	LOG_WEBRTC_INFO( logTag, "WebrtcAudioProvider::~WebrtcAudioProvider");

	int num_ifs=0;
	stopping = true;
	
	codecSelector.release();

	voeBase->DeleteChannel( localToneChannel );
	voeBase->DeleteChannel( localRingChannel );
	voeBase->Terminate();

	if(voeBase->Release() !=0) {
		LOG_WEBRTC_ERROR( logTag, "~WebrtcAudioProvider(): voeBase->Release failed");
	}
	
	if ((num_ifs=voeDTMF->Release())!=0)
		LOG_WEBRTC_ERROR( logTag, "~WebrtcAudioProvider(): voeDTMF->Release failed, num_ifs left= %d",num_ifs);
	if ((num_ifs=voeFile->Release())!=0)
		LOG_WEBRTC_ERROR( logTag, "~WebrtcAudioProvider(): voeFile->Release failed, num_ifs left= %d ",num_ifs);
	if((num_ifs=voeHw->Release())!=0)
		LOG_WEBRTC_ERROR( logTag, "~WebrtcAudioProvider(): voeHw->Release failed, num_ifs left= %d " ,num_ifs);
	if((num_ifs=voeNetwork->Release())!=0)
		LOG_WEBRTC_ERROR( logTag, "~WebrtcAudioProvider(): voeNetwork->Release() failed, num_ifs left= %d" ,num_ifs);
	if((num_ifs=voeVolumeControl->Release())!=0)
		LOG_WEBRTC_ERROR( logTag, "~WebrtcAudioProvider(): voeVolumeControl->Release() failed, num_ifs left= %d" ,num_ifs);
	if((num_ifs=voeVoiceQuality->Release())!=0)
		LOG_WEBRTC_ERROR( logTag, "~WebrtcAudioProvider(): voeVoiceQuality->Release() failed, num_ifs left= %d ",num_ifs );
    if((num_ifs=voeEncryption->Release())!=0)
        LOG_WEBRTC_ERROR( logTag, "~WebrtcAudioProvider(): voeEncryption->Release() failed, num_ifs left= %d ",num_ifs );
	if(webrtc::VoiceEngine::Delete( voeVoice, true ) == false)
		LOG_WEBRTC_ERROR( logTag, "~WebrtcAudioProvider(): voeVoiceEngine::Delete failed" );

	delete toneGen;
	toneGen = NULL;

	delete ringGen;
	ringGen = NULL;

	
	voeFile = NULL;
	voeHw = NULL;
	voeDTMF = NULL;
	voeNetwork = NULL;
	voeVolumeControl = NULL;
	voeVoiceQuality = NULL;
	voeVoice = NULL;
	voeBase = NULL;
    voeEncryption = NULL;

	LOG_WEBRTC_INFO( logTag, "WebrtcAudioProvider::shutdown done");
}

std::vector<std::string> WebrtcAudioProvider::getRecordingDevices() {
	AutoLock lock(m_lock);
	char name[128];
	char guid[128];
	int  nRec = 0;
	memset(name,0,128);
	memset(guid,0,128);
	std::vector<std::string> deviceList;
    voeHw->GetNumOfRecordingDevices(nRec);

	for ( int i = 0; i < nRec; i++ ) {
		if ( voeHw->GetRecordingDeviceName( i, name, guid ) == 0 )
			deviceList.push_back( name );
	}
	return deviceList;
}

std::vector<std::string> WebrtcAudioProvider::getPlayoutDevices() {
	AutoLock lock(m_lock);
	char name[128];
	char guid[128];
	int nPlay = 0;
	std::vector<std::string> deviceList;
	memset(name,0,128);
	memset(guid,0,128);
	voeHw->GetNumOfPlayoutDevices( nPlay);
	for ( int i = 0; i < nPlay; i++ ) {
		if ( voeHw->GetPlayoutDeviceName( i, name, guid ) == 0 )
			deviceList.push_back( name );
	}
	return deviceList;
}

bool WebrtcAudioProvider::setRecordingDevice( const std::string& device ) {
	AutoLock lock(m_lock);
	char name[128];
	char guid[128];
	int nRec = 0, nPlay = 0;
	int playoutIndex, recordingIndex;
	memset(name,0,128);
	memset(guid,0,128);
	voeHw->GetNumOfRecordingDevices(  nRec );
	voeHw->GetNumOfPlayoutDevices(  nPlay );

	
	for ( recordingIndex = 0; recordingIndex < nRec; recordingIndex++ ) {
		if ( voeHw->GetRecordingDeviceName( recordingIndex, name, guid ) == 0 ) {
			if ( device.compare( name ) == 0 ) break;
		}
	}
	if ( recordingIndex == nRec ) {
		return false;  
	}

	
	
	name[0] = '\0';
	for ( playoutIndex = nPlay - 1; playoutIndex >= -1; playoutIndex-- ) {
		if ( voeHw->GetPlayoutDeviceName( playoutIndex, name, guid ) == 0 ) {
			if ( playoutDevice.compare( name ) == 0 ) break;
		}
		else name[0] = '\0';
	}
	if ( playoutIndex < -1 ) {
		playoutIndex = -1; 
	}

	if ( voeHw->SetRecordingDevice( recordingIndex ) == 0 
			&& voeHw->SetPlayoutDevice( playoutIndex) == 0 ) {
		recordingDevice = device;
		playoutDevice = name;	
		return true;
	}
	return false;
}

bool WebrtcAudioProvider::setPlayoutDevice( const std::string& device ) {
	AutoLock lock(m_lock);
	char name[128];
	char guid[128];
	int nPlay = 0, nRec = 0;
	int playoutIndex, recordingIndex;

	voeHw->GetNumOfRecordingDevices(  nRec );
	voeHw->GetNumOfPlayoutDevices(  nPlay );

	
	for ( playoutIndex = 0; playoutIndex < nPlay; playoutIndex++ ) {
		if ( voeHw->GetPlayoutDeviceName( playoutIndex, name, guid ) == 0 ) {
			if ( device.compare( name ) == 0 ) break;
		}
	}
	if ( playoutIndex == nPlay ) {
		return false; 
	}

	
	
	name[0] = '\0';
	guid[0] = '\0';
	for ( recordingIndex = nRec - 1; recordingIndex >= -1; recordingIndex-- ) {
		if ( voeHw->GetRecordingDeviceName( recordingIndex, name, guid ) == 0 ) {
			if ( recordingDevice.compare( name ) == 0 ) break;
		}
		else name[0] = '\0';
	}
	if ( recordingIndex < -1 ) {
		recordingIndex = -1; 
	}

	if ( voeHw->SetRecordingDevice( recordingIndex ) == 0 &&
			voeHw->SetPlayoutDevice( playoutIndex) == 0 ) {
		playoutDevice = device;
		recordingDevice = name;	
		return true;
	}
	return false;
}

WebrtcAudioStreamPtr WebrtcAudioProvider::getStreamByChannel( int channel ) {
	AutoLock lock(streamMapMutex);
	for( std::map<int, WebrtcAudioStreamPtr>::const_iterator it = streamMap.begin(); it != streamMap.end(); it++ ) {
		WebrtcAudioStreamPtr stream = it->second;
		if(stream->channelId == channel)
			return stream;
	}
	return WebrtcAudioStreamPtr();
}

WebrtcAudioStreamPtr WebrtcAudioProvider::getStream( int streamId ) {
	AutoLock lock(streamMapMutex);
	std::map<int, WebrtcAudioStreamPtr>::const_iterator it = streamMap.find( streamId );
	return ( it != streamMap.end() ) ? it->second : WebrtcAudioStreamPtr();
}

int WebrtcAudioProvider::getChannelForStreamId( int streamId ) {
	WebrtcAudioStreamPtr stream = getStream(streamId);
	return ( stream != NULL ) ? stream->channelId : -1;
}

int WebrtcAudioProvider::getCodecList( CodecRequestType requestType ) {
	AutoLock lock(m_lock);
	return codecSelector.advertiseCodecs(requestType);
}

int WebrtcAudioProvider::rxAlloc( int groupId, int streamId, int requestedPort ) {
	AutoLock lock(m_lock);
	LOG_WEBRTC_INFO( logTag, "rxAllocAudio: groupId=%d, streamId=%d, requestedPort=%d", groupId, streamId, requestedPort  );
	int channel = voeBase->CreateChannel();
	if ( channel == -1 ) {
		LOG_WEBRTC_ERROR( logTag, "rxAllocAudio: CreateChannel failed, error %d", voeBase->LastError() );
		return 0;
	}
	LOG_WEBRTC_DEBUG( logTag, "rxAllocAudio: Created channel %d", channel );
	voeNetwork->SetPeriodicDeadOrAliveStatus(channel, true);

	int beginPort;		
	int tryPort;		

	if ( requestedPort == 0  || requestedPort < startPort || requestedPort >= endPort )
		tryPort = startPort;
	else
		tryPort = requestedPort;

	beginPort = tryPort;

	const char * pLocalAddr = NULL;

	if (localIP.size() > 0) {
		pLocalAddr = localIP.c_str();
	}

	do {
		if ( voeBase->SetLocalReceiver( channel, tryPort, webrtc::kVoEDefault, pLocalAddr ) == 0 ) {
			int port, RTCPport;
			char ipaddr[64];
			ipaddr[0]='\0';
			
			voeBase->GetLocalReceiver(channel, port,RTCPport, ipaddr);
			localIP = ipaddr;
			LOG_WEBRTC_DEBUG( logTag, "rxAllocAudio: IPAddr: %d", ipaddr );
			LOG_WEBRTC_DEBUG( logTag, "rxAllocAudio: Allocated port %d", tryPort );
			WebrtcAudioStreamPtr stream(new WebrtcAudioStream(streamId, channel)); {
				AutoLock lock(streamMapMutex);
				streamMap[streamId] = stream;
			}
			setVolume(streamId, defaultVolume);
			return tryPort;
		}

		int errCode = voeBase->LastError();
		if ( errCode == VE_SOCKET_ERROR ||			
			 errCode == VE_BINDING_SOCKET_TO_LOCAL_ADDRESS_FAILED ||
			errCode == VE_RTCP_SOCKET_ERROR ) {
	        tryPort += 2;
	        if ( tryPort >= endPort )
	        	tryPort = startPort;
        }
		else {
			LOG_WEBRTC_ERROR( logTag, "rxAllocAudio: SetLocalReceiver returned error %d", errCode );
			voeBase->DeleteChannel( channel );
			return 0;
		}
	}
	while ( tryPort != beginPort );

	LOG_WEBRTC_WARN( logTag, "rxAllocAudio: No ports available?" );
	voeBase->DeleteChannel( channel );
	return 0;
}

int WebrtcAudioProvider::rxOpen( int groupId, int streamId, int requestedPort, int listenIp, bool isMulticast ) {
	AutoLock lock(m_lock);
	LOG_WEBRTC_INFO( logTag, "rxOpenAudio: groupId=%d, streamId=%d", groupId, streamId );
	int channel = getChannelForStreamId( streamId );
	if ( channel >= 0 ) {
		LOG_WEBRTC_DEBUG( logTag, "rxOpenAudio: return requestedPort=%d", requestedPort );
		return requestedPort;
	}
	return 0;
}

int WebrtcAudioProvider::rxStart( int groupId, int streamId, int payloadType, int packPeriod, int localPort,
		int rfc2833PayloadType, EncryptionAlgorithm algorithm, unsigned char* key, int keyLen, unsigned char* salt, int saltLen, int mode, int party ) {
	AutoLock lock(m_lock);
	LOG_WEBRTC_INFO( logTag, "rxStartAudio: groupId=%d, streamId=%d", groupId, streamId );
	int channel = getChannelForStreamId( streamId );
	if ( channel >= 0 ) {
		int dynamicPayloadType = -1;

	    if (CHECK_DYNAMIC_PAYLOAD_TYPE(payloadType)) {
	        dynamicPayloadType = EXTRACT_DYNAMIC_PAYLOAD_TYPE(payloadType);
	        payloadType = CLEAR_DYNAMIC_PAYLOAD_TYPE(payloadType);
	    }

	    if (dynamicPayloadType != -1) {
			webrtc::CodecInst codec;

			if (codecSelector.select(payloadType, dynamicPayloadType, packPeriod, codec) != 0) {
				LOG_WEBRTC_ERROR( logTag, "rxStartAudio cannot select codec (payloadType=%d, packPeriod=%d)",
						payloadType, packPeriod );
				return -1;
			}

			if (codecSelector.setReceive(channel, codec) != 0) {
				LOG_WEBRTC_ERROR( logTag, "rxStartAudio cannot set receive codec to channel=%d", channel );
				return -1;
			}
	    }

        switch(algorithm) {
            case EncryptionAlgorithm_NONE:
                LOG_WEBRTC_DEBUG( logTag, "rxStartAudio: using non-secure RTP for channel %d", channel);
                break;

            case EncryptionAlgorithm_AES_128_COUNTER: {
                unsigned char key[WEBRTC_KEY_LENGTH];

                LOG_WEBRTC_DEBUG( logTag, "rxStartAudio: using secure RTP for channel %d", channel);

                if(!provider->getKey(key, keyLen, salt, saltLen, key, sizeof(key))) {
                    LOG_WEBRTC_ERROR( logTag, "rxStartAudio: failed to generate key on channel %d", channel );
                    return -1;
                }

                










                memset(key, 0x00, sizeof(key));
                break;
            }  
        }

	    if (voeBase->StartReceive( channel ) == -1) {
	    	LOG_WEBRTC_ERROR( logTag, "rxStartAudio: cannot start listen on channel %d", channel );
	    	return -1;
	    }

	    LOG_WEBRTC_DEBUG( logTag, "rxStartAudio: Listening on channel %d", channel );

		if (voeBase->StartPlayout( channel ) == -1) {
			LOG_WEBRTC_ERROR( logTag, "rxStartAudio: cannot start playout on channel %d, stop listen", channel );
			
		}

		WebrtcAudioStreamPtr stream = getStream(streamId);
		if(stream != NULL)
			stream->isRxStarted = true;
		LOG_WEBRTC_DEBUG( logTag, "rxStartAudio: Playing on channel %d", channel );
		return 0;
	}
	else {
		LOG_WEBRTC_ERROR( logTag, "rxStartAudio: getChannelForStreamId failed streamId %d",streamId );
	}
	return -1;
}

void WebrtcAudioProvider::rxClose( int groupId, int streamId ) {
	AutoLock lock(m_lock);
	LOG_WEBRTC_INFO( logTag, "rxCloseAudio: groupId=%d, streamId=%d", groupId, streamId );
	int channel = getChannelForStreamId( streamId );
	if ( channel >= 0 ) {
		WebrtcAudioStreamPtr stream = getStream(streamId);
		if(stream != NULL)
			stream->isRxStarted = false;
		voeBase->StopPlayout( channel );
		LOG_WEBRTC_DEBUG( logTag, "rxCloseAudio: Stop playout on channel %d", channel );
	}
}

void WebrtcAudioProvider::rxRelease( int groupId, int streamId, int port )
{
	AutoLock lock(m_lock);
	LOG_WEBRTC_INFO( logTag, "rxReleaseAudio: groupId=%d, streamId=%d", groupId, streamId );
	int channel = getChannelForStreamId( streamId );
	if ( channel >= 0 ) {
		voeBase->StopReceive( channel );
		voeBase->DeleteChannel( channel ); {
			AutoLock lock(streamMapMutex);
			streamMap.erase(streamId);
		}
		LOG_WEBRTC_DEBUG( logTag, "rxReleaseAudio: Delete channel %d, release port %d", channel, port);
	}
	else {
		LOG_WEBRTC_ERROR( logTag, "rxReleaseAudio: getChannelForStreamId failed streamId %d",streamId );
	}
}

const unsigned char m_iGQOSServiceType =0x00000003;

int WebrtcAudioProvider::txStart( int groupId, int streamId, int payloadType, int packPeriod, bool vad, short tos,
		char* remoteIpAddr, int remotePort, int rfc2833PayloadType, EncryptionAlgorithm algorithm, unsigned char* key, int keyLen,
		unsigned char* salt, int saltLen, int mode, int party ) {
	AutoLock lock(m_lock);
	LOG_WEBRTC_INFO( logTag, "txStartAudio: groupId=%d, streamId=%d", groupId, streamId);
	int channel = getChannelForStreamId( streamId );
	if ( channel >= 0 ) {
		int dynamicPayloadType = -1;

	    if (CHECK_DYNAMIC_PAYLOAD_TYPE(payloadType)) {
	        dynamicPayloadType = EXTRACT_DYNAMIC_PAYLOAD_TYPE(payloadType);
	        payloadType = CLEAR_DYNAMIC_PAYLOAD_TYPE(payloadType);
	    }

	    webrtc::CodecInst codec;

		
		if (codecSelector.select(payloadType, dynamicPayloadType, packPeriod, codec) != 0) {
			LOG_WEBRTC_ERROR( logTag, "txStartAudio cannot select codec (payloadType=%d, packPeriod=%d)",
					payloadType, packPeriod );
			return -1;
		}

		
		if (codecSelector.setSend(channel, codec,payloadType,vad) != 0) {
			LOG_WEBRTC_ERROR( logTag, "txStartAudio cannot set send codec on channel=%d", channel );
			 LOG_WEBRTC_ERROR( logTag, "VoEAudioProvider: Error: %d", voeBase->LastError() );
			
			return -1;
		}

        switch(algorithm) {
            case EncryptionAlgorithm_NONE:
                LOG_WEBRTC_DEBUG( logTag, "txStartAudio: using non-secure RTP for channel %d", channel);
                break;

            case EncryptionAlgorithm_AES_128_COUNTER: {
                unsigned char key[WEBRTC_KEY_LENGTH];

                LOG_WEBRTC_DEBUG( logTag, "txStartAudio: using secure RTP for channel %d", channel);

                if(!provider->getKey(key, keyLen, salt, saltLen, key, sizeof(key))) {
                    LOG_WEBRTC_ERROR( logTag, "txStartAudio: failed to generate key on channel %d", channel );
                    return -1;
                }

                










                memset(key, 0x00, sizeof(key));

                break;
            }  
        }

        unsigned char dscpSixBit = DSCPValue>>2;
		voeBase->SetSendDestination( channel, remotePort, remoteIpAddr );
#ifdef WIN32
		if (IsVistaOrNewer()) {
			LOG_WEBRTC_DEBUG( logTag, "Vista or later");
			if(voeNetwork->SetSendTOS(channel, dscpSixBit, false ) == -1) {
				LOG_WEBRTC_DEBUG( logTag, "openIngressChannel():voeVE_SetSendTOS() returned error");
			}
			LOG_WEBRTC_DEBUG( logTag, " Wrapper::openIngressChannel:- voeVE_SetSendTOS(), useSetSockOpt = false");
		}
		else {
			if(voeNetwork->SetSendTOS(channel, dscpSixBit, true ) == -1) {
				LOG_WEBRTC_DEBUG( logTag, "openIngressChannel():voeVE_SetSendTOS() returned error");
			}
			LOG_WEBRTC_DEBUG( logTag, "Wrapper::openIngressChannel:- voeVE_SetSendTOS(), useSetSockOpt = true");
		}
#else
		voeNetwork->SetSendTOS(channel, dscpSixBit, -1, true );
#endif
		voeBase->StartSend( channel );
		WebrtcAudioStreamPtr stream = getStream(streamId);
		if(stream != NULL)
			stream->isTxStarted = true;
		LOG_WEBRTC_DEBUG( logTag, "txStartAudio: Sending to %s:%d on channel %d", remoteIpAddr, remotePort, channel );
		return 0;
	}
	else {
			LOG_WEBRTC_ERROR( logTag, "txStartAudio: getChannelForStreamId failed streamId %d",streamId );
			return -1;
	}
}

void WebrtcAudioProvider::txClose( int groupId, int streamId ) {
	AutoLock lock(m_lock);
	LOG_WEBRTC_INFO( logTag, "txCloseAudio: groupId=%d, streamId=%d", groupId, streamId);
	int channel = getChannelForStreamId( streamId );
	if ( channel >= 0 ) {
		WebrtcAudioStreamPtr stream = getStream(streamId);
		if(stream != NULL)
			stream->isTxStarted = false;
		voeBase->StopSend( channel );
		LOG_WEBRTC_DEBUG( logTag, "txCloseAudio: Stop transmit on channel %d", channel );
	}
	else {
		LOG_WEBRTC_ERROR( logTag, "txClose: getChannelForStreamId failed streanId %d",streamId );

	}
}

int WebrtcAudioProvider::toneStart( ToneType type, ToneDirection direction, int alertInfo, int groupId, int streamId, bool useBackup ) {
	AutoLock lock(m_lock);
	LOG_WEBRTC_INFO( logTag, "mediaToneStart: tone=%d, direction=%d, groupId=%d, streamId=%d", type, direction, groupId, streamId );
	if(toneGen != NULL) {
		LOG_WEBRTC_INFO( logTag, "mediaToneStart: tone already in progress - stop current tone [using dodgy parameters] and replace it." );
		toneStop(type, groupId, streamId);
	}
	toneGen = new WebrtcToneGenerator( type );
	voeBase->StartPlayout( localToneChannel );
	voeFile->StartPlayingFileLocally( localToneChannel, toneGen, webrtc::kFileFormatPcm8kHzFile);
	return 0;
}

int WebrtcAudioProvider::toneStop( ToneType type, int groupId, int streamId ) {
	
	LOG_WEBRTC_INFO( logTag, "mediaToneStop: tone=%d, groupId=%d, streamId=%d", type, groupId, streamId );
	if ( voeFile->IsPlayingFileLocally( localToneChannel ) == 1 ) {
		voeBase->StopPlayout( localToneChannel );
		voeFile->StopPlayingFileLocally( localToneChannel );
	}
	delete toneGen;
	toneGen = NULL;
	return 0;
}

int WebrtcAudioProvider::ringStart( int lineId, RingMode mode, bool once )
{
	AutoLock lock(m_lock);
	LOG_WEBRTC_INFO( logTag, "mediaRingStart: line=%d, mode=%d, once=%d", lineId, mode, once );
	if(ringGen != NULL) {
		LOG_WEBRTC_INFO( logTag, "mediaRingStart: ringing already in progress - do nothing." );
		return 0;
	}
	ringGen = new WebrtcRingGenerator( mode, once );
	ringGen->SetScaleFactor(ringerVolume);
	voeBase->StartPlayout( localRingChannel );
	voeFile->StartPlayingFileLocally( localRingChannel, ringGen, webrtc::kFileFormatPcm8kHzFile);
	return 0;
}

int WebrtcAudioProvider::ringStop( int lineId ) {
	AutoLock lock(m_lock);
	LOG_WEBRTC_INFO( logTag, "mediaRingStop: line=%d", lineId );
	if ( voeFile->IsPlayingFileLocally( localRingChannel ) == 1 ) {
		voeBase->StopPlayout( localRingChannel );
		voeFile->StopPlayingFileLocally( localRingChannel );
	}
	delete ringGen;
	ringGen = NULL;
	return 0;
}

int WebrtcAudioProvider::sendDtmf( int streamId, int digit) {
	AutoLock lock(m_lock);

	int rfc2833Payload=101;
    int channel = getChannelForStreamId( streamId );
	
	if(channel >= 0) {
		voeDTMF->SetDtmfFeedbackStatus(true);
		voeDTMF->SetSendTelephoneEventPayloadType(channel, rfc2833Payload); 
		voeDTMF->SendTelephoneEvent(channel, digit);
		return 0;
	}
    else {
    	LOG_WEBRTC_INFO( logTag, "sendDtmf() failed to map stream to channel");
    }

	return -1;
}


bool WebrtcAudioProvider::mute( int streamId, bool mute )
{
	AutoLock lock(m_lock);
    LOG_WEBRTC_INFO( logTag, "audio mute: streamId=%d, mute=%d", streamId, mute );
    int channel = getChannelForStreamId( streamId );
    bool returnVal = false;

    if ( channel >= 0 ) {
		if (voeVolumeControl->SetInputMute(channel, mute) != -1) {
			returnVal= true;
		}
		else {
			LOG_WEBRTC_INFO( logTag, "mute returned failure from SetInputMute");
		}
    }
    else {
    	LOG_WEBRTC_INFO( logTag, "failed to map stream to channel");
    }
    return returnVal;
}

bool WebrtcAudioProvider::isMuted( int streamId ) {
	AutoLock lock(m_lock);
	bool mute=false;

	voeVolumeControl->GetInputMute(getChannelForStreamId(streamId), mute);
	return mute;
}

bool WebrtcAudioProvider::setDefaultVolume( int volume ) {
	AutoLock lock(m_lock);
	defaultVolume = volume;
    return true;
}

int WebrtcAudioProvider::getDefaultVolume() {
	AutoLock lock(m_lock);
    return defaultVolume;
}

bool WebrtcAudioProvider::setRingerVolume( int volume ) {
	AutoLock lock(m_lock);
	LOG_WEBRTC_INFO( logTag, "setRingerVolume: volume=%d", volume );
	if (voeVolumeControl->SetChannelOutputVolumeScaling(localRingChannel, volume * 0.01f) != -1) {
		ringerVolume = volume;
		if(ringGen != NULL) {
			ringGen->SetScaleFactor(ringerVolume);
		}
		return true;
	}
	else {
		LOG_WEBRTC_INFO( logTag, "setRingerVolume() returned failure from SetChannelOutputVolumeScaling");
		return false;
	}
}

int WebrtcAudioProvider::getRingerVolume() {
	AutoLock lock(m_lock);
    return ringerVolume;
}

bool WebrtcAudioProvider::setVolume( int streamId, int volume ) {
	LOG_WEBRTC_INFO( logTag, "setVolume: streamId=%d, volume=%d", streamId, volume );
	int channel = getChannelForStreamId( streamId );
	bool returnVal = false;

	if ( channel >= 0 ) {
		
		if (voeVolumeControl->SetChannelOutputVolumeScaling(channel, volume * 0.01f) != -1) {
			returnVal = true;
		}
		else {
			LOG_WEBRTC_INFO( logTag, "setVolume() retuned failure from SetChannelOutputVolumeScaling");
		}
	}
	else {
		LOG_WEBRTC_INFO( logTag, "failed to map stream to channel");
	}
	return returnVal;
}

int  WebrtcAudioProvider::getVolume( int streamId ) {
	AutoLock lock(m_lock);
	float voeVolume = 0;

	if(voeVolumeControl->GetChannelOutputVolumeScaling(getChannelForStreamId(streamId), voeVolume) != -1) {
		
		float volume = voeVolume * 100.0f; 
		return (int)(volume + 0.5f);		
	}
	else {
		LOG_WEBRTC_INFO( logTag, "getVolume retuned failure from GetChannelOutputVolumeScaling");
		return -1;
	}
}


void WebrtcAudioProvider::Print(const webrtc::TraceLevel level, const char* message, const int length) {
	if (strstr(message, "eventNumber=") != NULL || strstr(message, "DTMF event ") != NULL)
		return;
}

void WebrtcAudioProvider::CallbackOnError(const int errCode, const int channel) {
	LOG_WEBRTC_ERROR( logTag, "CallbackOnError() ERROR %d on channel %d", errCode, channel );
}

void WebrtcAudioProvider::OnPeriodicDeadOrAlive(int channel, bool isAlive) {
	WebrtcAudioStreamPtr stream = getStreamByChannel(channel);
	if(stream != NULL && (stream->isRxStarted || stream->isTxStarted)) {
		if(stream->isAlive != isAlive) {
			LOG_WEBRTC_INFO( logTag, "voe channel %d is %s", channel, (isAlive ? "alive" : "dead") );
			stream->isAlive = isAlive;
			










		}
	}
}
#ifdef WIN32 
bool IsUserAdmin() {
	BOOL bAdm = TRUE;
	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	PSID AdministratorsGroup; 
	bAdm = AllocateAndInitializeSid(
									&NtAuthority,
									2,
									SECURITY_BUILTIN_DOMAIN_RID,
									DOMAIN_ALIAS_RID_ADMINS,
									0, 0, 0, 0, 0, 0,
									&AdministratorsGroup);
	if(bAdm)  {
		if (!CheckTokenMembership( NULL, AdministratorsGroup, &bAdm)) {
			bAdm = FALSE;
		} 
		
		FreeSid(AdministratorsGroup); 
	}
	return (bAdm == TRUE);
}

bool IsVistaOrNewer()
{
    OSVERSIONINFOEX osVersion;
	ZeroMemory(&osVersion, sizeof(OSVERSIONINFOEX));
    osVersion.dwOSVersionInfoSize   = sizeof(osVersion);

    if (GetVersionEx((LPOSVERSIONINFO)&osVersion)) {

        
        if (osVersion.dwMajorVersion >= 6) {
            
            return TRUE;
        }
    } 

	
    return FALSE;
}
#endif
} 

#endif

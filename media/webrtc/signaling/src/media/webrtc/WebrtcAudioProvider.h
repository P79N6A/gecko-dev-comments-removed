






































#ifndef WebrtcAUDIOMEDIAPROVIDER_H_
#define WebrtcAUDIOMEDIAPROVIDER_H_

#ifndef _USE_CPVE

#include "csf_common.h"
#include "CSFAudioControl.h"
#include "CSFAudioTermination.h"
#include "WebrtcAudioCodecSelector.h"

#include "voe_base.h"
#include "voe_file.h"
#include "voe_hardware.h"
#include "voe_dtmf.h"
#include "voe_network.h"
#include "voe_audio_processing.h"
#include "voe_volume_control.h"
#include "voe_encryption.h"
#include <string>
#include <map>
#include "base/lock.h"


namespace CSF
{
    class WebrtcMediaProvider;
    class WebrtcToneGenerator;
    class WebrtcRingGenerator;
    class WebrtcAudioStream;
    class WebrtcVideoProvider;
    DECLARE_PTR(WebrtcAudioStream);

    class WebrtcAudioProvider : public AudioControl, AudioTermination, webrtc::VoiceEngineObserver,
            webrtc::VoEConnectionObserver
            ,webrtc::TraceCallback {
    friend class WebrtcVideoProvider;

    public:

        WebrtcAudioProvider( WebrtcMediaProvider* provider );

        
        ~WebrtcAudioProvider();

        
        
        int init();

        
        AudioControl* getAudioControl() { return this; }

        std::vector<std::string> getRecordingDevices();
        std::vector<std::string> getPlayoutDevices();

        std::string getRecordingDevice() { return recordingDevice; }
        std::string getPlayoutDevice() { return playoutDevice; }

        bool setRecordingDevice( const std::string& name );
        bool setPlayoutDevice( const std::string& name );

        bool setDefaultVolume( int volume );
        int getDefaultVolume();

        bool setRingerVolume( int volume );
        int getRingerVolume();

        bool setVolume( int streamId, int volume );
        int  getVolume( int streamId );

        AudioTermination * getAudioTermination() { return this; }

        int  getCodecList( CodecRequestType requestType );

        int  rxAlloc    ( int groupId, int streamId, int requestedPort );
        void setLocalIP    ( const char* addr ) { localIP = addr; }
        int  rxOpen        ( int groupId, int streamId, int requestedPort, int listenIp, bool isMulticast );
        int  rxStart    ( int groupId, int streamId, int payloadType, int packPeriod, int localPort, int rfc2833PayloadType,
                                EncryptionAlgorithm algorithm, unsigned char* key, int keyLen, unsigned char* salt, int saltLen, int mode, int party );
        void rxClose    ( int groupId, int streamId);
        void rxRelease    ( int groupId, int streamId, int port );
        int  txStart    ( int groupId, int streamId, int payloadType, int packPeriod, bool vad, short tos,
                                char* remoteIpAddr, int remotePort, int rfc2833PayloadType, EncryptionAlgorithm algorithm,
                                unsigned char* key, int keyLen, unsigned char* salt, int saltLen, int mode, int party );
        void txClose    ( int groupId, int streamId);

        int  toneStart    ( ToneType type, ToneDirection direction, int alertInfo, int groupId, int streamId, bool useBackup );
        int  toneStop    ( ToneType type, int groupId, int streamId);
        int  ringStart    ( int lineId, RingMode mode, bool once );
        int  ringStop    ( int lineId );
        int  sendDtmf    ( int streamId, int digit);
        bool  mute        ( int streamId, bool mute );
        bool isMuted    ( int streamId );
        void setMediaPorts( int startPort, int endPort ) { this->startPort = startPort; this->endPort = endPort; }
        void setDSCPValue (int value){this->DSCPValue = value;}
        void setVADEnabled(bool VADEnabled){this->VADEnabled = VADEnabled;}

        
        webrtc::VoiceEngine* getVoiceEngine() { return voeVoice; }

    protected:
        
        void CallbackOnError(const int errCode, const int channel);
        void Print(const webrtc::TraceLevel level, const char* message, const int length);
        
        void OnPeriodicDeadOrAlive(int channel, bool alive);

        int getChannelForStreamId( int streamId );
        WebrtcAudioStreamPtr getStream( int streamId );
        WebrtcAudioStreamPtr getStreamByChannel( int channelId );

    private:
        WebrtcMediaProvider* provider;
        webrtc::VoiceEngine* voeVoice;
        webrtc::VoEBase* voeBase;
        webrtc::VoEFile* voeFile;
        webrtc::VoEHardware* voeHw;
        webrtc::VoEDtmf* voeDTMF;
        webrtc::VoENetwork* voeNetwork;
        webrtc::VoEVolumeControl* voeVolumeControl;
        webrtc::VoEAudioProcessing* voeVoiceQuality; 
        webrtc::VoEEncryption* voeEncryption;
        int localToneChannel;
        int localRingChannel;
        std::string recordingDevice;
        std::string playoutDevice;
        std::map<int, int> streamToChannel;
        std::map<int, WebrtcAudioStreamPtr> streamMap;
        WebrtcToneGenerator* toneGen;    
        WebrtcRingGenerator* ringGen;    
        std::string localIP;
        int startPort;
        int endPort;
        int defaultVolume;  
        int ringerVolume;   
        int DSCPValue;
        bool VADEnabled;

        WebrtcAudioCodecSelector codecSelector;

        
        

        Lock m_lock;
        
        
        
        Lock streamMapMutex;
        bool stopping;
    };
    const unsigned short targetLeveldBOvdefault =3 ;
    const unsigned short digitalCompressionGaindBdefault =9;
    const bool limiterEnableon = true;
} 

#endif
#endif 

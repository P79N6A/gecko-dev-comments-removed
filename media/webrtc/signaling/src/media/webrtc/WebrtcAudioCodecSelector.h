






































#ifndef WEBRTCAUDIOCODECSELECTOR_H_
#define WEBRTCAUDIOCODECSELECTOR_H_

#ifndef _USE_CPVE

#include <CSFAudioTermination.h>
#include "voe_base.h"
#include "voe_codec.h"
#include <map>


typedef enum {
  WebrtcAudioPayloadType_PCMU = 0,
  WebrtcAudioPayloadType_PCMA = 8,
  WebrtcAudioPayloadType_G722 = 9,
  WebrtcAudioPayloadType_iLBC = 102,
  WebrtcAudioPayloadType_ISAC = 103,
  WebrtcAudioPayloadType_TELEPHONE_EVENT = 106,
  WebrtcAudioPayloadType_ISACLC = 119,
  WebrtcAudioPayloadType_DIM = -1

} WebrtcAudioPayloadType;

const int ComfortNoisePayloadType = 13;
const int SamplingFreq8000Hz =8000;
const int SamplingFreq16000Hz =16000;
const int SamplingFreq32000Hz =32000;

namespace CSF
{


class WebrtcAudioCodecSelector
{
public:
	
	WebrtcAudioCodecSelector();

	
	~WebrtcAudioCodecSelector();

	int init( webrtc::VoiceEngine* voeVoice, bool useLowBandwidthCodecOnly, bool advertiseG722Codec );

	void release();

	
	int  advertiseCodecs( CodecRequestType requestType );

	
	
	int select( int payloadType, int dynamicPayloadType, int packetSize, webrtc::CodecInst& selectedCoded );

	
	
	int setSend(int channel, const webrtc::CodecInst& codec,int payloadType,bool vad);

	
	
	int setReceive(int channel, const webrtc::CodecInst& codec);

private:
	
	webrtc::VoECodec* voeCodec;

	std::map<int, webrtc::CodecInst*> codecMap;
};

} 

#endif
#endif 











#ifndef WEBRTC_MODULES_AUDIO_CONFERENCE_MIXER_SOURCE_AUDIO_FRAME_MANIPULATOR_H_
#define WEBRTC_MODULES_AUDIO_CONFERENCE_MIXER_SOURCE_AUDIO_FRAME_MANIPULATOR_H_

namespace webrtc {
class AudioFrame;


void CalculateEnergy(AudioFrame& audioFrame);


void RampIn(AudioFrame& audioFrame);
void RampOut(AudioFrame& audioFrame);

} 

#endif 

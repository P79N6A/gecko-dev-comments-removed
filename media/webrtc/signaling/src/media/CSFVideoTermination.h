






































#ifndef CSFVIDEOMEDIATERMINATION_H_
#define CSFVIDEOMEDIATERMINATION_H_

#include <CSFMediaTermination.h>
#include <CSFVideoControl.h>

typedef enum
{
	VideoCodecMask_H264 = 1,
	VideoCodecMask_H263 = 2

} VideoCodecMask;

#if __cplusplus

namespace CSF
{
	class VideoTermination : public MediaTermination
	{
	public:
		virtual void setRemoteWindow( int streamId, VideoWindowHandle window) = 0;
		virtual int setExternalRenderer( int streamId, VideoFormat videoFormat, ExternalRendererHandle render) = 0;
		virtual void sendIFrame	( int streamId ) = 0;
		virtual bool  mute		( int streamId, bool mute ) = 0;
		virtual void setAudioStreamId( int streamId) = 0;
	};

} 

#endif 

#endif 

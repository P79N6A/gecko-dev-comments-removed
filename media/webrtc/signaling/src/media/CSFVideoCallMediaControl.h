






































#ifndef CSFVIDEOCALLMEDIACONTROL_H_
#define CSFVIDEOCALLMEDIACONTROL_H_

#include <csf/CSFVideoMediaControl.h>

#if __cplusplus

namespace CSF
{
	class VideoCallMediaControl
	{
	public:
		virtual void setVideoMode( VideoEnableMode mode ) = 0;

		
		virtual void setRemoteWindow( VideoWindowHandle window ) = 0;
		virtual void showRemoteWindow( bool show ) = 0;
	};

} 

#endif 

#endif 

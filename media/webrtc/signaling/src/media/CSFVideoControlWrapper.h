



#ifndef CSFVIDEOCONTROLWRAPPER_H_
#define CSFVIDEOCONTROLWRAPPER_H_

#include "CC_Common.h"
#include "CSFVideoControl.h"

#if __cplusplus

#include <string>
#include <vector>

namespace CSF
{
    DECLARE_NS_PTR(VideoControlWrapper)
    typedef void *VideoWindowHandle;

	class ECC_API VideoControlWrapper : public VideoControl
	{
	public:
		VideoControlWrapper(VideoControl * videoControl){_realVideoControl = videoControl;};

		virtual void setVideoMode( bool enable );

		
		virtual void setPreviewWindow( VideoWindowHandle window, int top, int left, int bottom, int right, RenderScaling style );
		virtual void showPreviewWindow( bool show );

		
		virtual std::vector<std::string> getCaptureDevices();

		virtual std::string getCaptureDevice();
		virtual bool setCaptureDevice( const std::string& name );

		virtual void setVideoControl( VideoControl * videoControl ){_realVideoControl = videoControl;};

	private:
		VideoControl * _realVideoControl;
	};

} 

#endif 

#endif 

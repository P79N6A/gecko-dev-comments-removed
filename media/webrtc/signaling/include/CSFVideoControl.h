






































#pragma once

#include "CC_Common.h"
#include "ECC_Types.h"

#include <string>
#include <vector>

namespace CSF
{
	DECLARE_PTR(VideoControl);
	class ECC_API VideoControl
	{
	public:
		virtual void setVideoMode( bool enable ) = 0;

		
		virtual void setPreviewWindow( VideoWindowHandle window, int top, int left, int bottom, int right, RenderScaling style ) = 0;
		virtual void showPreviewWindow( bool show ) = 0;

		
		virtual std::vector<std::string> getCaptureDevices() = 0;

		virtual std::string getCaptureDevice() = 0;
		virtual bool setCaptureDevice( const std::string& name ) = 0;
	};

}; 

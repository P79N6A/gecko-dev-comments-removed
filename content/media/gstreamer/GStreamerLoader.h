




#ifndef GStreamerLoader_h_
#define GStreamerLoader_h_

#include <gst/gst.h>
#include <gst/gstbuffer.h>
#include <gst/gstelementfactory.h>
#include <gst/gststructure.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <gst/video/video.h>

namespace mozilla {





bool load_gstreamer();





#define GST_FUNC(_, func) extern typeof(::func)* func;
#define REPLACE_FUNC(func) GST_FUNC(-1, func)
#include "GStreamerFunctionList.h"
#undef GST_FUNC
#undef REPLACE_FUNC

}

#endif 











#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_DESKTOP_CAPTURE_TYPES_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_DESKTOP_CAPTURE_TYPES_H_

#include <stdint.h>

#include "webrtc/modules/desktop_capture/desktop_geometry.h"
#include "webrtc/typedefs.h"

namespace webrtc {





typedef intptr_t WindowId;

const WindowId kNullWindowId = 0;





typedef intptr_t ScreenId;


const ScreenId kFullDesktopScreenId = -1;

const ScreenId kInvalidScreenId = -2;

}  

#endif  


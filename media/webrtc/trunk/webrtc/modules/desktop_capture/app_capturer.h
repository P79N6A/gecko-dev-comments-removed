









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_APP_CAPTURER_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_APP_CAPTURER_H_

#include <vector>
#include <string>

#include "webrtc/modules/desktop_capture/desktop_capture_types.h"
#include "webrtc/modules/desktop_capture/desktop_capturer.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class DesktopCaptureOptions;

class AppCapturer : public DesktopCapturer {
public:
    typedef webrtc::ProcessId ProcessId;
    struct App {
        ProcessId id;
        
        std::string title;
    };
    typedef std::vector<App> AppList;

    static AppCapturer* Create(const DesktopCaptureOptions& options);
    static AppCapturer* Create();

    virtual ~AppCapturer() {}

    
    virtual bool GetAppList(AppList* apps) = 0;
    virtual bool SelectApp(ProcessId id) = 0;
    virtual bool BringAppToFront() = 0;
};

}  

#endif  


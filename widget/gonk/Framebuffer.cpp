
















#include "Framebuffer.h"

#include "android/log.h"
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>

#include "nsSize.h"
#include "mozilla/FileUtils.h"

#define LOG(args...) __android_log_print(ANDROID_LOG_INFO, "Gonk" , ## args)

using namespace std;

namespace mozilla {
namespace Framebuffer {

static size_t sMappedSize;
static struct fb_var_screeninfo sVi;
static gfxIntSize *sScreenSize = nullptr;

bool
GetSize(nsIntSize *aScreenSize) {
    
    if (sScreenSize) {
        *aScreenSize = *sScreenSize;
        return true;
    }

    ScopedClose fd(open("/dev/graphics/fb0", O_RDWR));
    if (0 > fd.get()) {
        LOG("Error opening framebuffer device");
        return false;
    }

    if (0 > ioctl(fd.get(), FBIOGET_VSCREENINFO, &sVi)) {
        LOG("Error getting variable screeninfo");
        return false;
    }

    sScreenSize = new gfxIntSize(sVi.xres, sVi.yres);
    *aScreenSize = *sScreenSize;
    return true;
}

} 
} 








































#include <fcntl.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include <vector>

#include "android/log.h"

#include "Framebuffer.h"
#include "gfxImageSurface.h"
#include "mozilla/FileUtils.h"
#include "nsTArray.h"

#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "Gonk" , ## args)

using namespace std;

namespace mozilla {

namespace Framebuffer {

static int sFd = -1;
static size_t sMappedSize;
static struct fb_var_screeninfo sVi;
static size_t sActiveBuffer;
typedef vector<nsRefPtr<gfxImageSurface> > BufferVector;
BufferVector* sBuffers;

BufferVector& Buffers() { return *sBuffers; }

bool
SetGraphicsMode()
{
    ScopedClose fd(open("/dev/tty0", O_RDWR | O_SYNC));
    if (0 > fd.mFd) {
        
        LOG("No /dev/tty0?");
    } else if (ioctl(fd.mFd, KDSETMODE, (void*) KD_GRAPHICS)) {
        LOG("Error setting graphics mode on /dev/tty0");
        return false;
    }
    return true;
}

bool
Open(nsIntSize* aScreenSize)
{
    if (0 <= sFd)
        return true;

    if (!SetGraphicsMode())
        return false;

    ScopedClose fd(open("/dev/graphics/fb0", O_RDWR));
    if (0 > fd.mFd) {
        LOG("Error opening framebuffer device");
        return false;
    }

    struct fb_fix_screeninfo fi;
    if (0 > ioctl(fd.mFd, FBIOGET_FSCREENINFO, &fi)) {
        LOG("Error getting fixed screeninfo");
        return false;
    }

    if (0 > ioctl(fd.mFd, FBIOGET_VSCREENINFO, &sVi)) {
        LOG("Error getting variable screeninfo");
        return false;
    }

    sMappedSize = fi.smem_len;
    void* mem = mmap(0, sMappedSize, PROT_READ | PROT_WRITE, MAP_SHARED,
                     fd.mFd, 0);
    if (MAP_FAILED == mem) {
        LOG("Error mmap'ing framebuffer");
        return false;
    }

    sFd = fd.mFd;
    fd.mFd = -1;

    
    
    
    gfxASurface::gfxImageFormat format = gfxASurface::ImageFormatRGB16_565;
    int bytesPerPixel = gfxASurface::BytePerPixelFromFormat(format);
    gfxIntSize size(sVi.xres, sVi.yres);
    long stride = size.width * bytesPerPixel;
    size_t numFrameBytes = stride * size.height;

    sBuffers = new BufferVector(2);
    unsigned char* data = static_cast<unsigned char*>(mem);
    for (size_t i = 0; i < 2; ++i, data += numFrameBytes) {
      memset(data, 0, numFrameBytes);
      Buffers()[i] = new gfxImageSurface(data, size, stride, format);
    }

    
    Present();

    *aScreenSize = size;
    return true;
}

void
Close()
{
    if (0 > sFd)
        return;

    munmap(Buffers()[0]->Data(), sMappedSize);
    delete sBuffers;
    sBuffers = NULL;

    close(sFd);
    sFd = -1;
}

gfxASurface*
BackBuffer()
{
    return Buffers()[!sActiveBuffer];
}

void
Present()
{
    sActiveBuffer = !sActiveBuffer;

    sVi.yres_virtual = sVi.yres * 2;
    sVi.yoffset = sActiveBuffer * sVi.yres;
    sVi.bits_per_pixel = 16;
    if (ioctl(sFd, FBIOPUT_VSCREENINFO, &sVi) < 0) {
        LOG("Error presenting front buffer");
    }
}

} 

} 

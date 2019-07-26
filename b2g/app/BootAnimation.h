#ifndef BOOTANIMATION_H
#define BOOTANIMATION_H

namespace android {
class FramebufferNativeWindow;
}



__attribute__ ((weak))
android::FramebufferNativeWindow* NativeWindow();


__attribute__ ((weak))
void StopBootAnimation();

#endif 

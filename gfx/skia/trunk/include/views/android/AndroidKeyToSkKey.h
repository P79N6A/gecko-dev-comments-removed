








#ifndef _ANDROID_TO_SKIA_KEYCODES_H
#define _ANDROID_TO_SKIA_KEYCODES_H

#include "android/keycodes.h"
#include "SkKey.h"



SkKey AndroidKeycodeToSkKey(int keycode) {
    switch (keycode) {
        case AKEYCODE_DPAD_LEFT:
            return kLeft_SkKey;
        case AKEYCODE_DPAD_RIGHT:
            return kRight_SkKey;
        case AKEYCODE_DPAD_UP:
            return kUp_SkKey;
        case AKEYCODE_DPAD_DOWN:
            return kDown_SkKey;
        case AKEYCODE_BACK:
            return kBack_SkKey;
        default:
            return kNONE_SkKey;
    }
}

#endif

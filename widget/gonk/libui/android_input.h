















#ifndef _ANDROID_INPUT_H
#define _ANDROID_INPUT_H
























#include <stdint.h>
#include <sys/types.h>
#include "android_keycodes.h"
#include <android/looper.h>

#ifdef __cplusplus
extern "C" {
#endif





enum {
    
    AKEY_STATE_UNKNOWN = -1,

    
    AKEY_STATE_UP = 0,

    
    AKEY_STATE_DOWN = 1,

    
    AKEY_STATE_VIRTUAL = 2
};




enum {
    
    AMETA_NONE = 0,

    
    AMETA_ALT_ON = 0x02,

    
    AMETA_ALT_LEFT_ON = 0x10,

    
    AMETA_ALT_RIGHT_ON = 0x20,

    
    AMETA_SHIFT_ON = 0x01,

    
    AMETA_SHIFT_LEFT_ON = 0x40,

    
    AMETA_SHIFT_RIGHT_ON = 0x80,

    
    AMETA_SYM_ON = 0x04,

    
    AMETA_FUNCTION_ON = 0x08,

    
    AMETA_CTRL_ON = 0x1000,

    
    AMETA_CTRL_LEFT_ON = 0x2000,

    
    AMETA_CTRL_RIGHT_ON = 0x4000,

    
    AMETA_META_ON = 0x10000,

    
    AMETA_META_LEFT_ON = 0x20000,

    
    AMETA_META_RIGHT_ON = 0x40000,

    
    AMETA_CAPS_LOCK_ON = 0x100000,

    
    AMETA_NUM_LOCK_ON = 0x200000,

    
    AMETA_SCROLL_LOCK_ON = 0x400000,
};







struct AInputEvent;
typedef struct AInputEvent AInputEvent;




enum {
    
    AINPUT_EVENT_TYPE_KEY = 1,

    
    AINPUT_EVENT_TYPE_MOTION = 2
};




enum {
    
    AKEY_EVENT_ACTION_DOWN = 0,

    
    AKEY_EVENT_ACTION_UP = 1,

    



    AKEY_EVENT_ACTION_MULTIPLE = 2
};




enum {
    
    AKEY_EVENT_FLAG_WOKE_HERE = 0x1,

    
    AKEY_EVENT_FLAG_SOFT_KEYBOARD = 0x2,

    
    AKEY_EVENT_FLAG_KEEP_TOUCH_MODE = 0x4,

    


    AKEY_EVENT_FLAG_FROM_SYSTEM = 0x8,

    




    AKEY_EVENT_FLAG_EDITOR_ACTION = 0x10,

    







    AKEY_EVENT_FLAG_CANCELED = 0x20,

    


    AKEY_EVENT_FLAG_VIRTUAL_HARD_KEY = 0x40,

    

    AKEY_EVENT_FLAG_LONG_PRESS = 0x80,

    

    AKEY_EVENT_FLAG_CANCELED_LONG_PRESS = 0x100,

    



    AKEY_EVENT_FLAG_TRACKING = 0x200,

    





    AKEY_EVENT_FLAG_FALLBACK = 0x400,
};








#define AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT 8

enum {
    

    AMOTION_EVENT_ACTION_MASK = 0xff,

    




    AMOTION_EVENT_ACTION_POINTER_INDEX_MASK  = 0xff00,

    

    AMOTION_EVENT_ACTION_DOWN = 0,

    


    AMOTION_EVENT_ACTION_UP = 1,

    



    AMOTION_EVENT_ACTION_MOVE = 2,

    



    AMOTION_EVENT_ACTION_CANCEL = 3,

    


    AMOTION_EVENT_ACTION_OUTSIDE = 4,

    


    AMOTION_EVENT_ACTION_POINTER_DOWN = 5,

    


    AMOTION_EVENT_ACTION_POINTER_UP = 6,

    



    AMOTION_EVENT_ACTION_HOVER_MOVE = 7,

    






    AMOTION_EVENT_ACTION_SCROLL = 8,

    

    AMOTION_EVENT_ACTION_HOVER_ENTER = 9,

    

    AMOTION_EVENT_ACTION_HOVER_EXIT = 10,
};




enum {
    








    AMOTION_EVENT_FLAG_WINDOW_IS_OBSCURED = 0x1,
};




enum {
    
    AMOTION_EVENT_EDGE_FLAG_NONE = 0,

    
    AMOTION_EVENT_EDGE_FLAG_TOP = 0x01,

    
    AMOTION_EVENT_EDGE_FLAG_BOTTOM = 0x02,

    
    AMOTION_EVENT_EDGE_FLAG_LEFT = 0x04,

    
    AMOTION_EVENT_EDGE_FLAG_RIGHT = 0x08
};





enum {
    AMOTION_EVENT_AXIS_X = 0,
    AMOTION_EVENT_AXIS_Y = 1,
    AMOTION_EVENT_AXIS_PRESSURE = 2,
    AMOTION_EVENT_AXIS_SIZE = 3,
    AMOTION_EVENT_AXIS_TOUCH_MAJOR = 4,
    AMOTION_EVENT_AXIS_TOUCH_MINOR = 5,
    AMOTION_EVENT_AXIS_TOOL_MAJOR = 6,
    AMOTION_EVENT_AXIS_TOOL_MINOR = 7,
    AMOTION_EVENT_AXIS_ORIENTATION = 8,
    AMOTION_EVENT_AXIS_VSCROLL = 9,
    AMOTION_EVENT_AXIS_HSCROLL = 10,
    AMOTION_EVENT_AXIS_Z = 11,
    AMOTION_EVENT_AXIS_RX = 12,
    AMOTION_EVENT_AXIS_RY = 13,
    AMOTION_EVENT_AXIS_RZ = 14,
    AMOTION_EVENT_AXIS_HAT_X = 15,
    AMOTION_EVENT_AXIS_HAT_Y = 16,
    AMOTION_EVENT_AXIS_LTRIGGER = 17,
    AMOTION_EVENT_AXIS_RTRIGGER = 18,
    AMOTION_EVENT_AXIS_THROTTLE = 19,
    AMOTION_EVENT_AXIS_RUDDER = 20,
    AMOTION_EVENT_AXIS_WHEEL = 21,
    AMOTION_EVENT_AXIS_GAS = 22,
    AMOTION_EVENT_AXIS_BRAKE = 23,
    AMOTION_EVENT_AXIS_DISTANCE = 24,
    AMOTION_EVENT_AXIS_TILT = 25,
    AMOTION_EVENT_AXIS_GENERIC_1 = 32,
    AMOTION_EVENT_AXIS_GENERIC_2 = 33,
    AMOTION_EVENT_AXIS_GENERIC_3 = 34,
    AMOTION_EVENT_AXIS_GENERIC_4 = 35,
    AMOTION_EVENT_AXIS_GENERIC_5 = 36,
    AMOTION_EVENT_AXIS_GENERIC_6 = 37,
    AMOTION_EVENT_AXIS_GENERIC_7 = 38,
    AMOTION_EVENT_AXIS_GENERIC_8 = 39,
    AMOTION_EVENT_AXIS_GENERIC_9 = 40,
    AMOTION_EVENT_AXIS_GENERIC_10 = 41,
    AMOTION_EVENT_AXIS_GENERIC_11 = 42,
    AMOTION_EVENT_AXIS_GENERIC_12 = 43,
    AMOTION_EVENT_AXIS_GENERIC_13 = 44,
    AMOTION_EVENT_AXIS_GENERIC_14 = 45,
    AMOTION_EVENT_AXIS_GENERIC_15 = 46,
    AMOTION_EVENT_AXIS_GENERIC_16 = 47,

    
    
};





enum {
    AMOTION_EVENT_BUTTON_PRIMARY = 1 << 0,
    AMOTION_EVENT_BUTTON_SECONDARY = 1 << 1,
    AMOTION_EVENT_BUTTON_TERTIARY = 1 << 2,
    AMOTION_EVENT_BUTTON_BACK = 1 << 3,
    AMOTION_EVENT_BUTTON_FORWARD = 1 << 4,
};





enum {
    AMOTION_EVENT_TOOL_TYPE_UNKNOWN = 0,
    AMOTION_EVENT_TOOL_TYPE_FINGER = 1,
    AMOTION_EVENT_TOOL_TYPE_STYLUS = 2,
    AMOTION_EVENT_TOOL_TYPE_MOUSE = 3,
    AMOTION_EVENT_TOOL_TYPE_ERASER = 4,
};







enum {
    AINPUT_SOURCE_CLASS_MASK = 0x000000ff,

    AINPUT_SOURCE_CLASS_BUTTON = 0x00000001,
    AINPUT_SOURCE_CLASS_POINTER = 0x00000002,
    AINPUT_SOURCE_CLASS_NAVIGATION = 0x00000004,
    AINPUT_SOURCE_CLASS_POSITION = 0x00000008,
    AINPUT_SOURCE_CLASS_JOYSTICK = 0x00000010,
};

enum {
    AINPUT_SOURCE_UNKNOWN = 0x00000000,

    AINPUT_SOURCE_KEYBOARD = 0x00000100 | AINPUT_SOURCE_CLASS_BUTTON,
    AINPUT_SOURCE_DPAD = 0x00000200 | AINPUT_SOURCE_CLASS_BUTTON,
    AINPUT_SOURCE_GAMEPAD = 0x00000400 | AINPUT_SOURCE_CLASS_BUTTON,
    AINPUT_SOURCE_TOUCHSCREEN = 0x00001000 | AINPUT_SOURCE_CLASS_POINTER,
    AINPUT_SOURCE_MOUSE = 0x00002000 | AINPUT_SOURCE_CLASS_POINTER,
    AINPUT_SOURCE_STYLUS = 0x00004000 | AINPUT_SOURCE_CLASS_POINTER,
    AINPUT_SOURCE_TRACKBALL = 0x00010000 | AINPUT_SOURCE_CLASS_NAVIGATION,
    AINPUT_SOURCE_TOUCHPAD = 0x00100000 | AINPUT_SOURCE_CLASS_POSITION,
    AINPUT_SOURCE_JOYSTICK = 0x01000000 | AINPUT_SOURCE_CLASS_JOYSTICK,

    AINPUT_SOURCE_ANY = 0xffffff00,
};






enum {
    AINPUT_KEYBOARD_TYPE_NONE = 0,
    AINPUT_KEYBOARD_TYPE_NON_ALPHABETIC = 1,
    AINPUT_KEYBOARD_TYPE_ALPHABETIC = 2,
};










enum {
    AINPUT_MOTION_RANGE_X = AMOTION_EVENT_AXIS_X,
    AINPUT_MOTION_RANGE_Y = AMOTION_EVENT_AXIS_Y,
    AINPUT_MOTION_RANGE_PRESSURE = AMOTION_EVENT_AXIS_PRESSURE,
    AINPUT_MOTION_RANGE_SIZE = AMOTION_EVENT_AXIS_SIZE,
    AINPUT_MOTION_RANGE_TOUCH_MAJOR = AMOTION_EVENT_AXIS_TOUCH_MAJOR,
    AINPUT_MOTION_RANGE_TOUCH_MINOR = AMOTION_EVENT_AXIS_TOUCH_MINOR,
    AINPUT_MOTION_RANGE_TOOL_MAJOR = AMOTION_EVENT_AXIS_TOOL_MAJOR,
    AINPUT_MOTION_RANGE_TOOL_MINOR = AMOTION_EVENT_AXIS_TOOL_MINOR,
    AINPUT_MOTION_RANGE_ORIENTATION = AMOTION_EVENT_AXIS_ORIENTATION,
} __attribute__ ((deprecated));












int32_t AInputEvent_getType(const AInputEvent* event);











int32_t AInputEvent_getDeviceId(const AInputEvent* event);


int32_t AInputEvent_getSource(const AInputEvent* event);




int32_t AKeyEvent_getAction(const AInputEvent* key_event);


int32_t AKeyEvent_getFlags(const AInputEvent* key_event);



int32_t AKeyEvent_getKeyCode(const AInputEvent* key_event);



int32_t AKeyEvent_getScanCode(const AInputEvent* key_event);


int32_t AKeyEvent_getMetaState(const AInputEvent* key_event);





int32_t AKeyEvent_getRepeatCount(const AInputEvent* key_event);






int64_t AKeyEvent_getDownTime(const AInputEvent* key_event);



int64_t AKeyEvent_getEventTime(const AInputEvent* key_event);




int32_t AMotionEvent_getAction(const AInputEvent* motion_event);


int32_t AMotionEvent_getFlags(const AInputEvent* motion_event);



int32_t AMotionEvent_getMetaState(const AInputEvent* motion_event);


int32_t AMotionEvent_getButtonState(const AInputEvent* motion_event);




int32_t AMotionEvent_getEdgeFlags(const AInputEvent* motion_event);



int64_t AMotionEvent_getDownTime(const AInputEvent* motion_event);



int64_t AMotionEvent_getEventTime(const AInputEvent* motion_event);





float AMotionEvent_getXOffset(const AInputEvent* motion_event);





float AMotionEvent_getYOffset(const AInputEvent* motion_event);




float AMotionEvent_getXPrecision(const AInputEvent* motion_event);




float AMotionEvent_getYPrecision(const AInputEvent* motion_event);



size_t AMotionEvent_getPointerCount(const AInputEvent* motion_event);





int32_t AMotionEvent_getPointerId(const AInputEvent* motion_event, size_t pointer_index);




int32_t AMotionEvent_getToolType(const AInputEvent* motion_event, size_t pointer_index);





float AMotionEvent_getRawX(const AInputEvent* motion_event, size_t pointer_index);





float AMotionEvent_getRawY(const AInputEvent* motion_event, size_t pointer_index);




float AMotionEvent_getX(const AInputEvent* motion_event, size_t pointer_index);




float AMotionEvent_getY(const AInputEvent* motion_event, size_t pointer_index);





float AMotionEvent_getPressure(const AInputEvent* motion_event, size_t pointer_index);







float AMotionEvent_getSize(const AInputEvent* motion_event, size_t pointer_index);



float AMotionEvent_getTouchMajor(const AInputEvent* motion_event, size_t pointer_index);



float AMotionEvent_getTouchMinor(const AInputEvent* motion_event, size_t pointer_index);





float AMotionEvent_getToolMajor(const AInputEvent* motion_event, size_t pointer_index);





float AMotionEvent_getToolMinor(const AInputEvent* motion_event, size_t pointer_index);









float AMotionEvent_getOrientation(const AInputEvent* motion_event, size_t pointer_index);


float AMotionEvent_getAxisValue(const AInputEvent* motion_event,
        int32_t axis, size_t pointer_index);





size_t AMotionEvent_getHistorySize(const AInputEvent* motion_event);



int64_t AMotionEvent_getHistoricalEventTime(AInputEvent* motion_event,
        size_t history_index);








float AMotionEvent_getHistoricalRawX(const AInputEvent* motion_event, size_t pointer_index,
        size_t history_index);








float AMotionEvent_getHistoricalRawY(const AInputEvent* motion_event, size_t pointer_index,
        size_t history_index);





float AMotionEvent_getHistoricalX(AInputEvent* motion_event, size_t pointer_index,
        size_t history_index);





float AMotionEvent_getHistoricalY(AInputEvent* motion_event, size_t pointer_index,
        size_t history_index);






float AMotionEvent_getHistoricalPressure(AInputEvent* motion_event, size_t pointer_index,
        size_t history_index);








float AMotionEvent_getHistoricalSize(AInputEvent* motion_event, size_t pointer_index,
        size_t history_index);




float AMotionEvent_getHistoricalTouchMajor(const AInputEvent* motion_event, size_t pointer_index,
        size_t history_index);




float AMotionEvent_getHistoricalTouchMinor(const AInputEvent* motion_event, size_t pointer_index,
        size_t history_index);






float AMotionEvent_getHistoricalToolMajor(const AInputEvent* motion_event, size_t pointer_index,
        size_t history_index);






float AMotionEvent_getHistoricalToolMinor(const AInputEvent* motion_event, size_t pointer_index,
        size_t history_index);










float AMotionEvent_getHistoricalOrientation(const AInputEvent* motion_event, size_t pointer_index,
        size_t history_index);



float AMotionEvent_getHistoricalAxisValue(const AInputEvent* motion_event,
        int32_t axis, size_t pointer_index, size_t history_index);








struct AInputQueue;
typedef struct AInputQueue AInputQueue;





void AInputQueue_attachLooper(AInputQueue* queue, ALooper* looper,
        int ident, ALooper_callbackFunc callback, void* data);




void AInputQueue_detachLooper(AInputQueue* queue);






int32_t AInputQueue_hasEvents(AInputQueue* queue);





int32_t AInputQueue_getEvent(AInputQueue* queue, AInputEvent** outEvent);









int32_t AInputQueue_preDispatchEvent(AInputQueue* queue, AInputEvent* event);





void AInputQueue_finishEvent(AInputQueue* queue, AInputEvent* event, int handled);

#ifdef __cplusplus
}
#endif

#endif

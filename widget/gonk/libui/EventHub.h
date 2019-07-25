
















#ifndef _RUNTIME_EVENT_HUB_H
#define _RUNTIME_EVENT_HUB_H

#include "Input.h"
#include "Keyboard.h"
#include "KeyLayoutMap.h"
#include "KeyCharacterMap.h"
#include "VirtualKeyMap.h"
#include <utils/String8.h>
#include <utils/threads.h>
#include <utils/Log.h>
#include <utils/threads.h>
#include <utils/List.h>
#include <utils/Errors.h>
#include <utils/PropertyMap.h>
#include <utils/Vector.h>
#include <utils/KeyedVector.h>

#include <linux/input.h>
#include <sys/epoll.h>



#define BTN_FIRST 0x100  // first button scancode
#define BTN_LAST 0x15f   // last button scancode

namespace android {




struct RawEvent {
    nsecs_t when;
    int32_t deviceId;
    int32_t type;
    int32_t scanCode;
    int32_t keyCode;
    int32_t value;
    uint32_t flags;
};


struct RawAbsoluteAxisInfo {
    bool valid; 

    int32_t minValue;  
    int32_t maxValue;  
    int32_t flat;      
    int32_t fuzz;      
    int32_t resolution; 

    inline void clear() {
        valid = false;
        minValue = 0;
        maxValue = 0;
        flat = 0;
        fuzz = 0;
        resolution = 0;
    }
};




enum {
    
    INPUT_DEVICE_CLASS_KEYBOARD      = 0x00000001,

    
    INPUT_DEVICE_CLASS_ALPHAKEY      = 0x00000002,

    
    INPUT_DEVICE_CLASS_TOUCH         = 0x00000004,

    
    INPUT_DEVICE_CLASS_CURSOR        = 0x00000008,

    
    INPUT_DEVICE_CLASS_TOUCH_MT      = 0x00000010,

    
    INPUT_DEVICE_CLASS_DPAD          = 0x00000020,

    
    INPUT_DEVICE_CLASS_GAMEPAD       = 0x00000040,

    
    INPUT_DEVICE_CLASS_SWITCH        = 0x00000080,

    
    INPUT_DEVICE_CLASS_JOYSTICK      = 0x00000100,

    
    INPUT_DEVICE_CLASS_EXTERNAL      = 0x80000000,
};





extern uint32_t getAbsAxisUsage(int32_t axis, uint32_t deviceClasses);














class EventHubInterface : public virtual RefBase {
protected:
    EventHubInterface() { }
    virtual ~EventHubInterface() { }

public:
    
    enum {
        
        DEVICE_ADDED = 0x10000000,
        
        DEVICE_REMOVED = 0x20000000,
        
        
        FINISHED_DEVICE_SCAN = 0x30000000,

        FIRST_SYNTHETIC_EVENT = DEVICE_ADDED,
    };

    virtual uint32_t getDeviceClasses(int32_t deviceId) const = 0;

    virtual String8 getDeviceName(int32_t deviceId) const = 0;

    virtual void getConfiguration(int32_t deviceId, PropertyMap* outConfiguration) const = 0;

    virtual status_t getAbsoluteAxisInfo(int32_t deviceId, int axis,
            RawAbsoluteAxisInfo* outAxisInfo) const = 0;

    virtual bool hasRelativeAxis(int32_t deviceId, int axis) const = 0;

    virtual bool hasInputProperty(int32_t deviceId, int property) const = 0;

    virtual status_t mapKey(int32_t deviceId, int scancode,
            int32_t* outKeycode, uint32_t* outFlags) const = 0;

    virtual status_t mapAxis(int32_t deviceId, int scancode,
            AxisInfo* outAxisInfo) const = 0;

    
    
    virtual void setExcludedDevices(const Vector<String8>& devices) = 0;

    











    virtual size_t getEvents(int timeoutMillis, RawEvent* buffer, size_t bufferSize) = 0;

    


    virtual int32_t getScanCodeState(int32_t deviceId, int32_t scanCode) const = 0;
    virtual int32_t getKeyCodeState(int32_t deviceId, int32_t keyCode) const = 0;
    virtual int32_t getSwitchState(int32_t deviceId, int32_t sw) const = 0;
    virtual status_t getAbsoluteAxisValue(int32_t deviceId, int32_t axis,
            int32_t* outValue) const = 0;

    


    virtual bool markSupportedKeyCodes(int32_t deviceId, size_t numCodes, const int32_t* keyCodes,
            uint8_t* outFlags) const = 0;

    virtual bool hasScanCode(int32_t deviceId, int32_t scanCode) const = 0;
    virtual bool hasLed(int32_t deviceId, int32_t led) const = 0;
    virtual void setLedState(int32_t deviceId, int32_t led, bool on) = 0;

    virtual void getVirtualKeyDefinitions(int32_t deviceId,
            Vector<VirtualKeyDefinition>& outVirtualKeys) const = 0;

    virtual String8 getKeyCharacterMapFile(int32_t deviceId) const = 0;

    
    virtual void requestReopenDevices() = 0;

    
    virtual void wake() = 0;

    
    virtual void dump(String8& dump) = 0;

    
    virtual void monitor() = 0;
};

class EventHub : public EventHubInterface
{
public:
    EventHub();

    virtual uint32_t getDeviceClasses(int32_t deviceId) const;

    virtual String8 getDeviceName(int32_t deviceId) const;

    virtual void getConfiguration(int32_t deviceId, PropertyMap* outConfiguration) const;

    virtual status_t getAbsoluteAxisInfo(int32_t deviceId, int axis,
            RawAbsoluteAxisInfo* outAxisInfo) const;

    virtual bool hasRelativeAxis(int32_t deviceId, int axis) const;

    virtual bool hasInputProperty(int32_t deviceId, int property) const;

    virtual status_t mapKey(int32_t deviceId, int scancode,
            int32_t* outKeycode, uint32_t* outFlags) const;

    virtual status_t mapAxis(int32_t deviceId, int scancode,
            AxisInfo* outAxisInfo) const;

    virtual void setExcludedDevices(const Vector<String8>& devices);

    virtual int32_t getScanCodeState(int32_t deviceId, int32_t scanCode) const;
    virtual int32_t getKeyCodeState(int32_t deviceId, int32_t keyCode) const;
    virtual int32_t getSwitchState(int32_t deviceId, int32_t sw) const;
    virtual status_t getAbsoluteAxisValue(int32_t deviceId, int32_t axis, int32_t* outValue) const;

    virtual bool markSupportedKeyCodes(int32_t deviceId, size_t numCodes,
            const int32_t* keyCodes, uint8_t* outFlags) const;

    virtual size_t getEvents(int timeoutMillis, RawEvent* buffer, size_t bufferSize);

    virtual bool hasScanCode(int32_t deviceId, int32_t scanCode) const;
    virtual bool hasLed(int32_t deviceId, int32_t led) const;
    virtual void setLedState(int32_t deviceId, int32_t led, bool on);

    virtual void getVirtualKeyDefinitions(int32_t deviceId,
            Vector<VirtualKeyDefinition>& outVirtualKeys) const;

    virtual String8 getKeyCharacterMapFile(int32_t deviceId) const;

    virtual void requestReopenDevices();

    virtual void wake();

    virtual void dump(String8& dump);
    virtual void monitor();

protected:
    virtual ~EventHub();

private:
    struct Device {
        Device* next;

        int fd;
        const int32_t id;
        const String8 path;
        const InputDeviceIdentifier identifier;

        uint32_t classes;

        uint8_t keyBitmask[(KEY_MAX + 1) / 8];
        uint8_t absBitmask[(ABS_MAX + 1) / 8];
        uint8_t relBitmask[(REL_MAX + 1) / 8];
        uint8_t swBitmask[(SW_MAX + 1) / 8];
        uint8_t ledBitmask[(LED_MAX + 1) / 8];
        uint8_t propBitmask[(INPUT_PROP_MAX + 1) / 8];

        String8 configurationFile;
        PropertyMap* configuration;
        VirtualKeyMap* virtualKeyMap;
        KeyMap keyMap;

        Device(int fd, int32_t id, const String8& path, const InputDeviceIdentifier& identifier);
        ~Device();

        void close();
    };

    status_t openDeviceLocked(const char *devicePath);
    status_t closeDeviceByPathLocked(const char *devicePath);

    void closeDeviceLocked(Device* device);
    void closeAllDevicesLocked();

    status_t scanDirLocked(const char *dirname);
    void scanDevicesLocked();
    status_t readNotifyLocked();

    Device* getDeviceLocked(int32_t deviceId) const;
    Device* getDeviceByPathLocked(const char* devicePath) const;

    bool hasKeycodeLocked(Device* device, int keycode) const;

    void loadConfigurationLocked(Device* device);
    status_t loadVirtualKeyMapLocked(Device* device);
    status_t loadKeyMapLocked(Device* device);

    bool isExternalDeviceLocked(Device* device);

    
    mutable Mutex mLock;

    
    
    int32_t mBuiltInKeyboardId;

    int32_t mNextDeviceId;

    KeyedVector<int32_t, Device*> mDevices;

    Device *mOpeningDevices;
    Device *mClosingDevices;

    bool mNeedToSendFinishedDeviceScan;
    bool mNeedToReopenDevices;
    bool mNeedToScanDevices;
    Vector<String8> mExcludedDevices;

    int mEpollFd;
    int mINotifyFd;
    int mWakeReadPipeFd;
    int mWakeWritePipeFd;

    
    static const uint32_t EPOLL_ID_INOTIFY = 0x80000001;
    static const uint32_t EPOLL_ID_WAKE = 0x80000002;

    
    static const int EPOLL_SIZE_HINT = 8;

    
    static const int EPOLL_MAX_EVENTS = 16;

    
    struct epoll_event mPendingEventItems[EPOLL_MAX_EVENTS];
    size_t mPendingEventCount;
    size_t mPendingEventIndex;
    bool mPendingINotify;

    
    int32_t mNumCpus;
};

}; 

#endif

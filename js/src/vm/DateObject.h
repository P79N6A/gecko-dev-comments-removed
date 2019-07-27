





#ifndef vm_DateObject_h_
#define vm_DateObject_h_

#include "jsobj.h"

#include "js/Value.h"

namespace js {

class DateTimeInfo;

class DateObject : public JSObject
{
    static const uint32_t UTC_TIME_SLOT = 0;
    static const uint32_t TZA_SLOT = 1;

    




    static const uint32_t COMPONENTS_START_SLOT = 2;

    static const uint32_t LOCAL_TIME_SLOT    = COMPONENTS_START_SLOT + 0;
    static const uint32_t LOCAL_YEAR_SLOT    = COMPONENTS_START_SLOT + 1;
    static const uint32_t LOCAL_MONTH_SLOT   = COMPONENTS_START_SLOT + 2;
    static const uint32_t LOCAL_DATE_SLOT    = COMPONENTS_START_SLOT + 3;
    static const uint32_t LOCAL_DAY_SLOT     = COMPONENTS_START_SLOT + 4;
    static const uint32_t LOCAL_HOURS_SLOT   = COMPONENTS_START_SLOT + 5;
    static const uint32_t LOCAL_MINUTES_SLOT = COMPONENTS_START_SLOT + 6;
    static const uint32_t LOCAL_SECONDS_SLOT = COMPONENTS_START_SLOT + 7;

    static const uint32_t RESERVED_SLOTS = LOCAL_SECONDS_SLOT + 1;

  public:
    static const Class class_;

    inline const js::Value &UTCTime() const {
        return getFixedSlot(UTC_TIME_SLOT);
    }

    
    void setUTCTime(double t);
    void setUTCTime(double t, MutableHandleValue vp);

    inline double cachedLocalTime(DateTimeInfo *dtInfo);

    
    
    
    void fillLocalTimeSlots(DateTimeInfo *dtInfo);

    static MOZ_ALWAYS_INLINE bool getTime_impl(JSContext *cx, CallArgs args);
    static MOZ_ALWAYS_INLINE bool getYear_impl(JSContext *cx, CallArgs args);
    static MOZ_ALWAYS_INLINE bool getFullYear_impl(JSContext *cx, CallArgs args);
    static MOZ_ALWAYS_INLINE bool getUTCFullYear_impl(JSContext *cx, CallArgs args);
    static MOZ_ALWAYS_INLINE bool getMonth_impl(JSContext *cx, CallArgs args);
    static MOZ_ALWAYS_INLINE bool getUTCMonth_impl(JSContext *cx, CallArgs args);
    static MOZ_ALWAYS_INLINE bool getDate_impl(JSContext *cx, CallArgs args);
    static MOZ_ALWAYS_INLINE bool getUTCDate_impl(JSContext *cx, CallArgs args);
    static MOZ_ALWAYS_INLINE bool getDay_impl(JSContext *cx, CallArgs args);
    static MOZ_ALWAYS_INLINE bool getUTCDay_impl(JSContext *cx, CallArgs args);
    static MOZ_ALWAYS_INLINE bool getHours_impl(JSContext *cx, CallArgs args);
    static MOZ_ALWAYS_INLINE bool getUTCHours_impl(JSContext *cx, CallArgs args);
    static MOZ_ALWAYS_INLINE bool getMinutes_impl(JSContext *cx, CallArgs args);
    static MOZ_ALWAYS_INLINE bool getUTCMinutes_impl(JSContext *cx, CallArgs args);
    static MOZ_ALWAYS_INLINE bool getUTCSeconds_impl(JSContext *cx, CallArgs args);
    static MOZ_ALWAYS_INLINE bool getUTCMilliseconds_impl(JSContext *cx, CallArgs args);
    static MOZ_ALWAYS_INLINE bool getTimezoneOffset_impl(JSContext *cx, CallArgs args);
};

} 

#endif 

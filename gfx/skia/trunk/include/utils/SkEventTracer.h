






#ifndef SkEventTracer_DEFINED
#define SkEventTracer_DEFINED









#include "SkTypes.h"



#define TRACE_DISABLED_BY_DEFAULT(name) "disabled-by-default-" name

class SK_API SkEventTracer {
public:

    typedef uint64_t Handle;

    static SkEventTracer* GetInstance();

    static void SetInstance(SkEventTracer* tracer) {
        SkDELETE(SkEventTracer::gInstance);
        SkEventTracer::gInstance = tracer;
    }

    virtual ~SkEventTracer() { }

    
    
    
    
    enum CategoryGroupEnabledFlags {
        
        kEnabledForRecording_CategoryGroupEnabledFlags = 1 << 0,
        
        kEnabledForMonitoring_CategoryGroupEnabledFlags = 1 << 1,
        
        kEnabledForEventCallback_CategoryGroupEnabledFlags = 1 << 2,
    };

    virtual const uint8_t* getCategoryGroupEnabled(const char* name) = 0;
    virtual const char* getCategoryGroupName(
      const uint8_t* categoryEnabledFlag) = 0;

    virtual SkEventTracer::Handle
        addTraceEvent(char phase,
                      const uint8_t* categoryEnabledFlag,
                      const char* name,
                      uint64_t id,
                      int32_t numArgs,
                      const char** argNames,
                      const uint8_t* argTypes,
                      const uint64_t* argValues,
                      uint8_t flags) = 0;

    virtual void
        updateTraceEventDuration(const uint8_t* categoryEnabledFlag,
                                 const char* name,
                                 SkEventTracer::Handle handle) = 0;
private:
    static SkEventTracer *gInstance;
};

#endif 

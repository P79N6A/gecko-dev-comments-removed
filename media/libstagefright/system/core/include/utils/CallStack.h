















#ifndef ANDROID_CALLSTACK_H
#define ANDROID_CALLSTACK_H

#include <stdint.h>
#include <sys/types.h>

#include <utils/String8.h>
#include <corkscrew/backtrace.h>



namespace android {

class CallStack
{
public:
    enum {
        MAX_DEPTH = 31
    };

    CallStack();
    CallStack(const char* logtag, int32_t ignoreDepth=1,
            int32_t maxDepth=MAX_DEPTH);
    CallStack(const CallStack& rhs);
    ~CallStack();

    CallStack& operator = (const CallStack& rhs);
    
    bool operator == (const CallStack& rhs) const;
    bool operator != (const CallStack& rhs) const;
    bool operator < (const CallStack& rhs) const;
    bool operator >= (const CallStack& rhs) const;
    bool operator > (const CallStack& rhs) const;
    bool operator <= (const CallStack& rhs) const;
    
    const void* operator [] (int index) const;
    
    void clear();

    void update(int32_t ignoreDepth=1, int32_t maxDepth=MAX_DEPTH);

    
    void dump(const char* logtag, const char* prefix = 0) const;

    
    String8 toString(const char* prefix = 0) const;
    
    size_t size() const { return mCount; }

private:
    size_t mCount;
    backtrace_frame_t mStack[MAX_DEPTH];
};

}; 




#endif

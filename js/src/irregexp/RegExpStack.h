





























#ifndef V8_REGEXP_STACK_H_
#define V8_REGEXP_STACK_H_

#include "jspubtd.h"
#include "js/Utility.h"

namespace js {
namespace irregexp {

class RegExpStack;







class RegExpStackScope
{
  public:
    

    
    explicit RegExpStackScope(JSRuntime *rt);

    
    ~RegExpStackScope();

  private:
    RegExpStack* regexp_stack;
};

class RegExpStack
{
  public:
    
    
    
    static const int kStackLimitSlack = 32;

    RegExpStack();
    ~RegExpStack();
    bool init();

    
    void reset();

    
    bool grow();

    
    const void *addressOfBase() { return &base_; }
    const void *addressOfLimit() { return &limit_; }

    void *base() { return base_; }
    void *limit() { return limit_; }

  private:
    
    static const uintptr_t kMemoryTop = static_cast<uintptr_t>(-1);

    
    static const size_t kMinimumStackSize = 1 * 1024;

    
    static const size_t kMaximumStackSize = 64 * 1024 * 1024;

    
    void *base_;

    
    size_t size;

    
    
    
    
    
    void *limit_;

    void updateLimit() {
        JS_ASSERT(size >= kStackLimitSlack * sizeof(void *));
        limit_ = static_cast<uint8_t *>(base()) + size - (kStackLimitSlack * sizeof(void *));
    }
};

int
GrowBacktrackStack(JSRuntime *rt);

}}  

#endif  

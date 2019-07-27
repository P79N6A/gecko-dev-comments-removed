





#ifndef jit_Label_h
#define jit_Label_h

#include "jit/Ion.h"

namespace js {
namespace jit {

struct LabelBase
{
  protected:
    
    
    int32_t offset_ : 31;
    bool bound_   : 1;

    
    void operator =(const LabelBase &label);
  public:
    static const int32_t INVALID_OFFSET = -1;

    LabelBase() : offset_(INVALID_OFFSET), bound_(false)
    { }

    
    
    bool bound() const {
        return bound_;
    }
    int32_t offset() const {
        JS_ASSERT(bound() || used());
        return offset_;
    }
    
    bool used() const {
        return !bound() && offset_ > INVALID_OFFSET;
    }
    
    void bind(int32_t offset) {
        JS_ASSERT(!bound());
        offset_ = offset;
        bound_ = true;
        JS_ASSERT(offset_ == offset);
    }
    
    void reset() {
        offset_ = INVALID_OFFSET;
        bound_ = false;
    }
    
    
    int32_t use(int32_t offset) {
        JS_ASSERT(!bound());

        int32_t old = offset_;
        offset_ = offset;
        JS_ASSERT(offset_ == offset);

        return old;
    }
};









class Label : public LabelBase
{
  public:
    ~Label()
    {
#ifdef DEBUG
        
        if (OOM_counter > OOM_maxAllocations)
            return;
        if (IonContext *context = MaybeGetIonContext()) {
            if (context->runtime->hadOutOfMemory())
                return;
        }

        MOZ_ASSERT(!used());
#endif
    }
};




class NonAssertingLabel : public Label
{
  public:
    ~NonAssertingLabel()
    {
#ifdef DEBUG
        if (used())
            bind(0);
#endif
    }
};

} } 

#endif 

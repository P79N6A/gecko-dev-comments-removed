






#ifndef SkDiscardableMemory_DEFINED
#define SkDiscardableMemory_DEFINED

#include "SkRefCnt.h"
#include "SkTypes.h"





class SK_API SkDiscardableMemory {
public:
    



    static SkDiscardableMemory* Create(size_t bytes);

    



    class Factory : public SkRefCnt {
    public:
        virtual SkDiscardableMemory* create(size_t bytes) = 0;
    private:
        typedef SkRefCnt INHERITED;
    };

    

    virtual ~SkDiscardableMemory() {}

    








    virtual bool lock() = 0;

    



    virtual void* data() = 0;

    



    virtual void unlock() = 0;
};

#endif

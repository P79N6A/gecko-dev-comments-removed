








































#ifndef jsion_coderef_h__
#define jsion_coderef_h__

namespace JSC {
    class ExecutablePool;
}

namespace js {
namespace ion {

class IonCode
{
    uint8 *code_;
    size_t size_;
    JSC::ExecutablePool *pool_;

  public:
    IonCode()
      : code_(NULL),
        pool_(NULL)
    { }
    IonCode(uint8 *code, size_t size, JSC::ExecutablePool *pool)
      : code_(code),
        size_(size),
        pool_(pool)
    { }

    uint8 *code() const {
        return code_;
    }
    size_t size() const {
        return size_;
    }
    void release();
};


struct IonScript
{
    IonCode method;

    static void Destroy(JSContext *cx, JSScript *script);
};

}
}

#endif 








#ifndef vm_PIC_h
#define vm_PIC_h

#include "jsapi.h"
#include "jscntxt.h"
#include "jsfriendapi.h"
#include "jsobj.h"

#include "gc/Barrier.h"
#include "gc/Heap.h"
#include "gc/Marking.h"

#include "js/Value.h"
#include "vm/GlobalObject.h"

namespace js {

class Shape;

template <typename Category> class PICChain;




template <typename Category>
class PICStub
{
  friend class PICChain<Category>;
  private:
    typedef typename Category::Stub CatStub;
    typedef typename Category::Chain CatChain;

  protected:
    CatStub *next_;

    PICStub() : next_(nullptr) {}
    explicit PICStub(const CatStub *next) : next_(next) {
        JS_ASSERT(next_);
    }
    explicit PICStub(const CatStub &other) : next_(other.next_) {}

  public:
    CatStub *next() const {
        return next_;
    }

  protected:
    void append(CatStub *stub) {
        JS_ASSERT(!next_);
        JS_ASSERT(!stub->next_);
        next_ = stub;
    }
};




template <typename Category>
class PICChain
{
  private:
    typedef typename Category::Stub CatStub;
    typedef typename Category::Chain CatChain;

  protected:
    CatStub *stubs_;

    PICChain() : stubs_(nullptr) {}
    
    PICChain(const PICChain<Category> &other) MOZ_DELETE;

  public:
    CatStub *stubs() const {
        return stubs_;
    }

    void addStub(CatStub *stub) {
        JS_ASSERT(stub);
        JS_ASSERT(!stub->next());
        if (!stubs_) {
            stubs_ = stub;
            return;
        }

        CatStub *cur = stubs_;
        while (cur->next())
            cur = cur->next();
        cur->append(stub);
    }

    unsigned numStubs() const {
        unsigned count = 0;
        for (CatStub *stub = stubs_; stub; stub = stub->next())
            count++;
        return count;
    }

    void removeStub(CatStub *stub, CatStub *previous) {
        if (previous) {
            JS_ASSERT(previous->next() == stub);
            previous->next_ = stub->next();
        } else {
            JS_ASSERT(stub == stubs_);
            stubs_ = stub->next();
        }
        js_delete(stub);
    }
};




struct ForOfPIC
{
    
    class Stub;
    class Chain;

    ForOfPIC() MOZ_DELETE;
    ForOfPIC(const ForOfPIC &other) MOZ_DELETE;

    typedef PICStub<ForOfPIC> BaseStub;
    typedef PICChain<ForOfPIC> BaseChain;

    



    class Stub : public BaseStub
    {
      private:
        
        Shape *shape_;

      public:
        explicit Stub(Shape *shape)
          : BaseStub(),
            shape_(shape)
        {
            JS_ASSERT(shape_);
        }

        Shape *shape() {
            return shape_;
        }
    };

    






















    class Chain : public BaseChain
    {
      private:
        
        HeapPtrObject arrayProto_;
        HeapPtrObject arrayIteratorProto_;

        
        
        HeapPtrShape arrayProtoShape_;
        uint32_t arrayProtoIteratorSlot_;
        HeapValue canonicalIteratorFunc_;

        
        
        HeapPtrShape arrayIteratorProtoShape_;
        uint32_t arrayIteratorProtoNextSlot_;
        HeapValue canonicalNextFunc_;

        
        bool initialized_;

        
        
        bool disabled_;

        static const unsigned MAX_STUBS = 10;

      public:
        Chain()
          : BaseChain(),
            arrayProto_(nullptr),
            arrayIteratorProto_(nullptr),
            arrayProtoShape_(nullptr),
            arrayProtoIteratorSlot_(-1),
            canonicalIteratorFunc_(UndefinedValue()),
            arrayIteratorProtoShape_(nullptr),
            arrayIteratorProtoNextSlot_(-1),
            initialized_(false),
            disabled_(false)
        {}

        
        bool initialize(JSContext *cx);

        
        Stub *isArrayOptimized(ArrayObject *obj);

        
        bool tryOptimizeArray(JSContext *cx, HandleObject array, bool *optimized);

        
        
        bool isArrayStateStillSane();

        
        inline bool isArrayNextStillSane() {
            return (arrayIteratorProto_->lastProperty() == arrayIteratorProtoShape_) &&
                (arrayIteratorProto_->getSlot(arrayIteratorProtoNextSlot_) == canonicalNextFunc_);
        }

        void mark(JSTracer *trc);
        void sweep(FreeOp *fop);

      private:
        
        Stub *getMatchingStub(JSObject *obj);

        
        bool isOptimizableArray(JSObject *obj);

        
        void reset(JSContext *cx);

        
        void eraseChain();
    };

    
    static const Class jsclass;

    static JSObject *createForOfPICObject(JSContext *cx, Handle<GlobalObject *> global);

    static inline Chain *fromJSObject(JSObject *obj) {
        JS_ASSERT(js::GetObjectClass(obj) == &ForOfPIC::jsclass);
        return (ForOfPIC::Chain *) obj->getPrivate();
    }
    static inline Chain *getOrCreate(JSContext *cx) {
        JSObject *obj = cx->global()->getForOfPICObject();
        if (obj)
            return fromJSObject(obj);
        return create(cx);
    }
    static Chain *create(JSContext *cx);
};


} 

#endif 

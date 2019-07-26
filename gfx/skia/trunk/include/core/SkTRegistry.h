








#ifndef SkTRegistry_DEFINED
#define SkTRegistry_DEFINED

#include "SkTypes.h"





template <typename T> class SkTRegistry : SkNoncopyable {
public:
    typedef T Factory;

    explicit SkTRegistry(T fact) : fFact(fact) {
#ifdef SK_BUILD_FOR_ANDROID
        
        {
            SkTRegistry* reg = gHead;
            while (reg) {
                if (reg == this) {
                    return;
                }
                reg = reg->fChain;
            }
        }
#endif
        fChain = gHead;
        gHead  = this;
    }

    static const SkTRegistry* Head() { return gHead; }

    const SkTRegistry* next() const { return fChain; }
    const Factory& factory() const { return fFact; }

private:
    Factory      fFact;
    SkTRegistry* fChain;

    static SkTRegistry* gHead;
};


template <typename T> SkTRegistry<T>* SkTRegistry<T>::gHead;

#endif

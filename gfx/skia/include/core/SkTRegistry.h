








#ifndef SkTRegistry_DEFINED
#define SkTRegistry_DEFINED

#include "SkTypes.h"





template <typename T, typename P> class SkTRegistry : SkNoncopyable {
public:
    typedef T (*Factory)(P);

    SkTRegistry(Factory fact) {
#ifdef ANDROID
        
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
        fFact = fact;
        fChain = gHead;
        gHead = this;
    }

    static const SkTRegistry* Head() { return gHead; }

    const SkTRegistry* next() const { return fChain; }
    Factory factory() const { return fFact; }

private:
    Factory      fFact;
    SkTRegistry* fChain;

    static SkTRegistry* gHead;
};


template <typename T, typename P> SkTRegistry<T, P>* SkTRegistry<T, P>::gHead;

#endif

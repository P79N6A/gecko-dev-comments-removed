















#ifndef A_ATOMIZER_H_

#define A_ATOMIZER_H_

#include <stdint.h>

#include <media/stagefright/foundation/ABase.h>
#include <media/stagefright/foundation/AString.h>
#include <utils/List.h>
#include <utils/Vector.h>
#include <utils/threads.h>

namespace stagefright {

struct AAtomizer {
    static const char *Atomize(const char *name);

private:
    static AAtomizer gAtomizer;

    Mutex mLock;
    Vector<List<AString> > mAtoms;

    AAtomizer();

    const char *atomize(const char *name);

    static uint32_t Hash(const char *s);

    DISALLOW_EVIL_CONSTRUCTORS(AAtomizer);
};

}  

#endif  

















#include <sys/types.h>

#include "AAtomizer.h"

namespace stagefright {


AAtomizer AAtomizer::gAtomizer;


const char *AAtomizer::Atomize(const char *name) {
    return gAtomizer.atomize(name);
}

AAtomizer::AAtomizer() {
    for (size_t i = 0; i < 128; ++i) {
        mAtoms.push(List<AString>());
    }
}

const char *AAtomizer::atomize(const char *name) {
    Mutex::Autolock autoLock(mLock);

    const size_t n = mAtoms.size();
    size_t index = AAtomizer::Hash(name) % n;
    List<AString> &entry = mAtoms.editItemAt(index);
    List<AString>::iterator it = entry.begin();
    while (it != entry.end()) {
        if ((*it) == name) {
            return (*it).c_str();
        }
        ++it;
    }

    entry.push_back(AString(name));

    return (*--entry.end()).c_str();
}


uint32_t AAtomizer::Hash(const char *s) {
    uint32_t sum = 0;
    while (*s != '\0') {
        sum = (sum * 31) + *s;
        ++s;
    }

    return sum;
}

}  









#ifndef MOZPOISONWRITEBASE_H
#define MOZPOISONWRITEBASE_H

#include <stdio.h>
#include <vector>
#include "nspr.h"
#include "mozilla/NullPtr.h"
#include "mozilla/Util.h"
#include "mozilla/Scoped.h"

namespace mozilla {


struct DebugFDAutoLockTraits {
  typedef PRLock *type;
  const static type empty() {
    return nullptr;
  }
  const static void release(type aL) {
    PR_Unlock(aL);
  }
};

class DebugFDAutoLock : public Scoped<DebugFDAutoLockTraits> {
public:
  static PRLock *getDebugFDsLock() {
    
    
    static PRLock *Lock = PR_NewLock();
    return Lock;
  }

  DebugFDAutoLock() : Scoped<DebugFDAutoLockTraits>(getDebugFDsLock()) {
    PR_Lock(get());
  }
};

bool PoisonWriteEnabled();
void PoisonWriteBase();
bool ValidWriteAssert(bool ok);
void BaseCleanup();



std::vector<int>& getDebugFDs();

}

#endif









#ifndef MOZPOISONWRITEBASE_H
#define MOZPOISONWRITEBASE_H

#include <stdio.h>
#include <vector>
#include "nspr.h"
#include "mozilla/NullPtr.h"
#include "mozilla/Util.h"
#include "mozilla/Scoped.h"

namespace mozilla {
bool PoisonWriteEnabled();
bool ValidWriteAssert(bool ok);
bool IsDebugFD(int fd);
}

#endif

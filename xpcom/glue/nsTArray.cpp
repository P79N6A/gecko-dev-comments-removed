





#include <string.h>
#include "nsTArray.h"
#include "nsXPCOM.h"
#include "nsDebug.h"
#include "mozilla/CheckedInt.h"

nsTArrayHeader nsTArrayHeader::sEmptyHdr = { 0, 0, 0 };

bool
IsTwiceTheRequiredBytesRepresentableAsUint32(size_t capacity, size_t elemSize) {
  using mozilla::CheckedUint32;
  return ((CheckedUint32(capacity) * elemSize) * 2).isValid();
}

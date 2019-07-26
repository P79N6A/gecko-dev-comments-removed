











#include "hash.h"




U_CAPI void U_EXPORT2
uhash_deleteHashtable(void *obj) {
    U_NAMESPACE_USE
    delete (Hashtable*) obj;
}








































#include "nscore.h"
#include NEW_H

#include "nsHashSets.h"

DHASH_SET(nsStringHashSet, PLDHashStringEntry, nsAString&)
DHASH_SET(nsCStringHashSet,PLDHashCStringEntry,nsACString&)
DHASH_SET(nsInt32HashSet,  PLDHashInt32Entry,  PRInt32)
DHASH_SET(nsVoidHashSet,   PLDHashVoidEntry,   void*)

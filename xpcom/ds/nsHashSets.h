




































#ifndef __nsHashSets_h__
#define __nsHashSets_h__

#include "nsDoubleHashtable.h"




































#define DECL_DHASH_SET(CLASSNAME,ENTRY_CLASS,KEY_TYPE)                        \
DECL_DHASH_WRAPPER(CLASSNAME##Super,ENTRY_CLASS,KEY_TYPE)                     \
class DHASH_EXPORT CLASSNAME : public CLASSNAME##Super {                      \
public:                                                                       \
  CLASSNAME() : CLASSNAME##Super() { }                                        \
  ~CLASSNAME() { }                                                            \
  nsresult Put(const KEY_TYPE aKey) {                                         \
    return AddEntry(aKey) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;                   \
  }                                                                           \
  PRBool Contains(const KEY_TYPE aKey) {                                      \
    return GetEntry(aKey) ? PR_TRUE : PR_FALSE;                               \
  }                                                                           \
};

#define DHASH_SET(CLASSNAME,ENTRY_CLASS,KEY_TYPE)                              \
DHASH_WRAPPER(CLASSNAME##Super,ENTRY_CLASS,KEY_TYPE)

#undef DHASH_EXPORT
#define DHASH_EXPORT NS_COM

DECL_DHASH_SET(nsStringHashSet, PLDHashStringEntry, nsAString&)
DECL_DHASH_SET(nsCStringHashSet,PLDHashCStringEntry,nsACString&)
DECL_DHASH_SET(nsInt32HashSet,  PLDHashInt32Entry,  PRInt32)
DECL_DHASH_SET(nsVoidHashSet,   PLDHashVoidEntry,   void*)

#undef DHASH_EXPORT
#define DHASH_EXPORT


#endif





#ifndef BASET_H
#define BASET_H

#ifdef DEBUG
static const char BASET_CVS_ID[] = "@(#) $RCSfile: baset.h,v $ $Revision: 1.9 $ $Date: 2012/04/25 14:49:26 $";
#endif 








#ifndef NSSBASET_H
#include "nssbaset.h"
#endif 

#include "plhash.h"

PR_BEGIN_EXTERN_C







struct nssArenaMarkStr;
typedef struct nssArenaMarkStr nssArenaMark;

#ifdef DEBUG











#define ARENA_THREADMARK


















#ifdef ARENA_THREADMARK
#define ARENA_DESTRUCTOR_LIST
#endif 

#endif 

typedef struct nssListStr nssList;
typedef struct nssListIteratorStr nssListIterator;
typedef PRBool (* nssListCompareFunc)(void *a, void *b);
typedef PRIntn (* nssListSortFunc)(void *a, void *b);
typedef void (* nssListElementDestructorFunc)(void *el);

typedef struct nssHashStr nssHash;
typedef void (PR_CALLBACK *nssHashIterator)(const void *key, 
                                            void *value, 
                                            void *arg);











#ifdef DEBUG
struct nssPointerTrackerStr {
  PRCallOnceType once;
  PZLock *lock;
  PLHashTable *table;
};
typedef struct nssPointerTrackerStr nssPointerTracker;
#endif 











enum nssStringTypeEnum {
  nssStringType_DirectoryString,
  nssStringType_TeletexString, 
  nssStringType_PrintableString,
  nssStringType_UniversalString,
  nssStringType_BMPString,
  nssStringType_UTF8String,
  nssStringType_PHGString,
  nssStringType_GeneralString,

  nssStringType_Unknown = -1
};
typedef enum nssStringTypeEnum nssStringType;

PR_END_EXTERN_C

#endif 

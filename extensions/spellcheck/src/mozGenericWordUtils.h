




































#ifndef mozGenericWordUtils_h__
#define mozGenericWordUtils_h__

#include "nsCOMPtr.h"
#include "mozISpellI18NUtil.h"
#include "nsCycleCollectionParticipant.h"

class mozGenericWordUtils : public mozISpellI18NUtil
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_MOZISPELLI18NUTIL
  NS_DECL_CYCLE_COLLECTION_CLASS(mozGenericWordUtils)

  mozGenericWordUtils();
  virtual ~mozGenericWordUtils();
};

#endif

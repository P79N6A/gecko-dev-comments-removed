




#ifndef mozGenericWordUtils_h__
#define mozGenericWordUtils_h__

#include "nsCOMPtr.h"
#include "mozISpellI18NUtil.h"
#include "nsCycleCollectionParticipant.h"

class mozGenericWordUtils : public mozISpellI18NUtil
{
protected:
  virtual ~mozGenericWordUtils();
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISPELLI18NUTIL

  mozGenericWordUtils();
};

#endif

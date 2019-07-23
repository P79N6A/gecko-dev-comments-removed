




































#ifndef mozGenericWordUtils_h__
#define mozGenericWordUtils_h__

#include "nsCOMPtr.h"
#include "mozISpellI18NUtil.h"

class mozGenericWordUtils : public mozISpellI18NUtil
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISPELLI18NUTIL

  mozGenericWordUtils();
  virtual ~mozGenericWordUtils();
};

#endif

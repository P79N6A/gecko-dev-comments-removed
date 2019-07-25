








































#ifndef NSSERIALIZATIONHELPER_H_
#define NSSERIALIZATIONHELPER_H_

#include "nsStringFwd.h"
#include "nsISerializationHelper.h"

class nsISerializable;
class nsISupports;




nsresult NS_SerializeToString(nsISerializable* obj,
                              nsCSubstring& str);




nsresult NS_DeserializeObject(const nsCSubstring& str,
                              nsISupports** obj);

class nsSerializationHelper : public nsISerializationHelper
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSISERIALIZATIONHELPER
};

#endif

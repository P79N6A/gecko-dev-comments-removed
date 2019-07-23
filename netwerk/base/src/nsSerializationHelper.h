








































#ifndef NSSERIALIZATIONHELPER_H_
#define NSSERIALIZATIONHELPER_H_

#include "nsStringFwd.h"

class nsISerializable;
class nsISupports;




nsresult NS_SerializeToString(nsISerializable* obj,
                              nsCSubstring& str);




nsresult NS_DeserializeObject(const nsCSubstring& str,
                              nsISupports** obj);

#endif

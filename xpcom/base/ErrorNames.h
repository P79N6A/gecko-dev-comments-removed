





#ifndef mozilla_ErrorNames_h
#define mozilla_ErrorNames_h

#include "nsError.h"

class nsACString;

namespace mozilla {






void GetErrorName(nsresult rv, nsACString& name);

}

#endif 






#ifndef COMPONENTS_PERFMEASUREMENT_H
#define COMPONENTS_PERFMEASUREMENT_H

#include "nsIXPCScriptable.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace jsperf {

class Module final : public nsIXPCScriptable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCSCRIPTABLE

  Module();

private:
  ~Module();
};

} 
} 

#endif

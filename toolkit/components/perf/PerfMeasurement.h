





































#ifndef COMPONENTS_PERFMEASUREMENT_H
#define COMPONENTS_PERFMEASUREMENT_H

#include "nsIXPCScriptable.h"

namespace mozilla {
namespace jsperf {

class Module : public nsIXPCScriptable
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

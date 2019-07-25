





































#ifndef COMPONENTS_JSDEBUGGER_H
#define COMPONENTS_JSDEBUGGER_H

#include "IJSDebugger.h"

namespace mozilla {
namespace jsdebugger {

class JSDebugger : public IJSDebugger
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IJSDEBUGGER

  JSDebugger();

private:
  ~JSDebugger();
};

}
}

#endif

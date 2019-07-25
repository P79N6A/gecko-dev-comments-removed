




#ifndef COMPONENTS_JSDEBUGGER_H
#define COMPONENTS_JSDEBUGGER_H

#include "IJSDebugger.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace jsdebugger {

class JSDebugger MOZ_FINAL : public IJSDebugger
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

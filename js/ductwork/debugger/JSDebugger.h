




#ifndef JSDebugger_h
#define JSDebugger_h

#include "IJSDebugger.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace jsdebugger {

class JSDebugger final : public IJSDebugger
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

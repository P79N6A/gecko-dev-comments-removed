





































#ifndef COMPONENTS_JSINSPECTOR_H
#define COMPONENTS_JSINSPECTOR_H

#include "nsIJSInspector.h"

namespace mozilla {
namespace jsinspector {

class nsJSInspector : public nsIJSInspector
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIJSINSPECTOR

  nsJSInspector();

private:
  ~nsJSInspector();

  PRUint32 mNestedLoopLevel;
};

}
}

#endif

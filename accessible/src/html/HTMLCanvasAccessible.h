




#include "HyperTextAccessible.h"

#ifndef mozilla_a11y_HTMLCanvasAccessible_h__
#define mozilla_a11y_HTMLCanvasAccessible_h__

namespace mozilla {
namespace a11y {




class HTMLCanvasAccessible : public HyperTextAccessible
{
public:
  HTMLCanvasAccessible(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~HTMLCanvasAccessible() { }

  
  virtual a11y::role NativeRole();
};

} 
} 

#endif

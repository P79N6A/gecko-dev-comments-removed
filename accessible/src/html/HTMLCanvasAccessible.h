




#ifndef mozilla_a11y_HTMLCanvasAccessible_h__
#define mozilla_a11y_HTMLCanvasAccessible_h__

#include "HyperTextAccessibleWrap.h"

namespace mozilla {
namespace a11y {




class HTMLCanvasAccessible : public HyperTextAccessibleWrap
{
public:
  HTMLCanvasAccessible(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~HTMLCanvasAccessible() { }

  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual a11y::role NativeRole();
};

} 
} 

#endif

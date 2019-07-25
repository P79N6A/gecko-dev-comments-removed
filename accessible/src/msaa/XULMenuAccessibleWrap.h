




#ifndef mozilla_a11y_XULMenuAccessibleWrap_h__
#define mozilla_a11y_XULMenuAccessibleWrap_h__

#include "XULMenuAccessible.h"

namespace mozilla {
namespace a11y {

class XULMenuitemAccessibleWrap : public XULMenuitemAccessible
{
public:
  XULMenuitemAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~XULMenuitemAccessibleWrap() {}

  
  virtual mozilla::a11y::ENameValueFlag Name(nsString& aName);
};

} 
} 

#endif

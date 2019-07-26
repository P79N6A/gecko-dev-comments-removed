





#ifndef mozilla_a11y_TextLeafAccessibleWrap_h__
#define mozilla_a11y_TextLeafAccessibleWrap_h__

#include "TextLeafAccessible.h"

namespace mozilla {
namespace a11y {





class TextLeafAccessibleWrap : public TextLeafAccessible
{
public:
  TextLeafAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc) :
    TextLeafAccessible(aContent, aDoc) { }
  virtual ~TextLeafAccessibleWrap() {}

  
  DECL_IUNKNOWN_INHERITED
};

} 
} 

#endif

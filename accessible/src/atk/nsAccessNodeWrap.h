









#ifndef _nsAccessNodeWrap_H_
#define _nsAccessNodeWrap_H_

#include "nsAccessNode.h"

class nsAccessNodeWrap :  public nsAccessNode
{
public: 
  nsAccessNodeWrap(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~nsAccessNodeWrap();

  static void InitAccessibility();
  static void ShutdownAccessibility();
};

#endif

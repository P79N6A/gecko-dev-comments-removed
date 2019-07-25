




#include "HyperTextAccessible.h"

#ifndef _nsHTMLCanvasAccessible_H_
#define _nsHTMLCanvasAccessible_H_




class nsHTMLCanvasAccessible : public HyperTextAccessible
{
public:
  nsHTMLCanvasAccessible(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~nsHTMLCanvasAccessible() { }

  
  virtual mozilla::a11y::role NativeRole();
};

#endif






































#include "nsHyperTextAccessible.h"

#ifndef _nsHTMLCanvasAccessible_H_
#define _nsHTMLCanvasAccessible_H_




class nsHTMLCanvasAccessible : public nsHyperTextAccessible
{
public:
  nsHTMLCanvasAccessible(nsIContent* aContent, nsDocAccessible* aDoc);
  virtual ~nsHTMLCanvasAccessible() { }

  
  virtual mozilla::a11y::role NativeRole();
};

#endif

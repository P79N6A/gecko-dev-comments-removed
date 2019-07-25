




































#include "nsHyperTextAccessible.h"

#ifndef _nsHTMLCanvasAccessible_H_
#define _nsHTMLCanvasAccessible_H_




class nsHTMLCanvasAccessible : public nsHyperTextAccessible
{
public:
  nsHTMLCanvasAccessible(nsIContent* aContent, nsIWeakReference* aShell);
  virtual ~nsHTMLCanvasAccessible() { }

  
  virtual PRUint32 NativeRole();
};

#endif

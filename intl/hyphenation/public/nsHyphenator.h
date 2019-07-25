




































#ifndef nsHyphenator_h__
#define nsHyphenator_h__

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsTArray.h"

class nsIUGenCategory;

class nsHyphenator
{
public:
  nsHyphenator(nsIFile *aFile);

  NS_INLINE_DECL_REFCOUNTING(nsHyphenator)

  PRBool IsValid();

  nsresult Hyphenate(const nsAString& aText, nsTArray<PRPackedBool>& aHyphens);

private:
  ~nsHyphenator();

protected:
  void                      *mDict;
  nsCOMPtr<nsIUGenCategory>  mCategories;
};

#endif 

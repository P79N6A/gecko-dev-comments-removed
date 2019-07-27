




#ifndef nsHyphenator_h__
#define nsHyphenator_h__

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsTArray.h"

class nsIURI;

class nsHyphenator
{
public:
  explicit nsHyphenator(nsIURI *aURI);

  NS_INLINE_DECL_REFCOUNTING(nsHyphenator)

  bool IsValid();

  nsresult Hyphenate(const nsAString& aText, FallibleTArray<bool>& aHyphens);

private:
  ~nsHyphenator();

protected:
  void                      *mDict;
};

#endif 

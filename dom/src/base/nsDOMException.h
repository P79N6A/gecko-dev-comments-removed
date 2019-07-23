






































#include "nsCOMPtr.h"
#include "nsIBaseDOMException.h"
#include "nsIException.h"

class nsBaseDOMException : public nsIException,
                           public nsIBaseDOMException
{
public:
  nsBaseDOMException();
  virtual ~nsBaseDOMException();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIEXCEPTION
  NS_IMETHOD Init(nsresult aNSResult, const char* aName,
                  const char* aMessage,
                  nsIException* aDefaultException);

protected:
  nsresult mResult;
  const char* mName;
  const char* mMessage;
  nsCOMPtr<nsIException> mInner;
};

#define DECL_INTERNAL_DOM_EXCEPTION(domname)                                 \
nsresult                                                                     \
NS_New##domname(nsresult aNSResult, nsIException* aDefaultException,         \
                nsIException** aException);


DECL_INTERNAL_DOM_EXCEPTION(DOMException)
DECL_INTERNAL_DOM_EXCEPTION(RangeException)
#ifdef MOZ_SVG
DECL_INTERNAL_DOM_EXCEPTION(SVGException)
#endif
DECL_INTERNAL_DOM_EXCEPTION(XPathException)







#ifndef mozilla_dom_domerror_h__
#define mozilla_dom_domerror_h__

#include "nsIDOMDOMError.h"

#include "nsCOMPtr.h"
#include "nsStringGlue.h"

namespace mozilla {
namespace dom {

class DOMError : public nsIDOMDOMError
{
  nsString mName;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMDOMERROR

  static already_AddRefed<nsIDOMDOMError>
  CreateForNSResult(nsresult rv);

  static already_AddRefed<nsIDOMDOMError>
  CreateWithName(const nsAString& aName)
  {
    nsCOMPtr<nsIDOMDOMError> error = new DOMError(aName);
    return error.forget();
  }

protected:
  DOMError(const nsAString& aName)
  : mName(aName)
  { }

  virtual ~DOMError()
  { }
};

} 
} 

#endif 

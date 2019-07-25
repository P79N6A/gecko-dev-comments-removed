





































#include "Helpers.h"
#include "mozIStorageError.h"
#include "nsString.h"
#include "nsNavHistory.h"

namespace mozilla {
namespace places {




NS_IMETHODIMP
AsyncStatementCallback::HandleError(mozIStorageError *aError)
{
#ifdef DEBUG
  PRInt32 result;
  nsresult rv = aError->GetResult(&result);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCAutoString message;
  rv = aError->GetMessage(message);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString warnMsg;
  warnMsg.Append("An error occurred while executing an async statement: ");
  warnMsg.Append(result);
  warnMsg.Append(" ");
  warnMsg.Append(message);
  NS_WARNING(warnMsg.get());
#endif

  return NS_OK;
}

#define URI_TO_URLCSTRING(uri, spec) \
  nsCAutoString spec; \
  if (NS_FAILED(aURI->GetSpec(spec))) { \
    return NS_ERROR_UNEXPECTED; \
  }


nsresult 
URIBinder::Bind(mozIStorageStatement* aStatement,
                PRInt32 aIndex,
                nsIURI* aURI)
{
  NS_ASSERTION(aStatement, "Must have non-null statement");
  NS_ASSERTION(aURI, "Must have non-null uri");

  URI_TO_URLCSTRING(aURI, spec);
  return URIBinder::Bind(aStatement, aIndex, spec);
}


nsresult 
URIBinder::Bind(mozIStorageStatement* aStatement,
                PRInt32 index,
                const nsACString& aURLString)
{
  NS_ASSERTION(aStatement, "Must have non-null statement");
  return aStatement->BindUTF8StringByIndex(
    index, StringHead(aURLString, URI_LENGTH_MAX)
  );
}


nsresult 
URIBinder::Bind(mozIStorageStatement* aStatement,
                const nsACString& aName,
                nsIURI* aURI)
{
  NS_ASSERTION(aStatement, "Must have non-null statement");
  NS_ASSERTION(aURI, "Must have non-null uri");

  URI_TO_URLCSTRING(aURI, spec);
  return URIBinder::Bind(aStatement, aName, spec);
}


nsresult 
URIBinder::Bind(mozIStorageStatement* aStatement,
                const nsACString& aName,
                const nsACString& aURLString)
{
  NS_ASSERTION(aStatement, "Must have non-null statement");
  return aStatement->BindUTF8StringByName(
    aName, StringHead(aURLString, URI_LENGTH_MAX)
  );
}


nsresult 
URIBinder::Bind(mozIStorageBindingParams* aParams,
                PRInt32 aIndex,
                nsIURI* aURI)
{
  NS_ASSERTION(aParams, "Must have non-null statement");
  NS_ASSERTION(aURI, "Must have non-null uri");

  URI_TO_URLCSTRING(aURI, spec);
  return URIBinder::Bind(aParams, aIndex, spec);
}


nsresult 
URIBinder::Bind(mozIStorageBindingParams* aParams,
                PRInt32 index,
                const nsACString& aURLString)
{
  NS_ASSERTION(aParams, "Must have non-null statement");
  return aParams->BindUTF8StringByIndex(
    index, StringHead(aURLString, URI_LENGTH_MAX)
  );
}


nsresult 
URIBinder::Bind(mozIStorageBindingParams* aParams,
                const nsACString& aName,
                nsIURI* aURI)
{
  NS_ASSERTION(aParams, "Must have non-null params array");
  NS_ASSERTION(aURI, "Must have non-null uri");

  URI_TO_URLCSTRING(aURI, spec);
  return URIBinder::Bind(aParams, aName, spec);
}


nsresult 
URIBinder::Bind(mozIStorageBindingParams* aParams,
                const nsACString& aName,
                const nsACString& aURLString)
{
  NS_ASSERTION(aParams, "Must have non-null params array");

  nsresult rv = aParams->BindUTF8StringByName(
    aName, StringHead(aURLString, URI_LENGTH_MAX)
  );
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

#undef URI_TO_URLCSTRING


} 
} 

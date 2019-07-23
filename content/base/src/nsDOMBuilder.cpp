









































#include "nsDOMBuilder.h"
#include "nsDOMError.h"
#include "nsContentUtils.h"  
#include "nsIDOMDocument.h"

nsresult
NS_NewDOMBuilder(nsIDOMDOMBuilder** aResult,
                 PRUint16 aMode,
                 const nsAString & aSchemaType,
		 nsIDOMDOMImplementation* aDOMImplementation)
{
  NS_PRECONDITION(aResult, "Null out ptr?  Who do you think you are, flouting XPCOM contract?");
  NS_PRECONDITION(aDOMImplementation, "How are we supposed to create documents without a DOMImplementation?");

  nsDOMBuilder* it = new nsDOMBuilder(aMode, aSchemaType, aDOMImplementation);
  if (!it)
    return NS_ERROR_OUT_OF_MEMORY;

  return CallQueryInterface(it, aResult);
}

nsDOMBuilder::nsDOMBuilder(PRUint16 aMode,
			   const nsAString& aSchemaType,
			   nsIDOMDOMImplementation* aDOMImplementation)
{
  mDOMImplementation = aDOMImplementation;
}

nsDOMBuilder::~nsDOMBuilder()
{
}

NS_IMPL_ADDREF(nsDOMBuilder)
NS_IMPL_RELEASE(nsDOMBuilder)

NS_INTERFACE_MAP_BEGIN(nsDOMBuilder)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDOMBuilder)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMDOMBuilder)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(DOMBuilder)
NS_INTERFACE_MAP_END


NS_IMETHODIMP
nsDOMBuilder::GetEntityResolver(nsIDOMDOMEntityResolver** aEntityResolver)
{
  *aEntityResolver = mEntityResolver;
  NS_IF_ADDREF(*aEntityResolver);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMBuilder::SetEntityResolver(nsIDOMDOMEntityResolver* aEntityResolver)
{
  mEntityResolver = aEntityResolver;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMBuilder::GetErrorHandler(nsIDOMDOMErrorHandler** aErrorHandler)
{
  *aErrorHandler = mErrorHandler;
  NS_IF_ADDREF(*aErrorHandler);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMBuilder::SetErrorHandler(nsIDOMDOMErrorHandler* aErrorHandler)
{
  mErrorHandler = aErrorHandler;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMBuilder::GetFilter(nsIDOMDOMBuilderFilter** aFilter)
{
  *aFilter = mFilter;
  NS_IF_ADDREF(*aFilter);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMBuilder::SetFilter(nsIDOMDOMBuilderFilter* aFilter)
{
  mFilter = aFilter;
  return NS_OK;
}


NS_IMETHODIMP
nsDOMBuilder::SetFeature(const nsAString& aName, PRBool aState)
{
  
  return NS_ERROR_DOM_NOT_FOUND_ERR;
}

NS_IMETHODIMP
nsDOMBuilder::CanSetFeature(const nsAString& aName, PRBool aState,
			    PRBool* aCanSet)
{
  
  *aCanSet = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMBuilder::GetFeature(const nsAString& aName, PRBool* aIsOn)
{
  
  return NS_ERROR_DOM_NOT_FOUND_ERR;
}
    
NS_IMETHODIMP
nsDOMBuilder::ParseURI(const nsAString& aURI, nsIDOMDocument** aDocument)
{
  *aDocument = nsnull;

  nsCOMPtr<nsIDOMDocument> domDoc;

  NS_NAMED_LITERAL_STRING(emptyStr, "");
  mDOMImplementation->CreateDocument(emptyStr,
				     emptyStr,
				     nsnull,
				     getter_AddRefs(domDoc));

  if (!domDoc)
    return NS_ERROR_FAILURE;

  

  
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDOMBuilder::Parse(nsIDOMDOMInputSource* aInputSource,
		    nsIDOMDocument** aDocument)
{
  *aDocument = nsnull;
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDOMBuilder::ParseWithContext(nsIDOMDOMInputSource* aInputSource,
			       nsIDOMNode* aContextNode,
			       PRUint16 aAction)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

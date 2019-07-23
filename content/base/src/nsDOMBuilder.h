









































#ifndef nsDOMBuilder_h__
#define nsDOMBuilder_h__

#include "nsIDOMDOMBuilder.h"
#include "nsIDOMDOMEntityResolver.h"
#include "nsIDOMDOMErrorHandler.h"
#include "nsIDOMDOMBuilderFilter.h"
#include "nsIDOMDOMImplementation.h"
#include "nsCOMPtr.h"

class nsDOMBuilder : public nsIDOMDOMBuilder
{
public:
  nsDOMBuilder(PRUint16 aMode, const nsAString& aSchemaType,
               nsIDOMDOMImplementation* aDOMImplementation);
  virtual ~nsDOMBuilder();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMDOMBUILDER

private:
  nsCOMPtr<nsIDOMDOMEntityResolver> mEntityResolver;
  nsCOMPtr<nsIDOMDOMErrorHandler> mErrorHandler;
  nsCOMPtr<nsIDOMDOMBuilderFilter> mFilter;
  
  nsCOMPtr<nsIDOMDOMImplementation> mDOMImplementation;
};

nsresult
NS_NewDOMBuilder(nsIDOMDOMBuilder** aResult,
                 PRUint16 aMode,
                 const nsAString & aSchemaType,
                 nsIDOMDOMImplementation* aDOMImplementation);

#endif 

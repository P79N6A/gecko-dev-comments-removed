




































#include "nsIScriptLoaderObserver.h"
#include "nsIScriptElement.h"
#include "nsStubMutationObserver.h"
#include "prtypes.h"







class nsScriptElement : public nsIScriptElement,
                        public nsStubMutationObserver
{
public:
  
  NS_DECL_NSISCRIPTLOADEROBSERVER

  
  NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED
  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED

protected:
  

  


  virtual PRBool HasScriptContent() = 0;

  












  virtual nsresult MaybeProcessScript();
};

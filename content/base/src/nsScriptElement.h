



#ifndef nsScriptElement_h
#define nsScriptElement_h

#include "mozilla/Attributes.h"
#include "nsIScriptLoaderObserver.h"
#include "nsIScriptElement.h"
#include "nsStubMutationObserver.h"







class nsScriptElement : public nsIScriptElement,
                        public nsStubMutationObserver
{
public:
  
  NS_DECL_NSISCRIPTLOADEROBSERVER

  
  NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED
  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED

  nsScriptElement(mozilla::dom::FromParser aFromParser)
    : nsIScriptElement(aFromParser)
  {
  }

  virtual nsresult FireErrorEvent() MOZ_OVERRIDE;

protected:
  

  


  virtual bool HasScriptContent() = 0;

  virtual bool MaybeProcessScript() MOZ_OVERRIDE;
};

#endif 

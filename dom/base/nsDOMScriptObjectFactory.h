



















#include "nsIDOMScriptObjectFactory.h"
#include "nsIObserver.h"
#include "mozilla/Attributes.h"

class nsDOMScriptObjectFactory final : public nsIDOMScriptObjectFactory,
                                       public nsIObserver
{
  ~nsDOMScriptObjectFactory() {}

public:
  nsDOMScriptObjectFactory();

  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIOBSERVER

  
  NS_IMETHOD_(nsISupports *) GetClassInfoInstance(nsDOMClassInfoID aID) override;
  NS_IMETHOD_(nsISupports *) GetExternalClassInfoInstance(const nsAString& aName) override;

  NS_IMETHOD RegisterDOMClassInfo(const char *aName,
                                  nsDOMClassInfoExternalConstructorFnc aConstructorFptr,
                                  const nsIID *aProtoChainInterface,
                                  const nsIID **aInterfaces,
                                  uint32_t aScriptableFlags,
                                  bool aHasClassInterface,
                                  const nsCID *aConstructorCID) override;
};


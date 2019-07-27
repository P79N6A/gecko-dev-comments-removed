





#ifndef SubstitutingProtocolHandler_h___
#define SubstitutingProtocolHandler_h___

#include "nsISubstitutingProtocolHandler.h"

#include "nsInterfaceHashtable.h"
#include "nsIOService.h"
#include "nsStandardURL.h"
#include "mozilla/chrome/RegistryMessageUtils.h"

class nsIIOService;

namespace mozilla {






class SubstitutingProtocolHandler
{
public:
  SubstitutingProtocolHandler(const char* aScheme, uint32_t aFlags);

  NS_INLINE_DECL_REFCOUNTING(SubstitutingProtocolHandler);
  NS_DECL_NON_VIRTUAL_NSIPROTOCOLHANDLER;
  NS_DECL_NON_VIRTUAL_NSISUBSTITUTINGPROTOCOLHANDLER;

  void CollectSubstitutions(InfallibleTArray<SubstitutionMapping>& aResources);

protected:
  virtual ~SubstitutingProtocolHandler() {}

  void SendSubstitution(const nsACString& aRoot, nsIURI* aBaseURI);

  
  
  virtual nsresult GetSubstitutionInternal(const nsACString& aRoot, nsIURI** aResult)
  {
    *aResult = nullptr;
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsIIOService* IOService() { return mIOService; }

private:
  nsCString mScheme;
  uint32_t mFlags;
  nsInterfaceHashtable<nsCStringHashKey,nsIURI> mSubstitutions;
  nsCOMPtr<nsIIOService> mIOService;
};


class SubstitutingURL : public nsStandardURL
{
public:
  SubstitutingURL() : nsStandardURL(true) {}
  virtual nsStandardURL* StartClone();
  virtual nsresult EnsureFile();
  NS_IMETHOD GetClassIDNoAlloc(nsCID *aCID);
};

} 

#endif 

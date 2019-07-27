





#ifndef __nznetworkzonepolicy_h__
#define __nznetworkzonepolicy_h__

#include "nsCOMPtr.h"
#include "mozilla/StaticPtr.h"
#include "nsINetworkZonePolicy.h"
#include "nsIObserver.h"

class nsILoadGroup;

namespace mozilla
{
namespace net
{








class nsNetworkZonePolicy : public nsINetworkZonePolicy
                          , public nsIObserver
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSINETWORKZONEPOLICY
  NS_DECL_NSIOBSERVER

  static already_AddRefed<nsNetworkZonePolicy> GetSingleton();

private:
  nsNetworkZonePolicy();
  virtual ~nsNetworkZonePolicy();

  
  already_AddRefed<nsILoadGroup> GetLoadGroupParent(nsILoadGroup *aLoadGroup);

  
  already_AddRefed<nsILoadGroup> GetOwningLoadGroup(nsILoadGroup *aLoadGroup);

  
  
  already_AddRefed<nsILoadGroup>
    GetParentDocShellsLoadGroup(nsILoadGroup *aLoadGroup);

  
  
  
  
  
  
  bool CheckLoadGroupHierarchy(nsILoadGroup *aLoadGroup);

  
  
  
  bool CheckLoadGroupAncestorHierarchies(nsILoadGroup *aLoadGroup);

  
  static bool sNZPEnabled;

  
  static bool sShutdown;

  
  static StaticRefPtr<nsNetworkZonePolicy> sSingleton;
};

} 
} 

#endif 






#ifndef GMPServiceChild_h_
#define GMPServiceChild_h_

#include "GMPService.h"
#include "base/process.h"
#include "mozilla/ipc/Transport.h"
#include "mozilla/gmp/PGMPServiceChild.h"

namespace mozilla {
namespace gmp {

#define GMP_DEFAULT_ASYNC_SHUTDONW_TIMEOUT 3000

class GMPContentParent;
class GMPServiceChild;
class GetServiceChildCallback;

class GeckoMediaPluginServiceChild : public GeckoMediaPluginService
{
  friend class GMPServiceChild;

public:
  static already_AddRefed<GeckoMediaPluginServiceChild> GetSingleton();

  NS_IMETHOD GetPluginVersionForAPI(const nsACString& aAPI,
                                    nsTArray<nsCString>* aTags,
                                    bool* aHasPlugin,
                                    nsACString& aOutVersion) override;
  NS_IMETHOD GetNodeId(const nsAString& aOrigin,
                       const nsAString& aTopLevelOrigin,
                       bool aInPrivateBrowsingMode,
                       const nsACString& aVersion,
                       UniquePtr<GetNodeIdCallback>&& aCallback) override;

  NS_DECL_NSIOBSERVER

  void SetServiceChild(UniquePtr<GMPServiceChild>&& aServiceChild);

  void RemoveGMPContentParent(GMPContentParent* aGMPContentParent);

protected:
  virtual void InitializePlugins() override
  {
    
  }
  virtual bool GetContentParentFrom(const nsACString& aNodeId,
                                    const nsCString& aAPI,
                                    const nsTArray<nsCString>& aTags,
                                    UniquePtr<GetGMPContentParentCallback>&& aCallback)
    override;

private:
  friend class OpenPGMPServiceChild;

  void GetServiceChild(UniquePtr<GetServiceChildCallback>&& aCallback);

  UniquePtr<GMPServiceChild> mServiceChild;
  nsTArray<UniquePtr<GetServiceChildCallback>> mGetServiceChildCallbacks;
};

class GMPServiceChild : public PGMPServiceChild
{
public:
  explicit GMPServiceChild();
  virtual ~GMPServiceChild();

  virtual PGMPContentParent* AllocPGMPContentParent(Transport* aTransport,
                                                    ProcessId aOtherPid)
    override;

  void GetBridgedGMPContentParent(ProcessId aOtherPid,
                                  GMPContentParent** aGMPContentParent);
  void RemoveGMPContentParent(GMPContentParent* aGMPContentParent);

  void GetAlreadyBridgedTo(nsTArray<ProcessId>& aAlreadyBridgedTo);

  static PGMPServiceChild* Create(Transport* aTransport, ProcessId aOtherPid);

private:
  nsRefPtrHashtable<nsUint64HashKey, GMPContentParent> mContentParents;
};

} 
} 

#endif 

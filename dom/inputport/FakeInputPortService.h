





#ifndef mozilla_dom_FakeInputPortService_h
#define mozilla_dom_FakeInputPortService_h

#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIInputPortService.h"
#include "nsTArray.h"

#define FAKE_INPUTPORT_SERVICE_CONTRACTID \
  "@mozilla.org/inputport/fakeinputportservice;1"
#define FAKE_INPUTPORT_SERVICE_CID \
  { 0xea6b01c5, 0xad04, 0x4f2a, \
    { 0x8a, 0xbe, 0x64, 0xdb, 0xa2, 0x22, 0xe3, 0x3d } }

class nsITimer;
class nsIInputPortData;

namespace mozilla {
namespace dom {

class FakeInputPortService final : public nsIInputPortService
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(FakeInputPortService)
  NS_DECL_NSIINPUTPORTSERVICE

  FakeInputPortService();

private:
  ~FakeInputPortService();

  void Init();

  void Shutdown();

  already_AddRefed<nsIInputPortData> MockInputPort(const nsAString& aId,
                                                   const nsAString& aType,
                                                   bool aIsConnected);

  nsCOMPtr<nsIInputPortListener> mInputPortListener;
  nsCOMPtr<nsITimer> mPortConnectionChangedTimer;
  nsTArray<nsCOMPtr<nsIInputPortData>> mPortDatas;
};

} 
} 

#endif 

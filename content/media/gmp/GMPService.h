




#ifndef GMPService_h_
#define GMPService_h_

#include "mozIGeckoMediaPluginService.h"
#include "nsIObserver.h"
#include "nsTArray.h"
#include "mozilla/Mutex.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsIThread.h"

class nsIFile;
template <class> class already_AddRefed;

namespace mozilla {
namespace gmp {

class GMPParent;

class GeckoMediaPluginService MOZ_FINAL : public mozIGeckoMediaPluginService
                                        , public nsIObserver
{
public:
  static already_AddRefed<GeckoMediaPluginService> GetGeckoMediaPluginService();

  GeckoMediaPluginService();
  void Init();

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_MOZIGECKOMEDIAPLUGINSERVICE
  NS_DECL_NSIOBSERVER

private:
  ~GeckoMediaPluginService();

  GMPParent* SelectPluginFromListForAPI(const nsCString& aAPI, const nsCString& aTag);
  GMPParent* SelectPluginForAPI(const nsCString& aAPI, const nsCString& aTag);
  void UnloadPlugins();

  void RefreshPluginList();
  void ProcessPossiblePlugin(nsIFile* aDir);
  nsresult SearchDirectory(nsIFile* aSearchDir);
  nsresult GetPossiblePlugins(nsTArray<nsCOMPtr<nsIFile>>& aDirs);
  nsresult GetDirectoriesToSearch(nsTArray<nsCOMPtr<nsIFile>>& aDirs);

  nsTArray<nsRefPtr<GMPParent>> mPlugins;
  Mutex mMutex; 
  nsCOMPtr<nsIThread> mGMPThread;
  bool mShuttingDown;
  bool mShuttingDownOnGMPThread;
  nsString mHomePath;
};

} 
} 

#endif 

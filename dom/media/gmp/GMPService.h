




#ifndef GMPService_h_
#define GMPService_h_

#include "nsString.h"
#include "mozIGeckoMediaPluginService.h"
#include "nsIObserver.h"
#include "nsTArray.h"
#include "mozilla/Attributes.h"
#include "mozilla/Monitor.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsIThread.h"

template <class> struct already_AddRefed;

namespace mozilla {

extern PRLogModuleInfo* GetGMPLog();

namespace gmp {

class GMPParent;

#define GMP_DEFAULT_ASYNC_SHUTDONW_TIMEOUT 3000

class GeckoMediaPluginService : public mozIGeckoMediaPluginService
                              , public nsIObserver
{
public:
  static already_AddRefed<GeckoMediaPluginService> GetGeckoMediaPluginService();

  virtual nsresult Init();

  NS_DECL_THREADSAFE_ISUPPORTS

  
  NS_IMETHOD GetThread(nsIThread** aThread) override;
  NS_IMETHOD HasPluginForAPI(const nsACString& aAPI, nsTArray<nsCString>* aTags,
                             bool *aRetVal) override;
  NS_IMETHOD GetGMPVideoDecoder(nsTArray<nsCString>* aTags,
                                const nsACString& aNodeId,
                                GMPVideoHost** aOutVideoHost,
                                GMPVideoDecoderProxy** aGMPVD) override;
  NS_IMETHOD GetGMPVideoEncoder(nsTArray<nsCString>* aTags,
                                const nsACString& aNodeId,
                                GMPVideoHost **aOutVideoHost,
                                GMPVideoEncoderProxy** aGMPVE) override;
  NS_IMETHOD GetGMPAudioDecoder(nsTArray<nsCString>* aTags,
                                const nsACString& aNodeId,
                                GMPAudioDecoderProxy **aGMPAD) override;
  NS_IMETHOD GetGMPDecryptor(nsTArray<nsCString>* aTags,
                             const nsACString& aNodeId,
                             GMPDecryptorProxy** aDecryptor) override;

  int32_t AsyncShutdownTimeoutMs();

  class PluginCrashCallback
  {
  public:
    NS_INLINE_DECL_REFCOUNTING(PluginCrashCallback)

    PluginCrashCallback(const nsACString& aPluginId)
      : mPluginId(aPluginId)
    {
      MOZ_ASSERT(NS_IsMainThread());
    }
    const nsACString& PluginId() const { return mPluginId; }
    virtual void Run(const nsACString& aPluginName, const nsAString& aPluginDumpId) = 0;
    virtual bool IsStillValid() = 0; 
  protected:
    virtual ~PluginCrashCallback()
    {
      MOZ_ASSERT(NS_IsMainThread());
    }
  private:
    const nsCString mPluginId;
  };
  void RemoveObsoletePluginCrashCallbacks(); 
  void AddPluginCrashCallback(nsRefPtr<PluginCrashCallback> aPluginCrashCallback);
  void RemovePluginCrashCallbacks(const nsACString& aPluginId);
  void RunPluginCrashCallbacks(const nsACString& aPluginId,
                               const nsACString& aPluginName,
                               const nsAString& aPluginDumpId);

protected:
  GeckoMediaPluginService();
  virtual ~GeckoMediaPluginService();

  virtual void InitializePlugins() = 0;
  virtual GMPParent* SelectPluginForAPI(const nsACString& aNodeId,
                                const nsCString& aAPI,
                                const nsTArray<nsCString>& aTags) = 0;

  nsresult GMPDispatch(nsIRunnable* event, uint32_t flags = NS_DISPATCH_NORMAL);
  void ShutdownGMPThread();

protected:
  Mutex mMutex; 
                
  nsCOMPtr<nsIThread> mGMPThread;
  bool mGMPThreadShutdown;
  bool mShuttingDownOnGMPThread;

  nsTArray<nsRefPtr<PluginCrashCallback>> mPluginCrashCallbacks;
};

} 
} 

#endif 

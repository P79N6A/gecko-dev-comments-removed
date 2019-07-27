



#ifndef mozilla_nativefilewatcher_h__
#define mozilla_nativefilewatcher_h__

#include "nsINativeFileWatcher.h"

namespace mozilla {

class NativeFileWatcherService final : public nsINativeFileWatcherService
{
public:
  NS_DECL_ISUPPORTS

  NativeFileWatcherService()
  {
  };

  nsresult Init()
  {
    return NS_OK;
  };

  NS_IMETHODIMP AddPath(const nsAString& aPathToWatch,
                        nsINativeFileWatcherCallback* aOnChange,
                        nsINativeFileWatcherErrorCallback* aOnError,
                        nsINativeFileWatcherSuccessCallback* aOnSuccess) override
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  };

  NS_IMETHODIMP RemovePath(const nsAString& aPathToRemove,
                           nsINativeFileWatcherCallback* aOnChange,
                           nsINativeFileWatcherErrorCallback* aOnError,
                           nsINativeFileWatcherSuccessCallback* aOnSuccess) override
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  };

private:
  ~NativeFileWatcherService() { };
  NativeFileWatcherService(const NativeFileWatcherService& other) = delete;
  void operator=(const NativeFileWatcherService& other) = delete;
};

NS_IMPL_ISUPPORTS(NativeFileWatcherService, nsINativeFileWatcherService);

} 

#endif 

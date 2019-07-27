




#ifndef AddonPathService_h
#define AddonPathService_h

#include "amIAddonPathService.h"
#include "nsString.h"
#include "nsTArray.h"

class nsIURI;
class JSAddonId;

namespace mozilla {

JSAddonId*
MapURIToAddonID(nsIURI* aURI);

class AddonPathService final : public amIAddonPathService
{
public:
  AddonPathService();

  static AddonPathService* GetInstance();

  JSAddonId* Find(const nsAString& path);
  static JSAddonId* FindAddonId(const nsAString& path);

  NS_DECL_ISUPPORTS
  NS_DECL_AMIADDONPATHSERVICE

  struct PathEntry
  {
    nsString mPath;
    JSAddonId* mAddonId;

    PathEntry(const nsAString& aPath, JSAddonId* aAddonId)
     : mPath(aPath), mAddonId(aAddonId)
    {}
  };

private:
  virtual ~AddonPathService();

  
  nsTArray<PathEntry> mPaths;

  static AddonPathService* sInstance;
};

} 

#endif

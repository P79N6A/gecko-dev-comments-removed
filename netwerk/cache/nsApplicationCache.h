




class nsApplicationCache : public nsIApplicationCache
                         , public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIAPPLICATIONCACHE

  nsApplicationCache(nsOfflineCacheDevice *device,
                     const nsACString &group,
                     const nsACString &clientID);

  nsApplicationCache();

  void MarkInvalid();

private:
  virtual ~nsApplicationCache();

  nsRefPtr<nsOfflineCacheDevice> mDevice;
  nsCString mGroup;
  nsCString mClientID;
  bool mValid;
};


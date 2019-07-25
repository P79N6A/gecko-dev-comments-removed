





































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

  virtual ~nsApplicationCache();

  void MarkInvalid();

private:
  nsRefPtr<nsOfflineCacheDevice> mDevice;
  nsCString mGroup;
  nsCString mClientID;
  bool mValid;
};


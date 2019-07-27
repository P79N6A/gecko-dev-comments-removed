









#ifndef mozilla_dom_Link_h__
#define mozilla_dom_Link_h__

#include "mozilla/IHistory.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/dom/URLSearchParams.h"
#include "nsIContent.h" 

namespace mozilla {

class EventStates;

namespace dom {

class Element;

#define MOZILLA_DOM_LINK_IMPLEMENTATION_IID               \
{ 0xb25edee6, 0xdd35, 0x4f8b,                             \
  { 0xab, 0x90, 0x66, 0xd0, 0xbd, 0x3c, 0x22, 0xd5 } }

class Link : public URLSearchParamsObserver
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(MOZILLA_DOM_LINK_IMPLEMENTATION_IID)

  


  explicit Link(Element* aElement);
  virtual void SetLinkState(nsLinkState aState);

  




  EventStates LinkState() const;

  


  nsIURI* GetURI() const;
  virtual nsIURI* GetURIExternal() const {
    return GetURI();
  }

  


  void SetProtocol(const nsAString &aProtocol, ErrorResult& aError);
  void SetUsername(const nsAString &aUsername, ErrorResult& aError);
  void SetPassword(const nsAString &aPassword, ErrorResult& aError);
  void SetHost(const nsAString &aHost, ErrorResult& aError);
  void SetHostname(const nsAString &aHostname, ErrorResult& aError);
  void SetPathname(const nsAString &aPathname, ErrorResult& aError);
  void SetSearch(const nsAString &aSearch, ErrorResult& aError);
  void SetSearchParams(mozilla::dom::URLSearchParams& aSearchParams);
  void SetPort(const nsAString &aPort, ErrorResult& aError);
  void SetHash(const nsAString &aHash, ErrorResult& aError);
  void GetOrigin(nsAString &aOrigin, ErrorResult& aError);
  void GetProtocol(nsAString &_protocol, ErrorResult& aError);
  void GetUsername(nsAString &aUsername, ErrorResult& aError);
  void GetPassword(nsAString &aPassword, ErrorResult& aError);
  void GetHost(nsAString &_host, ErrorResult& aError);
  void GetHostname(nsAString &_hostname, ErrorResult& aError);
  void GetPathname(nsAString &_pathname, ErrorResult& aError);
  void GetSearch(nsAString &_search, ErrorResult& aError);
  URLSearchParams* SearchParams();
  void GetPort(nsAString &_port, ErrorResult& aError);
  void GetHash(nsAString &_hash, ErrorResult& aError);

  






  void ResetLinkState(bool aNotify, bool aHasHref);
  
  
  Element* GetElement() const { return mElement; }

  


  virtual void OnDNSPrefetchDeferred() {  }
  
  


  virtual void OnDNSPrefetchRequested() {  }

  





  virtual bool HasDeferredDNSPrefetchRequest() { return true; }

  virtual size_t
    SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

  bool ElementHasHref() const;

  
  void URLSearchParamsUpdated(URLSearchParams* aSearchParams) MOZ_OVERRIDE;

protected:
  virtual ~Link();

  


  bool HasURI() const
  {
    if (HasCachedURI()) {
      return true;
    }

    return !!GetURI();
  }

  nsIURI* GetCachedURI() const { return mCachedURI; }
  bool HasCachedURI() const { return !!mCachedURI; }

  void UpdateURLSearchParams();

  
  void Unlink();
  void Traverse(nsCycleCollectionTraversalCallback &cb);

private:
  



  void UnregisterFromHistory();

  already_AddRefed<nsIURI> GetURIToMutate();
  void SetHrefAttribute(nsIURI *aURI);

  void CreateSearchParamsIfNeeded();

  void SetSearchInternal(const nsAString& aSearch);

  mutable nsCOMPtr<nsIURI> mCachedURI;

  Element * const mElement;

  
  
  nsCOMPtr<IHistory> mHistory;

  uint16_t mLinkState;

  bool mNeedsRegistration;

  bool mRegistered;

protected:
  nsRefPtr<URLSearchParams> mSearchParams;
};

NS_DEFINE_STATIC_IID_ACCESSOR(Link, MOZILLA_DOM_LINK_IMPLEMENTATION_IID)

} 
} 

#endif 

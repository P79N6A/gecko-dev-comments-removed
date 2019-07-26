









#ifndef mozilla_dom_Link_h__
#define mozilla_dom_Link_h__

#include "mozilla/IHistory.h"
#include "nsIContent.h"

namespace mozilla {
namespace dom {

class Element;

#define MOZILLA_DOM_LINK_IMPLEMENTATION_IID \
  { 0x7EA57721, 0xE373, 0x458E, \
    {0x8F, 0x44, 0xF8, 0x96, 0x56, 0xB4, 0x14, 0xF5 } }

class Link : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(MOZILLA_DOM_LINK_IMPLEMENTATION_IID)

  


  Link(Element* aElement);
  nsLinkState GetLinkState() const;
  virtual void SetLinkState(nsLinkState aState);

  




  nsEventStates LinkState() const;

  


  already_AddRefed<nsIURI> GetURI() const;
  virtual already_AddRefed<nsIURI> GetURIExternal() const {
    return GetURI();
  }

  


  void SetProtocol(const nsAString &aProtocol);
  void SetHost(const nsAString &aHost);
  void SetHostname(const nsAString &aHostname);
  void SetPathname(const nsAString &aPathname);
  void SetSearch(const nsAString &aSearch);
  void SetPort(const nsAString &aPort);
  void SetHash(const nsAString &aHash);
  void GetProtocol(nsAString &_protocol);
  void GetHost(nsAString &_host);
  void GetHostname(nsAString &_hostname);
  void GetPathname(nsAString &_pathname);
  void GetSearch(nsAString &_search);
  void GetPort(nsAString &_port);
  void GetHash(nsAString &_hash);

  






  void ResetLinkState(bool aNotify, bool aHasHref);
  
  
  Element* GetElement() const { return mElement; }

  


  virtual void OnDNSPrefetchDeferred() {  }
  
  


  virtual void OnDNSPrefetchRequested() {  }

  





  virtual bool HasDeferredDNSPrefetchRequest() { return true; }

  virtual size_t
    SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

  bool ElementHasHref() const;

protected:
  virtual ~Link();

  


  bool HasURI() const
  {
    if (mCachedURI)
      return true;

    nsCOMPtr<nsIURI> uri(GetURI());
    return !!uri;
  }

  nsIURI* GetCachedURI() const { return mCachedURI; }
  bool HasCachedURI() const { return !!mCachedURI; }

private:
  



  void UnregisterFromHistory();

  already_AddRefed<nsIURI> GetURIToMutate();
  void SetHrefAttribute(nsIURI *aURI);

  mutable nsCOMPtr<nsIURI> mCachedURI;

  Element * const mElement;

  
  
  nsCOMPtr<IHistory> mHistory;

  uint16_t mLinkState;

  bool mNeedsRegistration;

  bool mRegistered;
};

NS_DEFINE_STATIC_IID_ACCESSOR(Link, MOZILLA_DOM_LINK_IMPLEMENTATION_IID)

} 
} 

#endif 

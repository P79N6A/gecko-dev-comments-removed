









#ifndef mozilla_dom_Link_h__
#define mozilla_dom_Link_h__

#include "mozilla/dom/Element.h"
#include "mozilla/IHistory.h"

namespace mozilla {
namespace dom {

#define MOZILLA_DOM_LINK_IMPLEMENTATION_IID \
  { 0x7EA57721, 0xE373, 0x458E, \
    {0x8F, 0x44, 0xF8, 0x96, 0x56, 0xB4, 0x14, 0xF5 } }

class Link : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(MOZILLA_DOM_LINK_IMPLEMENTATION_IID)

  static const nsLinkState defaultState = eLinkState_Unknown;

  


  Link(Element* aElement);
  nsLinkState GetLinkState() const;
  virtual void SetLinkState(nsLinkState aState);

  




  nsEventStates LinkState() const;

  


  already_AddRefed<nsIURI> GetURI() const;
  virtual already_AddRefed<nsIURI> GetURIExternal() const {
    return GetURI();
  }

  


  nsresult SetProtocol(const nsAString &aProtocol);
  nsresult SetHost(const nsAString &aHost);
  nsresult SetHostname(const nsAString &aHostname);
  nsresult SetPathname(const nsAString &aPathname);
  nsresult SetSearch(const nsAString &aSearch);
  nsresult SetPort(const nsAString &aPort);
  nsresult SetHash(const nsAString &aHash);
  nsresult GetProtocol(nsAString &_protocol);
  nsresult GetHost(nsAString &_host);
  nsresult GetHostname(nsAString &_hostname);
  nsresult GetPathname(nsAString &_pathname);
  nsresult GetSearch(nsAString &_search);
  nsresult GetPort(nsAString &_port);
  nsresult GetHash(nsAString &_hash);

  






  void ResetLinkState(bool aNotify);
  
  
  Element* GetElement() const { return mElement; }

  


  virtual void OnDNSPrefetchDeferred() {  }
  
  


  virtual void OnDNSPrefetchRequested() {  }

  





  virtual bool HasDeferredDNSPrefetchRequest() { return true; }

  virtual size_t
    SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

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

  bool mRegistered;
};

NS_DEFINE_STATIC_IID_ACCESSOR(Link, MOZILLA_DOM_LINK_IMPLEMENTATION_IID)

} 
} 

#endif 

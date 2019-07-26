









#ifndef mozilla_dom_Link_h__
#define mozilla_dom_Link_h__

#include "mozilla/IHistory.h"
#include "mozilla/MemoryReporting.h"
#include "nsEventStates.h"
#include "nsIContent.h"

namespace mozilla {
namespace dom {

class Element;

#define MOZILLA_DOM_LINK_IMPLEMENTATION_IID               \
{ 0xb25edee6, 0xdd35, 0x4f8b,                             \
  { 0xab, 0x90, 0x66, 0xd0, 0xbd, 0x3c, 0x22, 0xd5 } }

class Link : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(MOZILLA_DOM_LINK_IMPLEMENTATION_IID)

  


  Link(Element* aElement);
  virtual void SetLinkState(nsLinkState aState);

  




  nsEventStates LinkState() const;

  


  nsIURI* GetURI() const;
  virtual nsIURI* GetURIExternal() const {
    return GetURI();
  }

  


  void SetProtocol(const nsAString &aProtocol);
  void SetUsername(const nsAString &aUsername);
  void SetPassword(const nsAString &aPassword);
  void SetHost(const nsAString &aHost);
  void SetHostname(const nsAString &aHostname);
  void SetPathname(const nsAString &aPathname);
  void SetSearch(const nsAString &aSearch);
  void SetPort(const nsAString &aPort);
  void SetHash(const nsAString &aHash);
  void GetOrigin(nsAString &aOrigin);
  void GetProtocol(nsAString &_protocol);
  void GetUsername(nsAString &aUsername);
  void GetPassword(nsAString &aPassword);
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
    SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

  bool ElementHasHref() const;

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

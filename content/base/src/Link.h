










































#ifndef mozilla_dom_Link_h__
#define mozilla_dom_Link_h__

#include "mozilla/dom/Element.h"
#include "mozilla/IHistory.h"

namespace mozilla {
namespace dom {

#define MOZILLA_DOM_LINK_IMPLEMENTATION_IID \
  { 0xa687a99c, 0x3893, 0x45c0, \
    {0x8e, 0xab, 0xb8, 0xf7, 0xd7, 0x9e, 0x9e, 0x7b } }

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

protected:
  virtual ~Link();

  bool HasCachedURI() const { return !!mCachedURI; }

private:
  



  void UnregisterFromHistory();

  already_AddRefed<nsIURI> GetURIToMutate();
  void SetHrefAttribute(nsIURI *aURI);

  nsLinkState mLinkState;

  mutable nsCOMPtr<nsIURI> mCachedURI;

  bool mRegistered;

  Element * const mElement;

  
  
  nsCOMPtr<IHistory> mHistory;
};

NS_DEFINE_STATIC_IID_ACCESSOR(Link, MOZILLA_DOM_LINK_IMPLEMENTATION_IID)

} 
} 

#endif 












































#ifndef mozilla_dom_Link_h__
#define mozilla_dom_Link_h__

#include "nsIContent.h"

namespace mozilla {
namespace dom {

#define MOZILLA_DOM_LINK_IMPLEMENTATION_IID \
  { 0x2be8af0, 0x32a5, 0x41dd, \
    { 0xa7, 0x68, 0x12, 0x55, 0xc4, 0x9a, 0xd4, 0xc5 } }

class Link : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(MOZILLA_DOM_LINK_IMPLEMENTATION_IID)

  static const nsLinkState defaultState = eLinkState_Unknown;
  Link();
  nsLinkState GetLinkState() const;
  virtual void SetLinkState(nsLinkState aState);

  




  PRInt32 LinkState() const;

  


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

private:
  



  void UnregisterFromHistory();

  already_AddRefed<nsIURI> GetURIToMutate();
  void SetHrefAttribute(nsIURI *aURI);

  nsLinkState mLinkState;

  mutable nsCOMPtr<nsIURI> mCachedURI;

  bool mRegistered;

  



  nsIContent *Content();
  nsIContent *mContent;
};

NS_DEFINE_STATIC_IID_ACCESSOR(Link, MOZILLA_DOM_LINK_IMPLEMENTATION_IID)

} 
} 

#endif 












































#ifndef mozilla_dom_Link_h__
#define mozilla_dom_Link_h__

#include "nsIContent.h"

namespace mozilla {
namespace dom {

class Link : public nsISupports
{
public:
  static const nsLinkState defaultState = eLinkState_Unknown;
  Link();
  virtual nsLinkState GetLinkState() const;
  virtual void SetLinkState(nsLinkState aState);

  




  PRInt32 LinkState() const;

  


  already_AddRefed<nsIURI> GetURI() const;

  


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

protected:
  


  virtual void ResetLinkState();

  virtual ~Link();

private:
  



  void UnregisterFromHistory();

  already_AddRefed<nsIURI> GetURIToMutate();
  void SetHrefAttribute(nsIURI *aURI);

  nsLinkState mLinkState;

  mutable nsCOMPtr<nsIURI> mCachedURI;

  bool mRegistered;
};

} 
} 

#endif 

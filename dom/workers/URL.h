





#ifndef mozilla_dom_workers_url_h__
#define mozilla_dom_workers_url_h__

#include "mozilla/dom/URLBinding.h"

#include "EventTarget.h"

BEGIN_WORKERS_NAMESPACE

class URL : public EventTarget
{
public: 
  static URL*
  Constructor(const GlobalObject& aGlobal, const nsAString& aUrl,
              URL& aBase, ErrorResult& aRv);
  static URL*
  Constructor(const GlobalObject& aGlobal, const nsAString& aUrl,
              const nsAString& aBase, ErrorResult& aRv);

  static void
  CreateObjectURL(const GlobalObject& aGlobal,
                  JSObject* aArg, const objectURLOptions& aOptions,
                  nsString& aResult, ErrorResult& aRv);

  static void
  CreateObjectURL(const GlobalObject& aGlobal,
                  JSObject& aArg, const objectURLOptions& aOptions,
                  nsString& aResult, ErrorResult& aRv);

  static void
  RevokeObjectURL(const GlobalObject& aGlobal, const nsAString& aUrl);

  void GetHref(nsString& aHref) const;

  void SetHref(const nsAString& aHref, ErrorResult& aRv);

  void GetOrigin(nsString& aOrigin) const;

  void GetProtocol(nsString& aProtocol) const;

  void SetProtocol(const nsAString& aProtocol);

  void GetUsername(nsString& aUsername) const;

  void SetUsername(const nsAString& aUsername);

  void GetPassword(nsString& aPassword) const;

  void SetPassword(const nsAString& aPassword);

  void GetHost(nsString& aHost) const;

  void SetHost(const nsAString& aHost);

  void GetHostname(nsString& aHostname) const;

  void SetHostname(const nsAString& aHostname);

  void GetPort(nsString& aPort) const;

  void SetPort(const nsAString& aPort);

  void GetPathname(nsString& aPathname) const;

  void SetPathname(const nsAString& aPathname);

  void GetSearch(nsString& aSearch) const;

  void SetSearch(const nsAString& aSearch);

  void GetHash(nsString& aHost) const;

  void SetHash(const nsAString& aHash);

private:
  mozilla::dom::URL* GetURL() const
  {
    return mURL;
  }

  nsRefPtr<mozilla::dom::URL> mURL;
};

END_WORKERS_NAMESPACE

#endif 





#ifndef _JSEPTRANSPORT_H_
#define _JSEPTRANSPORT_H_

#include <string>
#include <vector>

#include <mozilla/RefPtr.h>
#include <mozilla/UniquePtr.h>
#include "nsISupportsImpl.h"

#include "signaling/src/sdp/SdpAttribute.h"

namespace mozilla {

class JsepDtlsTransport
{
public:
  JsepDtlsTransport() : mRole(kJsepDtlsInvalidRole) {}

  virtual ~JsepDtlsTransport() {}

  enum Role {
    kJsepDtlsClient,
    kJsepDtlsServer,
    kJsepDtlsInvalidRole
  };

  virtual const SdpFingerprintAttributeList&
  GetFingerprints() const
  {
    return mFingerprints;
  }

  virtual Role
  GetRole() const
  {
    return mRole;
  }

private:
  friend class JsepSessionImpl;

  SdpFingerprintAttributeList mFingerprints;
  Role mRole;
};

class JsepIceTransport
{
public:
  JsepIceTransport() {}

  virtual ~JsepIceTransport() {}

  const std::string&
  GetUfrag() const
  {
    return mUfrag;
  }
  const std::string&
  GetPassword() const
  {
    return mPwd;
  }
  const std::vector<std::string>&
  GetCandidates() const
  {
    return mCandidates;
  }

private:
  friend class JsepSessionImpl;

  std::string mUfrag;
  std::string mPwd;
  std::vector<std::string> mCandidates;
};

class JsepTransport
{
public:
  JsepTransport(const std::string& id, size_t components)
      : mTransportId(id), mState(kJsepTransportOffered), mComponents(components)
  {
  }

  enum State {
    kJsepTransportOffered,
    kJsepTransportAccepted,
    kJsepTransportClosed
  };

  
  std::string mTransportId;

  
  State mState;

  
  UniquePtr<JsepIceTransport> mIce;
  UniquePtr<JsepDtlsTransport> mDtls;

  
  size_t mComponents;

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(JsepTransport);

protected:
  ~JsepTransport() {}
};

} 

#endif






#ifndef mozilla_dom_PresentationSessionRequest_h__
#define mozilla_dom_PresentationSessionRequest_h__

#include "nsIPresentationSessionRequest.h"
#include "nsCOMPtr.h"
#include "nsString.h"

namespace mozilla {
namespace dom {

class PresentationSessionRequest final : public nsIPresentationSessionRequest
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPRESENTATIONSESSIONREQUEST

  PresentationSessionRequest(nsIPresentationDevice* aDevice,
                             const nsAString& aUrl,
                             const nsAString& aPresentationId,
                             nsIPresentationControlChannel* aControlChannel);

private:
  virtual ~PresentationSessionRequest();

  nsString mUrl;
  nsString mPresentationId;
  nsCOMPtr<nsIPresentationDevice> mDevice;
  nsCOMPtr<nsIPresentationControlChannel> mControlChannel;
};

} 
} 

#endif 


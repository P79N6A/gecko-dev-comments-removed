





#ifndef mozilla_dom_fmradiorequestchild_h__
#define mozilla_dom_fmradiorequestchild_h__

#include "FMRadioCommon.h"
#include "mozilla/dom/PFMRadioRequestChild.h"
#include "DOMRequest.h"

BEGIN_FMRADIO_NAMESPACE

class ReplyRunnable;

class FMRadioRequestChild MOZ_FINAL : public PFMRadioRequestChild
{
public:
  FMRadioRequestChild(ReplyRunnable* aReplyRunnable);
  ~FMRadioRequestChild();

  virtual bool
  Recv__delete__(const FMRadioResponseType& aResponse) MOZ_OVERRIDE;

private:
  nsRefPtr<ReplyRunnable> mReplyRunnable;
};

END_FMRADIO_NAMESPACE

#endif 


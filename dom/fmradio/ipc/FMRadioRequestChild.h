





#ifndef mozilla_dom_fmradiorequestchild_h__
#define mozilla_dom_fmradiorequestchild_h__

#include "FMRadioCommon.h"
#include "mozilla/dom/PFMRadioRequestChild.h"
#include "DOMRequest.h"

BEGIN_FMRADIO_NAMESPACE

class FMRadioReplyRunnable;

class FMRadioRequestChild final : public PFMRadioRequestChild
{
public:
  FMRadioRequestChild(FMRadioReplyRunnable* aReplyRunnable);
  ~FMRadioRequestChild();

  virtual bool
  Recv__delete__(const FMRadioResponseType& aResponse) override;

private:
  nsRefPtr<FMRadioReplyRunnable> mReplyRunnable;
};

END_FMRADIO_NAMESPACE

#endif 


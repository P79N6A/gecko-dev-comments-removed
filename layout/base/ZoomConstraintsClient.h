




#ifndef ZoomConstraintsClient_h_
#define ZoomConstraintsClient_h_

#include "FrameMetrics.h"
#include "mozilla/Maybe.h"
#include "nsIDOMEventListener.h"
#include "nsIDocument.h"
#include "nsIObserver.h"
#include "nsWeakPtr.h"

class nsIDOMEventTarget;
class nsIDocument;
class nsIPresShell;

class ZoomConstraintsClient final : public nsIDOMEventListener,
                                    public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER
  NS_DECL_NSIOBSERVER

  ZoomConstraintsClient();

private:
  ~ZoomConstraintsClient();

public:
  void Init(nsIPresShell* aPresShell, nsIDocument *aDocument);
  void Destroy();

private:
  void RefreshZoomConstraints();

  nsCOMPtr<nsIDocument> mDocument;
  nsIPresShell* MOZ_NON_OWNING_REF mPresShell; 
  nsCOMPtr<nsIDOMEventTarget> mEventTarget;
  mozilla::Maybe<mozilla::layers::ScrollableLayerGuid> mGuid;
};

#endif


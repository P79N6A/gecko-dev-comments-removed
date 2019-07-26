




#ifndef nsDOMWindowUtils_h_
#define nsDOMWindowUtils_h_

#include "nsWeakReference.h"

#include "nsIDOMWindowUtils.h"
#include "mozilla/Attributes.h"
#include "mozilla/BasicEvents.h"

class nsGlobalWindow;
class nsIPresShell;
class nsIWidget;
class nsPresContext;
class nsIDocument;
struct nsPoint;

namespace mozilla {
  namespace layers {
    class LayerTransactionChild;
  }
}

class nsTranslationNodeList MOZ_FINAL : public nsITranslationNodeList
{
public:
  nsTranslationNodeList()
  {
    mNodes.SetCapacity(1000);
    mNodeIsRoot.SetCapacity(1000);
    mLength = 0;
  }

  NS_DECL_ISUPPORTS
  NS_DECL_NSITRANSLATIONNODELIST

  void AppendElement(nsIDOMNode* aElement, bool aIsRoot)
  {
    mNodes.AppendElement(aElement);
    mNodeIsRoot.AppendElement(aIsRoot);
    mLength++;
  }

private:
  nsTArray<nsCOMPtr<nsIDOMNode> > mNodes;
  nsTArray<bool> mNodeIsRoot;
  uint32_t mLength;
};

class nsDOMWindowUtils MOZ_FINAL : public nsIDOMWindowUtils,
                                   public nsSupportsWeakReference
{
public:
  nsDOMWindowUtils(nsGlobalWindow *aWindow);
  ~nsDOMWindowUtils();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMWINDOWUTILS

protected:
  nsWeakPtr mWindow;

  
  
  
  
  nsIWidget* GetWidget(nsPoint* aOffset = nullptr);
  nsIWidget* GetWidgetForElement(nsIDOMElement* aElement);

  nsIPresShell* GetPresShell();
  nsPresContext* GetPresContext();
  nsIDocument* GetDocument();
  mozilla::layers::LayerTransactionChild* GetLayerTransaction();

  NS_IMETHOD SendMouseEventCommon(const nsAString& aType,
                                  float aX,
                                  float aY,
                                  int32_t aButton,
                                  int32_t aClickCount,
                                  int32_t aModifiers,
                                  bool aIgnoreRootScrollFrame,
                                  float aPressure,
                                  unsigned short aInputSourceArg,
                                  bool aToWindow,
                                  bool *aPreventDefault,
                                  bool aIsSynthesized);

  NS_IMETHOD SendTouchEventCommon(const nsAString& aType,
                                  uint32_t* aIdentifiers,
                                  int32_t* aXs,
                                  int32_t* aYs,
                                  uint32_t* aRxs,
                                  uint32_t* aRys,
                                  float* aRotationAngles,
                                  float* aForces,
                                  uint32_t aCount,
                                  int32_t aModifiers,
                                  bool aIgnoreRootScrollFrame,
                                  bool aToWindow,
                                  bool* aPreventDefault);


  static mozilla::Modifiers GetWidgetModifiers(int32_t aModifiers);
};

#endif

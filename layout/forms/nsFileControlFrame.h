




#ifndef nsFileControlFrame_h___
#define nsFileControlFrame_h___

#include "mozilla/Attributes.h"
#include "nsBlockFrame.h"
#include "nsIFormControlFrame.h"
#include "nsIDOMEventListener.h"
#include "nsIAnonymousContentCreator.h"
#include "nsCOMPtr.h"

class nsIDOMDataTransfer;

class nsFileControlFrame : public nsBlockFrame,
                           public nsIFormControlFrame,
                           public nsIAnonymousContentCreator
{
public:
  explicit nsFileControlFrame(nsStyleContext* aContext);

  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) override;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) override;

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  virtual nsresult SetFormProperty(nsIAtom* aName, const nsAString& aValue) override;
  virtual void SetFocus(bool aOn, bool aRepaint) override;

  virtual nscoord GetMinISize(nsRenderingContext *aRenderingContext) override;

  virtual void DestroyFrom(nsIFrame* aDestructRoot) override;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override;
#endif

  virtual nsresult AttributeChanged(int32_t         aNameSpaceID,
                                    nsIAtom*        aAttribute,
                                    int32_t         aModType) override;
  virtual void ContentStatesChanged(mozilla::EventStates aStates) override;
  virtual bool IsLeaf() const override
  {
    return true;
  }

  
  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements) override;
  virtual void AppendAnonymousContentTo(nsTArray<nsIContent*>& aElements,
                                        uint32_t aFilter) override;

#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() override;
#endif

  typedef bool (*AcceptAttrCallback)(const nsAString&, void*);

protected:

  class MouseListener;
  friend class MouseListener;
  class MouseListener : public nsIDOMEventListener {
  public:
    NS_DECL_ISUPPORTS

    explicit MouseListener(nsFileControlFrame* aFrame)
     : mFrame(aFrame)
    {}

    void ForgetFrame() {
      mFrame = nullptr;
    }

  protected:
    virtual ~MouseListener() {}

    nsFileControlFrame* mFrame;
  };

  class SyncDisabledStateEvent;
  friend class SyncDisabledStateEvent;
  class SyncDisabledStateEvent : public nsRunnable
  {
  public:
    explicit SyncDisabledStateEvent(nsFileControlFrame* aFrame)
      : mFrame(aFrame)
    {}

    NS_IMETHOD Run() override {
      nsFileControlFrame* frame = static_cast<nsFileControlFrame*>(mFrame.GetFrame());
      NS_ENSURE_STATE(frame);

      frame->SyncDisabledState();
      return NS_OK;
    }

  private:
    nsWeakFrame mFrame;
  };

  class DnDListener: public MouseListener {
  public:
    explicit DnDListener(nsFileControlFrame* aFrame)
      : MouseListener(aFrame)
    {}

    NS_DECL_NSIDOMEVENTLISTENER

    static bool IsValidDropData(nsIDOMDataTransfer* aDOMDataTransfer);
    static bool CanDropTheseFiles(nsIDOMDataTransfer* aDOMDataTransfer, bool aSupportsMultiple);
  };

  virtual bool IsFrameOfType(uint32_t aFlags) const override
  {
    return nsBlockFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eReplaced | nsIFrame::eReplacedContainsBlock));
  }

  



  nsCOMPtr<nsIContent> mTextContent;
  



  nsCOMPtr<nsIContent> mBrowse;

  



  nsRefPtr<DnDListener> mMouseListener;

protected:
  


  void SyncDisabledState();

  


  void UpdateDisplayedValue(const nsAString& aValue, bool aNotify);
};

#endif 

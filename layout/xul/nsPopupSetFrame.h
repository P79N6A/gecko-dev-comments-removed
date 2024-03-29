





#ifndef nsPopupSetFrame_h__
#define nsPopupSetFrame_h__

#include "mozilla/Attributes.h"
#include "nsIAtom.h"
#include "nsBoxFrame.h"

nsIFrame* NS_NewPopupSetFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

class nsPopupSetFrame : public nsBoxFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  explicit nsPopupSetFrame(nsStyleContext* aContext):
    nsBoxFrame(aContext) {}

  ~nsPopupSetFrame() {}
  
  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) override;

  virtual void SetInitialChildList(ChildListID  aListID,
                                    nsFrameList& aChildList) override;
  virtual void AppendFrames(ChildListID     aListID,
                            nsFrameList&    aFrameList) override;
  virtual void RemoveFrame(ChildListID     aListID,
                           nsIFrame*       aOldFrame) override;
  virtual void InsertFrames(ChildListID     aListID,
                            nsIFrame*       aPrevFrame,
                            nsFrameList&    aFrameList) override;

  virtual const nsFrameList& GetChildList(ChildListID aList) const override;
  virtual void GetChildLists(nsTArray<ChildList>* aLists) const override;

  NS_IMETHOD DoLayout(nsBoxLayoutState& aBoxLayoutState) override;

  
  virtual void DestroyFrom(nsIFrame* aDestructRoot) override;

  virtual nsIAtom* GetType() const override;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override
  {
      return MakeFrameName(NS_LITERAL_STRING("PopupSet"), aResult);
  }
#endif

protected:
  void AddPopupFrameList(nsFrameList& aPopupFrameList);
  void RemovePopupFrame(nsIFrame* aPopup);
  
  nsFrameList mPopupList;
};

#endif

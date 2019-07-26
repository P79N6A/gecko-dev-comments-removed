





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

  nsPopupSetFrame(nsIPresShell* aShell, nsStyleContext* aContext):
    nsBoxFrame(aShell, aContext) {}

  ~nsPopupSetFrame() {}
  
  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;
  virtual nsresult AppendFrames(ChildListID     aListID,
                                nsFrameList&    aFrameList) MOZ_OVERRIDE;
  virtual nsresult RemoveFrame(ChildListID     aListID,
                               nsIFrame*       aOldFrame) MOZ_OVERRIDE;
  virtual nsresult InsertFrames(ChildListID     aListID,
                                nsIFrame*       aPrevFrame,
                                nsFrameList&    aFrameList) MOZ_OVERRIDE;
  virtual nsresult  SetInitialChildList(ChildListID  aListID,
                                        nsFrameList& aChildList) MOZ_OVERRIDE;

  virtual const nsFrameList& GetChildList(ChildListID aList) const MOZ_OVERRIDE;
  virtual void GetChildLists(nsTArray<ChildList>* aLists) const MOZ_OVERRIDE;

  NS_IMETHOD DoLayout(nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;

  
  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
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

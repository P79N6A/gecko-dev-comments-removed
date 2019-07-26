




#include "mozilla/Attributes.h"
#include "nsBoxFrame.h"

class nsITreeBoxObject;

nsIFrame* NS_NewTreeColFrame(nsIPresShell* aPresShell, 
                             nsStyleContext* aContext);

class nsTreeColFrame : public nsBoxFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  nsTreeColFrame(nsIPresShell* aPresShell,
                 nsStyleContext* aContext):
    nsBoxFrame(aPresShell, aContext) {}

  virtual void Init(nsIContent*      aContent,
                    nsIFrame*        aParent,
                    nsIFrame*        aPrevInFlow) MOZ_OVERRIDE;

  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

  virtual void BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                           const nsRect&           aDirtyRect,
                                           const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  NS_IMETHOD AttributeChanged(int32_t aNameSpaceID,
                              nsIAtom* aAttribute,
                              int32_t aModType) MOZ_OVERRIDE;

  virtual void SetBounds(nsBoxLayoutState& aBoxLayoutState, const nsRect& aRect,
                         bool aRemoveOverflowArea = false) MOZ_OVERRIDE;

  friend nsIFrame* NS_NewTreeColFrame(nsIPresShell* aPresShell,
                                      nsStyleContext* aContext);

protected:
  virtual ~nsTreeColFrame();

  


  nsITreeBoxObject* GetTreeBoxObject();

  



  void InvalidateColumns(bool aCanWalkFrameTree = true);
};

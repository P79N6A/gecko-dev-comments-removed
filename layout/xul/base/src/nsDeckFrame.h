












#ifndef nsDeckFrame_h___
#define nsDeckFrame_h___

#include "mozilla/Attributes.h"
#include "nsBoxFrame.h"

class nsDeckFrame : public nsBoxFrame
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsDeckFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewDeckFrame(nsIPresShell* aPresShell,
                                   nsStyleContext* aContext);

  NS_IMETHOD AttributeChanged(int32_t         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              int32_t         aModType) MOZ_OVERRIDE;

  NS_IMETHOD DoLayout(nsBoxLayoutState& aState) MOZ_OVERRIDE;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  virtual void BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                           const nsRect&           aDirtyRect,
                                           const nsDisplayListSet& aLists) MOZ_OVERRIDE;
                                         
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow) MOZ_OVERRIDE;

  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
      return MakeFrameName(NS_LITERAL_STRING("Deck"), aResult);
  }
#endif

  nsDeckFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  nsIFrame* GetSelectedBox();

protected:

  void IndexChanged();
  int32_t GetSelectedIndex();
  void HideBox(nsIFrame* aBox);

private:

  int32_t mIndex;

}; 

#endif


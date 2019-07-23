






































#ifndef nsImageMap_h___
#define nsImageMap_h___

#include "nsISupports.h"
#include "nsCoord.h"
#include "nsVoidArray.h"
#include "nsStubMutationObserver.h"
#include "nsIDOMFocusListener.h"
#include "nsIFrame.h"
#include "nsIImageMap.h"

class nsIDOMHTMLAreaElement;
class nsIDOMHTMLMapElement;
class nsPresContext;
class nsIRenderingContext;
class nsIURI;
class nsString;
class nsIDOMEvent;

class nsImageMap : public nsStubMutationObserver, public nsIDOMFocusListener,
                   public nsIImageMap
{
public:
  nsImageMap();

  nsresult Init(nsIPresShell* aPresShell, nsIFrame* aImageFrame, nsIDOMHTMLMapElement* aMap);

  





  PRBool IsInside(nscoord aX, nscoord aY,
                  nsIContent** aContent) const;

  void Draw(nsPresContext* aCX, nsIRenderingContext& aRC);
  
  



  void Destroy(void);
  
  
  NS_DECL_ISUPPORTS

  
  virtual void AttributeChanged(nsIDocument* aDocument, nsIContent* aContent,
                                PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                                PRInt32 aModType);
  virtual void ContentAppended(nsIDocument* aDocument, nsIContent* aContainer,
                               PRInt32 aNewIndexInContainer);
  virtual void ContentInserted(nsIDocument* aDocument, nsIContent* aContainer,
                               nsIContent* aChild, PRInt32 aIndexInContainer);
  virtual void ContentRemoved(nsIDocument* aDocument, nsIContent* aContainer,
                              nsIContent* aChild, PRInt32 aIndexInContainer);

  
  NS_IMETHOD Focus(nsIDOMEvent* aEvent);
  NS_IMETHOD Blur(nsIDOMEvent* aEvent);
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);

  
  NS_IMETHOD GetBoundsForAreaContent(nsIContent *aContent, 
                                     nsPresContext* aPresContext, 
                                     nsRect& aBounds);

protected:
  virtual ~nsImageMap();

  void FreeAreas();

  nsresult UpdateAreas();
  nsresult SearchForAreas(nsIContent* aParent, PRBool& aFoundArea,
                         PRBool& aFoundAnchor);

  nsresult AddArea(nsIContent* aArea);
 
  nsresult ChangeFocus(nsIDOMEvent* aEvent, PRBool aFocus);

  void MaybeUpdateAreas(nsIContent *aContent);

  nsIPresShell* mPresShell; 
  nsIFrame* mImageFrame;  
  nsCOMPtr<nsIContent> mMap;
  nsAutoVoidArray mAreas; 
  PRBool mContainsBlockContents;
};

#endif 








































#ifndef nsImageMap_h
#define nsImageMap_h

#include "nsISupports.h"
#include "nsCoord.h"
#include "nsTArray.h"
#include "nsStubMutationObserver.h"
#include "nsIDOMFocusListener.h"
#include "nsIFrame.h"

class Area;
class nsIDOMEvent;
class nsRenderingContext;

class nsImageMap : public nsStubMutationObserver, public nsIDOMFocusListener
{
public:
  nsImageMap();

  nsresult Init(nsIPresShell* aPresShell, nsIFrame* aImageFrame, nsIContent* aMap);

  





  PRBool IsInside(nscoord aX, nscoord aY,
                  nsIContent** aContent) const;

  void Draw(nsIFrame* aFrame, nsRenderingContext& aRC);
  
  



  void Destroy();
  
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  
  NS_IMETHOD Focus(nsIDOMEvent* aEvent);
  NS_IMETHOD Blur(nsIDOMEvent* aEvent);
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);

  nsresult GetBoundsForAreaContent(nsIContent *aContent,
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
  nsAutoTArray<Area*, 8> mAreas; 
  PRBool mContainsBlockContents;
};

#endif 

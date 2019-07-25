






































#ifndef nsImageMap_h
#define nsImageMap_h

#include "nsISupports.h"
#include "nsCoord.h"
#include "nsTArray.h"
#include "nsStubMutationObserver.h"
#include "nsIDOMEventListener.h"
#include "nsIFrame.h"

class Area;
class nsIDOMEvent;
class nsRenderingContext;
class nsImageFrame;

class nsImageMap : public nsStubMutationObserver,
                   public nsIDOMEventListener
{
public:
  nsImageMap();

  nsresult Init(nsImageFrame* aImageFrame, nsIContent* aMap);

  





  bool IsInside(nscoord aX, nscoord aY,
                  nsIContent** aContent) const;

  void Draw(nsIFrame* aFrame, nsRenderingContext& aRC);
  
  



  void Destroy();
  
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED
  NS_DECL_NSIMUTATIONOBSERVER_PARENTCHAINCHANGED  

  
  NS_DECL_NSIDOMEVENTLISTENER

  nsresult GetBoundsForAreaContent(nsIContent *aContent,
                                   nsRect& aBounds);

protected:
  virtual ~nsImageMap();

  void FreeAreas();

  nsresult UpdateAreas();
  nsresult SearchForAreas(nsIContent* aParent, bool& aFoundArea,
                          bool& aFoundAnchor);

  nsresult AddArea(nsIContent* aArea);
 
  void MaybeUpdateAreas(nsIContent *aContent);

  nsImageFrame* mImageFrame;  
  nsCOMPtr<nsIContent> mMap;
  nsAutoTArray<Area*, 8> mAreas; 
  bool mContainsBlockContents;
};

#endif 

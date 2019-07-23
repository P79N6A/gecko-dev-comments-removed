





































#ifndef nsContentAreaDragDrop_h__
#define nsContentAreaDragDrop_h__


#include "nsCOMPtr.h"

#include "nsIDragDropHandler.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMEventListener.h"
#include "nsITransferable.h"

class nsIDOMNode;
class nsIDOMWindow;
class nsIDOMDocument;
class nsIDOMDragEvent;
class nsISelection;
class nsITransferable;
class nsIPresShell;
class nsPresContext;
class nsIContent;
class nsIURI;
class nsIFile;
class nsISimpleEnumerator;
class nsDOMDataTransfer;


#define NS_CONTENTAREADRAGDROP_CID             \
{ 0x1f34bc80, 0x1bc7, 0x11d6, \
  { 0xa3, 0x84, 0xd7, 0x05, 0xdd, 0x07, 0x46, 0xfc } }

#define NS_CONTENTAREADRAGDROP_CONTRACTID "@mozilla.org:/content/content-area-dragdrop;1"










class nsContentAreaDragDrop : public nsIDragDropHandler,
                              public nsIDOMEventListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDRAGDROPHANDLER
  
  nsContentAreaDragDrop();
  virtual ~nsContentAreaDragDrop();

  NS_IMETHOD HandleEvent(nsIDOMEvent *event);

  

















  static nsresult GetDragData(nsIDOMWindow* aWindow,
                              nsIContent* aTarget,
                              nsIContent* aSelectionTargetNode,
                              PRBool aIsAltKeyPressed,
                              nsDOMDataTransfer* aDataTransfer,
                              PRBool* aCanDrag,
                              PRBool* aDragSelection,
                              nsIContent** aDragNode);

private:

  
  nsresult AddDragListener();
  nsresult RemoveDragListener();

  nsresult DragOver(nsIDOMDragEvent* aDragEvent);
  nsresult Drop(nsIDOMDragEvent* aDragEvent);

  
  static void NormalizeSelection(nsIDOMNode* inBaseNode,
                                 nsISelection* inSelection);
  static void GetEventDocument(nsIDOMEvent* inEvent,
                               nsIDOMDocument** outDocument);

  static void ExtractURLFromData(const nsACString & inFlavor,
                                 nsISupports* inDataWrapper, PRUint32 inDataLen,
                                 nsAString & outURL);

  nsCOMPtr<nsIDOMEventTarget> mEventTarget;

  
  
  nsIWebNavigation* mNavigator;

};




class nsContentAreaDragDropDataProvider : public nsIFlavorDataProvider
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFLAVORDATAPROVIDER

  virtual ~nsContentAreaDragDropDataProvider() {}

  nsresult SaveURIToFile(nsAString& inSourceURIString,
                         nsIFile* inDestFile);
};


#endif 


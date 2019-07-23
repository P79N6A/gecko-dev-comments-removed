





































#ifndef nsContentAreaDragDrop_h__
#define nsContentAreaDragDrop_h__


#include "nsCOMPtr.h"

#include "nsIDragDropHandler.h"
#include "nsIDOMDragListener.h"
#include "nsPIDOMEventTarget.h"
#include "nsITransferable.h"

class nsIDOMNode;
class nsISelection;
class nsITransferable;
class nsIImage;
class nsIPresShell;
class nsPresContext;
class nsIContent;
class nsIDocument;
class nsIURI;
class nsIFile;
class nsISimpleEnumerator;
class nsDOMDataTransfer;


#define NS_CONTENTAREADRAGDROP_CID             \
{ 0x1f34bc80, 0x1bc7, 0x11d6, \
  { 0xa3, 0x84, 0xd7, 0x05, 0xdd, 0x07, 0x46, 0xfc } }

#define NS_CONTENTAREADRAGDROP_CONTRACTID "@mozilla.org:/content/content-area-dragdrop;1"










class nsContentAreaDragDrop : public nsIDOMDragListener,
                              public nsIDragDropHandler
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDRAGDROPHANDLER
  
  nsContentAreaDragDrop();
  virtual ~nsContentAreaDragDrop();

  
  NS_IMETHOD DragEnter(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD DragOver(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD DragExit(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD DragDrop(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD DragGesture(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD Drag(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD DragEnd(nsIDOMEvent* aMouseEvent);
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

  
  static void NormalizeSelection(nsIDOMNode* inBaseNode,
                                 nsISelection* inSelection);
  static void GetEventDocument(nsIDOMEvent* inEvent,
                               nsIDOMDocument** outDocument);

  static void ExtractURLFromData(const nsACString & inFlavor,
                                 nsISupports* inDataWrapper, PRUint32 inDataLen,
                                 nsAString & outURL);

  PRPackedBool mListenerInstalled;

  nsCOMPtr<nsPIDOMEventTarget> mEventTarget;

  
  
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


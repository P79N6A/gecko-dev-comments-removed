





































#ifndef nsContentAreaDragDrop_h__
#define nsContentAreaDragDrop_h__


#include "nsCOMPtr.h"

#include "nsIDragDropHandler.h"
#include "nsIDOMDragListener.h"
#include "nsIDOMEventReceiver.h"
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


#define NS_CONTENTAREADRAGDROP_CID             \
{ 0x1f34bc80, 0x1bc7, 0x11d6, \
  { 0xa3, 0x84, 0xd7, 0x05, 0xdd, 0x07, 0x46, 0xfc } }

#define NS_CONTENTAREADRAGDROP_CONTRACTID "@mozilla.org:/content/content-area-dragdrop;1"










class nsContentAreaDragDrop : public nsIDOMDragListener,
                              public nsIDragDropHandler,
                              public nsIFlavorDataProvider
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDRAGDROPHANDLER
  NS_DECL_NSIFLAVORDATAPROVIDER
  
  nsContentAreaDragDrop();
  virtual ~nsContentAreaDragDrop();

  
  NS_IMETHOD DragEnter(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD DragOver(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD DragExit(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD DragDrop(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD DragGesture(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD HandleEvent(nsIDOMEvent *event);

private:

  
  nsresult AddDragListener();
  nsresult RemoveDragListener();

  
  static void NormalizeSelection(nsIDOMNode* inBaseNode,
                                 nsISelection* inSelection);
  static void GetEventDocument(nsIDOMEvent* inEvent,
                               nsIDOMDocument** outDocument);

  static nsresult SaveURIToFile(nsAString& inSourceURIString,
                                nsIFile* inDestFile);

  void ExtractURLFromData(const nsACString & inFlavor,
                          nsISupports* inDataWrapper, PRUint32 inDataLen,
                          nsAString & outURL);
  nsresult GetHookEnumeratorFromEvent(nsIDOMEvent* inEvent,
                                      nsISimpleEnumerator** outEnumerator);

  PRPackedBool mListenerInstalled;

  nsCOMPtr<nsIDOMEventReceiver> mEventReceiver;

  
  
  nsIWebNavigation* mNavigator;

};



#endif 


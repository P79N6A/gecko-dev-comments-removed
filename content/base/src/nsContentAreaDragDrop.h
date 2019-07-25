





































#ifndef nsContentAreaDragDrop_h__
#define nsContentAreaDragDrop_h__


#include "nsCOMPtr.h"

#include "nsIDOMEventTarget.h"
#include "nsIDOMEventListener.h"
#include "nsITransferable.h"

class nsIDOMNode;
class nsIDOMWindow;
class nsIDOMDocument;
class nsIDOMDragEvent;
class nsISelection;
class nsITransferable;
class nsIContent;
class nsIURI;
class nsIFile;
class nsISimpleEnumerator;
class nsDOMDataTransfer;




class nsContentAreaDragDrop
{
public:

  

















  static nsresult GetDragData(nsIDOMWindow* aWindow,
                              nsIContent* aTarget,
                              nsIContent* aSelectionTargetNode,
                              bool aIsAltKeyPressed,
                              nsDOMDataTransfer* aDataTransfer,
                              bool* aCanDrag,
                              bool* aDragSelection,
                              nsIContent** aDragNode);
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


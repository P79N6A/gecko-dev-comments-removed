




#ifndef nsContentAreaDragDrop_h__
#define nsContentAreaDragDrop_h__


#include "nsCOMPtr.h"

#include "nsIDOMEventListener.h"
#include "nsITransferable.h"

class nsPIDOMWindow;
class nsISelection;
class nsITransferable;
class nsIContent;
class nsIFile;

namespace mozilla {
namespace dom {
class DataTransfer;
}
}




class nsContentAreaDragDrop
{
public:

  

















  static nsresult GetDragData(nsPIDOMWindow* aWindow,
                              nsIContent* aTarget,
                              nsIContent* aSelectionTargetNode,
                              bool aIsAltKeyPressed,
                              mozilla::dom::DataTransfer* aDataTransfer,
                              bool* aCanDrag,
                              nsISelection** aSelection,
                              nsIContent** aDragNode);
};




class nsContentAreaDragDropDataProvider : public nsIFlavorDataProvider
{
  virtual ~nsContentAreaDragDropDataProvider() {}

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFLAVORDATAPROVIDER

  nsresult SaveURIToFile(nsAString& inSourceURIString,
                         nsIFile* inDestFile, bool isPrivate);
};


#endif 


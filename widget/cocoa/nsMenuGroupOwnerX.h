




#ifndef nsMenuGroupOwnerX_h_
#define nsMenuGroupOwnerX_h_

#import <Cocoa/Cocoa.h>

#include "nsMenuBaseX.h"
#include "nsIMutationObserver.h"
#include "nsHashKeys.h"
#include "nsDataHashtable.h"
#include "nsAutoPtr.h"
#include "nsString.h"


class nsMenuX;
class nsMenuItemX;
class nsChangeObserver;
class nsIWidget;
class nsIContent;
class nsIDocument;

class nsMenuGroupOwnerX : public nsMenuObjectX, public nsIMutationObserver
{
public:
  nsMenuGroupOwnerX();
  virtual ~nsMenuGroupOwnerX();

  nsresult Create(nsIContent * aContent);

  void RegisterForContentChanges(nsIContent* aContent,
                                 nsChangeObserver* aMenuObject);
  void UnregisterForContentChanges(nsIContent* aContent);
  uint32_t RegisterForCommand(nsMenuItemX* aItem);
  void UnregisterCommand(uint32_t aCommandID);
  nsMenuItemX* GetMenuItemForCommandID(uint32_t inCommandID);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIMUTATIONOBSERVER

protected:
  nsChangeObserver* LookupContentChangeObserver(nsIContent* aContent);

  uint32_t  mCurrentCommandID;  
                                
  nsIDocument* mDocument;       

  
  nsDataHashtable<nsPtrHashKey<nsIContent>, nsChangeObserver *> mContentToObserverTable;

  
  nsDataHashtable<nsUint32HashKey, nsMenuItemX *> mCommandToMenuObjectTable;
};

#endif 

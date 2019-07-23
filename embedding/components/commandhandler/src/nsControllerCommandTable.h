





































#ifndef nsControllerCommandTable_h_
#define nsControllerCommandTable_h_


#include "nsIControllerCommandTable.h"
#include "nsWeakReference.h"
#include "nsHashtable.h"

class nsIControllerCommand;

class  nsControllerCommandTable : public nsIControllerCommandTable,
                                  public nsSupportsWeakReference
{
public:

                  nsControllerCommandTable();
  virtual         ~nsControllerCommandTable();

  NS_DECL_ISUPPORTS

  NS_DECL_NSICONTROLLERCOMMANDTABLE

protected:

  nsSupportsHashtable   mCommandsTable;   
  PRBool                mMutable;         
};


#endif 

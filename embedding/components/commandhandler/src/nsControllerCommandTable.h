




#ifndef nsControllerCommandTable_h_
#define nsControllerCommandTable_h_


#include "nsIControllerCommandTable.h"
#include "nsWeakReference.h"
#include "nsInterfaceHashtable.h"

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

  
  nsInterfaceHashtable<nsCStringHashKey, nsIControllerCommand> mCommandsTable;

  
  bool mMutable;
};


#endif 

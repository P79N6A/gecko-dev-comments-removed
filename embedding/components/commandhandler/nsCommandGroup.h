




#ifndef nsCommandGroup_h__
#define nsCommandGroup_h__

#include "nsIController.h"
#include "nsClassHashtable.h"
#include "nsHashKeys.h"


#define NS_CONTROLLER_COMMAND_GROUP_CID \
{ 0xecd55a01, 0x2780, 0x11d5, { 0xa7, 0x3c, 0xca, 0x64, 0x1a, 0x68, 0x13, 0xbc } }

#define NS_CONTROLLER_COMMAND_GROUP_CONTRACTID \
 "@mozilla.org/embedcomp/controller-command-group;1"

class nsControllerCommandGroup : public nsIControllerCommandGroup
{
public:
  nsControllerCommandGroup();

  NS_DECL_ISUPPORTS
  NS_DECL_NSICONTROLLERCOMMANDGROUP

public:
  typedef nsClassHashtable<nsCStringHashKey, nsTArray<nsCString>> GroupsHashtable;

protected:
  virtual ~nsControllerCommandGroup();

  void ClearGroupsHash();

protected:
  GroupsHashtable mGroupsHash; 
                               
};

#endif 

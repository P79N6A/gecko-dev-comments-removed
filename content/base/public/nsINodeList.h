




































#ifndef nsINodeList_h___
#define nsINodeList_h___

#include "nsIDOMNodeList.h"
#include "nsWrapperCache.h"

class nsINode;
class nsIContent;


#define NS_INODELIST_IID \
{ 0xe60b773e, 0x5d20, 0x43f6, \
 { 0xb0, 0x8c, 0xfd, 0x65, 0x26, 0xce, 0xe0, 0x7a } }




class nsINodeList : public nsIDOMNodeList,
                    public nsWrapperCache
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_INODELIST_IID)

  



  virtual PRInt32 IndexOf(nsIContent* aContent) = 0;

  


  virtual nsINode* GetParentObject() = 0;
};

#define NS_NODELIST_OFFSET_AND_INTERFACE_TABLE_BEGIN(_class)                  \
  NS_OFFSET_AND_INTERFACE_TABLE_BEGIN_AMBIGUOUS(_class, nsINodeList)

NS_DEFINE_STATIC_IID_ACCESSOR(nsINodeList, NS_INODELIST_IID)

#endif 

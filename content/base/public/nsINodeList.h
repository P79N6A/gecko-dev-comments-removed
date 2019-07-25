




































#ifndef nsINodeList_h___
#define nsINodeList_h___

#include "nsIDOMNodeList.h"
#include "nsWrapperCache.h"

class nsINode;
class nsIContent;


#define NS_INODELIST_IID \
{ 0xa842c1b5, 0x9a6f, 0x4afa, \
 { 0x9c, 0x1c, 0xf5, 0xf7, 0xdc, 0x70, 0x82, 0xd9 } }





class nsINodeList : public nsIDOMNodeList,
                    public nsWrapperCache
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_INODELIST_IID)

  


  virtual nsIContent* GetNodeAt(PRUint32 aIndex) = 0;

  



  virtual PRInt32 IndexOf(nsIContent* aContent) = 0;

  


  virtual nsINode* GetParentObject() = 0;
};

#define NS_NODELIST_OFFSET_AND_INTERFACE_TABLE_BEGIN(_class)                  \
  NS_OFFSET_AND_INTERFACE_TABLE_BEGIN_AMBIGUOUS(_class, nsINodeList)

NS_DEFINE_STATIC_IID_ACCESSOR(nsINodeList, NS_INODELIST_IID)

#endif 

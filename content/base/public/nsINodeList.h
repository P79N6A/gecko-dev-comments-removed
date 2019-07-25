




































#ifndef nsINodeList_h___
#define nsINodeList_h___

#include "nsIDOMNodeList.h"
#include "nsWrapperCache.h"

class nsINode;
class nsIContent;


#define NS_INODELIST_IID \
{ 0xe683725e, 0xe75a, 0x4d62, \
 { 0x88, 0x5e, 0x5d, 0xd3, 0x1c, 0x27, 0x4b, 0xcc } }





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

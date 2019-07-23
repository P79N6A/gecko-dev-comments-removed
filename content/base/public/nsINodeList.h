




































#ifndef nsINodeList_h___
#define nsINodeList_h___

#include "nsIDOMNodeList.h"

class nsIContent;


#define NS_INODELIST_IID \
{ 0x57ac9ea2, 0xe95f, 0x4856, \
 { 0xbb, 0xac, 0x82, 0x2d, 0x65, 0xb1, 0x92, 0x57 } }





class nsINodeList : public nsIDOMNodeList
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_INODELIST_IID)

  


  virtual nsIContent* GetNodeAt(PRUint32 aIndex) = 0;

  



  virtual PRInt32 IndexOf(nsIContent* aContent) = 0;
};

#define NS_NODELIST_OFFSET_AND_INTERFACE_TABLE_BEGIN(_class)                  \
  NS_OFFSET_AND_INTERFACE_TABLE_BEGIN_AMBIGUOUS(_class, nsINodeList)

NS_DEFINE_STATIC_IID_ACCESSOR(nsINodeList, NS_INODELIST_IID)

#endif 

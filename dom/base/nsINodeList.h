




#ifndef nsINodeList_h___
#define nsINodeList_h___

#include "nsIDOMNodeList.h"
#include "nsWrapperCache.h"


#define NS_INODELIST_IID \
{ 0xadb5e54c, 0x6e96, 0x4102, \
 { 0x8d, 0x40, 0xe0, 0x12, 0x3d, 0xcf, 0x48, 0x7a } }

class nsIContent;
class nsINode;




class nsINodeList : public nsIDOMNodeList,
                    public nsWrapperCache
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_INODELIST_IID)

  



  virtual int32_t IndexOf(nsIContent* aContent) = 0;

  


  virtual nsINode* GetParentObject() = 0;

  using nsIDOMNodeList::Item;

  uint32_t Length()
  {
    uint32_t length;
    GetLength(&length);
    return length;
  }
  virtual nsIContent* Item(uint32_t aIndex) = 0;
  nsIContent* IndexedGetter(uint32_t aIndex, bool& aFound)
  {
    nsIContent* item = Item(aIndex);
    aFound = !!item;
    return item;
  }
};

#define NS_NODELIST_OFFSET_AND_INTERFACE_TABLE_BEGIN(_class)                  \
  NS_OFFSET_AND_INTERFACE_TABLE_BEGIN_AMBIGUOUS(_class, nsINodeList)

NS_DEFINE_STATIC_IID_ACCESSOR(nsINodeList, NS_INODELIST_IID)

#endif 

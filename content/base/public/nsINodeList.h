




































#ifndef nsINodeList_h___
#define nsINodeList_h___

class nsINode;
class nsIDOMNodeList;


#define NS_INODELIST_IID \
{ 0x943420c4, 0x8774, 0x43ea, \
 { 0xb3, 0x53, 0x62, 0xa1, 0x26, 0x1c, 0x9b, 0x55 } }




class nsINodeList : public nsIDOMNodeList
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_INODELIST_IID)

  


  virtual nsINode* GetNodeAt(PRUint32 aIndex) = 0;
};

#define NS_NODELIST_OFFSET_AND_INTERFACE_TABLE_BEGIN(_class)                  \
  NS_OFFSET_AND_INTERFACE_TABLE_BEGIN_AMBIGUOUS(_class, nsINodeList)

NS_DEFINE_STATIC_IID_ACCESSOR(nsINodeList, NS_INODELIST_IID)

#endif 

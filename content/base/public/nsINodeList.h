




































#ifndef nsINodeList_h___
#define nsINodeList_h___

class nsINode;


#define NS_INODELIST_IID \
{ 0x06a6639a, 0x2d47, 0x4551, \
 { 0x94, 0xef, 0x93, 0xb8, 0xe1, 0x09, 0x3a, 0xb3 } }





class nsINodeList : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_INODELIST_IID)

  


  virtual nsINode* GetNodeAt(PRUint32 aIndex) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsINodeList, NS_INODELIST_IID)

#endif 

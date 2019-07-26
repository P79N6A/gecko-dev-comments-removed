




#ifndef nsIAttribute_h___
#define nsIAttribute_h___

#include "nsINode.h"

class nsDOMAttributeMap;
class nsIContent;

#define NS_IATTRIBUTE_IID  \
{ 0x8d9d7dbf, 0xc42d, 0x4715, \
  { 0x95, 0xcf, 0x7a, 0x5e, 0xd5, 0xa4, 0x47, 0x70 } }

class nsIAttribute : public nsINode
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IATTRIBUTE_IID)

  virtual void SetMap(nsDOMAttributeMap *aMap) = 0;
  
  nsDOMAttributeMap *GetMap()
  {
    return mAttrMap;
  }

  nsINodeInfo *NodeInfo() const
  {
    return mNodeInfo;
  }

  virtual nsIContent* GetContent() const = 0;

  



  virtual nsresult SetOwnerDocument(nsIDocument* aDocument) = 0;

protected:
#ifdef MOZILLA_INTERNAL_API
  nsIAttribute(nsDOMAttributeMap *aAttrMap, already_AddRefed<nsINodeInfo> aNodeInfo,
               bool aNsAware);
#endif 

  nsRefPtr<nsDOMAttributeMap> mAttrMap;
  bool mNsAware;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIAttribute, NS_IATTRIBUTE_IID)

#endif 

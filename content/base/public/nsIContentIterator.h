




































#ifndef __nsIContentIterator_h___
#define __nsIContentIterator_h___

#include "nsISupports.h"

class nsINode;
class nsIDOMRange;
class nsIRange;
class nsRange;

#define NS_ICONTENTITERATOR_IID \
{ 0x2550078e, 0xae87, 0x4914, \
 { 0xb3, 0x04, 0xe4, 0xd1, 0x46, 0x19, 0x3d, 0x5f } }

class nsIContentIterator : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICONTENTITERATOR_IID)

  

  virtual nsresult Init(nsINode* aRoot) = 0;

  


  virtual nsresult Init(nsIDOMRange* aRange) = 0;
  virtual nsresult Init(nsIRange* aRange) = 0;

  

  virtual void First() = 0;

  

  virtual void Last() = 0;

  

  virtual void Next() = 0;

  

  virtual void Prev() = 0;

  


  virtual nsINode *GetCurrentNode() = 0;

  



  virtual PRBool IsDone() = 0;

  

  virtual nsresult PositionAt(nsINode* aCurNode) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIContentIterator, NS_ICONTENTITERATOR_IID)

#endif 


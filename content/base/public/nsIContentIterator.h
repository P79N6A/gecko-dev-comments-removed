




































#ifndef __nsIContentIterator_h___
#define __nsIContentIterator_h___

#include "nsISupports.h"

class nsINode;
class nsIDOMRange;

#define NS_ICONTENTITERATOR_IID \
{ 0x716a396c, 0xdc4e, 0x4d10, \
  { 0xbd, 0x07, 0x27, 0xee, 0xae, 0x85, 0xe3, 0x86 } }

class nsIContentIterator : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICONTENTITERATOR_IID)

  

  virtual nsresult Init(nsINode* aRoot) = 0;

  

  virtual nsresult Init(nsIDOMRange* aRange) = 0;

  

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


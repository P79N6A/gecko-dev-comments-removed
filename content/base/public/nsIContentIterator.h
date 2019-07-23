




































#ifndef __nsIContentIterator_h___
#define __nsIContentIterator_h___

#include "nsISupports.h"

class nsIContent;
class nsIDOMRange;

#define NS_ICONTENTITERTOR_IID \
{0xa6cf90e4, 0x15b3, 0x11d2,   \
{0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32} }

class nsIContentIterator : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICONTENTITERTOR_IID)

  

  virtual nsresult Init(nsIContent* aRoot) = 0;

  

  virtual nsresult Init(nsIDOMRange* aRange) = 0;

  

  virtual void First() = 0;

  

  virtual void Last() = 0;

  

  virtual void Next() = 0;

  

  virtual void Prev() = 0;

  


  virtual nsIContent *GetCurrentNode() = 0;

  



  virtual PRBool IsDone() = 0;

  

  virtual nsresult PositionAt(nsIContent* aCurNode) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIContentIterator, NS_ICONTENTITERTOR_IID)

#endif 


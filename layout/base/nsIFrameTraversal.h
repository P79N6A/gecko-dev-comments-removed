



































#ifndef NSIFRAMETRAVERSAL_H
#define NSIFRAMETRAVERSAL_H

#include "nsISupports.h"
#include "nsIFrame.h"

#define NS_IFRAMEENUMERATOR_IID \
{ 0x7c633f5d, 0x91eb, 0x494e, \
  { 0xa1, 0x40, 0x17, 0x46, 0x17, 0x4c, 0x23, 0xd3 } }

class nsIFrameEnumerator : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFRAMEENUMERATOR_IID)

  virtual void First() = 0;
  virtual void Next() = 0;
  virtual nsIFrame* CurrentItem() = 0;
  virtual bool IsDone() = 0;

  virtual void Last() = 0;
  virtual void Prev() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFrameEnumerator, NS_IFRAMEENUMERATOR_IID)

enum nsIteratorType {
  eLeaf,
  ePreOrder,
  ePostOrder
};


#define NS_IFRAMETRAVERSAL_IID \
{ 0x9d469828, 0x9bf2, 0x4151, { 0xa3, 0x85, 0x05, 0xf3, 0x02, 0x19, 0x22, 0x1b } }

class nsIFrameTraversal : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFRAMETRAVERSAL_IID)

  














  NS_IMETHOD NewFrameTraversal(nsIFrameEnumerator **aEnumerator,
                               nsPresContext* aPresContext,
                               nsIFrame *aStart,
                               PRInt32 aType,
                               bool aVisual,
                               bool aLockInScrollView,
                               bool aFollowOOFs) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFrameTraversal, NS_IFRAMETRAVERSAL_IID)

#endif 

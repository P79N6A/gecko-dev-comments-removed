




































#ifndef nsPIListBoxObject_h__
#define nsPIListBoxObject_h__

class nsListBoxBodyFrame;


#define NS_PILISTBOXOBJECT_IID \
{ 0xfa9549f7, 0xee09, 0x48fc, \
  { 0x89, 0xf7, 0x30, 0xcc, 0xee, 0xe2, 0x1c, 0x15 } }

#include "nsIListBoxObject.h"

class nsPIListBoxObject : public nsIListBoxObject {
 public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_PILISTBOXOBJECT_IID)
  



  virtual nsListBoxBodyFrame* GetListBoxBody(bool aFlush) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsPIListBoxObject, NS_PILISTBOXOBJECT_IID)

#endif 

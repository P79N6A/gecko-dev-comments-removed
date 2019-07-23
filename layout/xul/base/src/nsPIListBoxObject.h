




































#ifndef nsPIListBoxObject_h__
#define nsPIListBoxObject_h__

#define NS_PILISTBOXOBJECT_IID \
{ 0x965f3d0b, 0x2960, 0x40f5, \
  { 0xaa, 0xab, 0x32, 0xd2, 0xae, 0x09, 0x90, 0x94 } }

#include "nsIListBoxObject.h"

class nsPIListBoxObject : public nsIListBoxObject {
 public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_PILISTBOXOBJECT_IID)
  


  virtual nsIListBoxObject* GetListBoxBody() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsPIListBoxObject, NS_PILISTBOXOBJECT_IID)

#endif 

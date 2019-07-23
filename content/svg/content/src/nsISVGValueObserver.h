






































#ifndef __NS_ISVGVALUEOBSERVER_H__
#define __NS_ISVGVALUEOBSERVER_H__

#include "nsWeakReference.h"
#include "nsISVGValue.h"





#define NS_ISVGVALUEOBSERVER_IID \
  { 0x485029a4, 0x2449, 0x45c1, \
    { 0x98, 0x14, 0x08, 0xf3, 0x81, 0x32, 0xca, 0x4c } }

class nsISVGValueObserver : public nsSupportsWeakReference
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISVGVALUEOBSERVER_IID)
  
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable,
                                     nsISVGValue::modificationType)=0;
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable,
                                     nsISVGValue::modificationType)=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsISVGValueObserver, NS_ISVGVALUEOBSERVER_IID)

#endif 


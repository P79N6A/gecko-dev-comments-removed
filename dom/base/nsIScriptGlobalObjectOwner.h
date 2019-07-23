





































#ifndef nsIScriptGlobalObjectOwner_h__
#define nsIScriptGlobalObjectOwner_h__

#include "nsISupports.h"

class nsIScriptGlobalObject;

#define NS_ISCRIPTGLOBALOBJECTOWNER_IID \
  {0xfd25ca8e, 0x6b63, 0x435f, \
    { 0xb8, 0xc6, 0xb8, 0x07, 0x68, 0xa4, 0x0a, 0xdc }}






class nsIScriptGlobalObjectOwner : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTGLOBALOBJECTOWNER_IID)

  


  virtual nsIScriptGlobalObject* GetScriptGlobalObject() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptGlobalObjectOwner,
                              NS_ISCRIPTGLOBALOBJECTOWNER_IID)

#endif 

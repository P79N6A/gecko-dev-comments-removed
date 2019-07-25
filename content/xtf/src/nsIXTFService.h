





































#ifndef __NS_IXTFSERVICE_H__
#define __NS_IXTFSERVICE_H__

#include "nsISupports.h"

class nsIContent;
class nsINodeInfo;


#define NS_IXTFSERVICE_IID                             \
  { 0x4ac3826f, 0x280e, 0x4572, \
    { 0x9e, 0xde, 0x6c, 0x81, 0xa4, 0x79, 0x78, 0x61 } }
class nsIXTFService : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IXTFSERVICE_IID)

    
    virtual nsresult CreateElement(nsIContent** aResult,
                                   already_AddRefed<nsINodeInfo> aNodeInfo) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIXTFService, NS_IXTFSERVICE_IID)





#define NS_XTFSERVICE_CID                             \
  { 0x4ec832da, 0x6ae7, 0x4185, { 0x80, 0x7b, 0xda, 0xdd, 0xcb, 0x5d, 0xa3, 0x7a } }

#define NS_XTFSERVICE_CONTRACTID "@mozilla.org/xtf/xtf-service;1"

nsresult NS_NewXTFService(nsIXTFService** aResult);

#endif 


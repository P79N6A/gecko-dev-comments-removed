





































#ifndef __NS_IXTFSERVICE_H__
#define __NS_IXTFSERVICE_H__

#include "nsISupports.h"

class nsIContent;
class nsINodeInfo;


#define NS_IXTFSERVICE_IID                             \
  { 0x02ad2add, 0xc5ec, 0x4362, { 0xbb, 0x5f, 0xe2, 0xc6, 0x9b, 0xa7, 0x61, 0x51 } }

class nsIXTFService : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IXTFSERVICE_IID)

    
    virtual nsresult CreateElement(nsIContent** aResult,
                                   nsINodeInfo* aNodeInfo)=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIXTFService, NS_IXTFSERVICE_IID)





#define NS_XTFSERVICE_CID                             \
  { 0x4ec832da, 0x6ae7, 0x4185, { 0x80, 0x7b, 0xda, 0xdd, 0xcb, 0x5d, 0xa3, 0x7a } }

#define NS_XTFSERVICE_CONTRACTID "@mozilla.org/xtf/xtf-service;1"

nsresult NS_NewXTFService(nsIXTFService** aResult);

#endif 


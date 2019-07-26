




#ifndef nsIParserService_h__
#define nsIParserService_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsHTMLTags.h"

class nsIParser;

#define NS_PARSERSERVICE_CONTRACTID "@mozilla.org/parser/parser-service;1"


#define NS_IPARSERSERVICE_IID \
{ 0x90a92e37, 0xabd6, 0x441b, { 0x9b, 0x39, 0x40, 0x64, 0xd9, 0x8e, 0x1e, 0xde } }

class nsIParserService : public nsISupports {
 public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPARSERSERVICE_IID)

  









  virtual int32_t HTMLAtomTagToId(nsIAtom* aAtom) const = 0;

  








  virtual int32_t HTMLCaseSensitiveAtomTagToId(nsIAtom* aAtom) const = 0;

  









  virtual int32_t HTMLStringTagToId(const nsAString& aTag) const = 0;

  










  virtual const char16_t *HTMLIdToStringTag(int32_t aId) const = 0;

  










  virtual nsIAtom *HTMLIdToAtomTag(int32_t aId) const = 0;
  
  NS_IMETHOD HTMLConvertEntityToUnicode(const nsAString& aEntity, 
                                        int32_t* aUnicode) const = 0;

  NS_IMETHOD HTMLConvertUnicodeToEntity(int32_t aUnicode,
                                        nsCString& aEntity) const = 0;

  NS_IMETHOD IsContainer(int32_t aId, bool& aIsContainer) const = 0;
  NS_IMETHOD IsBlock(int32_t aId, bool& aIsBlock) const = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIParserService, NS_IPARSERSERVICE_IID)

#endif 

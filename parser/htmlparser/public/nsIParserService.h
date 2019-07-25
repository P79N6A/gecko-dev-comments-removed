




































#ifndef nsIParserService_h__
#define nsIParserService_h__

#include "nsISupports.h"
#include "nsStringGlue.h"
#include "nsHTMLTags.h"
#include "nsIElementObserver.h"

class nsIParser;
class nsIParserNode;

#define NS_PARSERSERVICE_CONTRACTID "@mozilla.org/parser/parser-service;1"


#define NS_IPARSERSERVICE_IID \
{ 0x90a92e37, 0xabd6, 0x441b, { 0x9b, 0x39, 0x40, 0x64, 0xd9, 0x8e, 0x1e, 0xde } }

class nsIParserService : public nsISupports {
 public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPARSERSERVICE_IID)

  









  virtual PRInt32 HTMLAtomTagToId(nsIAtom* aAtom) const = 0;

  








  virtual PRInt32 HTMLCaseSensitiveAtomTagToId(nsIAtom* aAtom) const = 0;

  









  virtual PRInt32 HTMLStringTagToId(const nsAString& aTag) const = 0;

  










  virtual const PRUnichar *HTMLIdToStringTag(PRInt32 aId) const = 0;

  










  virtual nsIAtom *HTMLIdToAtomTag(PRInt32 aId) const = 0;
  
  NS_IMETHOD HTMLConvertEntityToUnicode(const nsAString& aEntity, 
                                        PRInt32* aUnicode) const = 0;

  NS_IMETHOD HTMLConvertUnicodeToEntity(PRInt32 aUnicode,
                                        nsCString& aEntity) const = 0;

  NS_IMETHOD IsContainer(PRInt32 aId, bool& aIsContainer) const = 0;
  NS_IMETHOD IsBlock(PRInt32 aId, bool& aIsBlock) const = 0;

  virtual nsresult CheckQName(const nsAString& aQName,
                              bool aNamespaceAware,
                              const PRUnichar** aColon) = 0;
  virtual bool IsXMLLetter(PRUnichar aChar) = 0;
  virtual bool IsXMLNCNameChar(PRUnichar aChar) = 0;

  














  virtual PRUint32 DecodeEntity(const PRUnichar* aStart, const PRUnichar* aEnd,
                                const PRUnichar** aNext,
                                PRUnichar* aResult) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIParserService, NS_IPARSERSERVICE_IID)

#endif 

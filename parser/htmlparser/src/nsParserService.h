




































#ifndef NS_PARSERSERVICE_H__
#define NS_PARSERSERVICE_H__

#include "nsIParserService.h"
#include "nsDTDUtils.h"

extern "C" int MOZ_XMLIsLetter(const char* ptr);
extern "C" int MOZ_XMLIsNCNameChar(const char* ptr);
extern "C" int MOZ_XMLTranslateEntity(const char* ptr, const char* end,
                                      const char** next, PRUnichar* result);

class nsParserService : public nsIParserService {
public:
  nsParserService();
  virtual ~nsParserService();

  NS_DECL_ISUPPORTS

  PRInt32 HTMLAtomTagToId(nsIAtom* aAtom) const;

  PRInt32 HTMLCaseSensitiveAtomTagToId(nsIAtom* aAtom) const;

  PRInt32 HTMLStringTagToId(const nsAString& aTag) const;

  const PRUnichar *HTMLIdToStringTag(PRInt32 aId) const;
  
  nsIAtom *HTMLIdToAtomTag(PRInt32 aId) const;

  NS_IMETHOD HTMLConvertEntityToUnicode(const nsAString& aEntity, 
                                        PRInt32* aUnicode) const;
  NS_IMETHOD HTMLConvertUnicodeToEntity(PRInt32 aUnicode,
                                        nsCString& aEntity) const;
  NS_IMETHOD IsContainer(PRInt32 aId, bool& aIsContainer) const;
  NS_IMETHOD IsBlock(PRInt32 aId, bool& aIsBlock) const;

  nsresult CheckQName(const nsAString& aQName,
                      bool aNamespaceAware, const PRUnichar** aColon);
};

#endif

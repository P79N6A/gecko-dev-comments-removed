




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

  int32_t HTMLAtomTagToId(nsIAtom* aAtom) const;

  int32_t HTMLCaseSensitiveAtomTagToId(nsIAtom* aAtom) const;

  int32_t HTMLStringTagToId(const nsAString& aTag) const;

  const PRUnichar *HTMLIdToStringTag(int32_t aId) const;
  
  nsIAtom *HTMLIdToAtomTag(int32_t aId) const;

  NS_IMETHOD HTMLConvertEntityToUnicode(const nsAString& aEntity, 
                                        int32_t* aUnicode) const;
  NS_IMETHOD HTMLConvertUnicodeToEntity(int32_t aUnicode,
                                        nsCString& aEntity) const;
  NS_IMETHOD IsContainer(int32_t aId, bool& aIsContainer) const;
  NS_IMETHOD IsBlock(int32_t aId, bool& aIsBlock) const;
};

#endif

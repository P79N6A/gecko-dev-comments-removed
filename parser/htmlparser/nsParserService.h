




#ifndef NS_PARSERSERVICE_H__
#define NS_PARSERSERVICE_H__

#include "nsIParserService.h"

extern "C" int MOZ_XMLIsLetter(const char* ptr);
extern "C" int MOZ_XMLIsNCNameChar(const char* ptr);
extern "C" int MOZ_XMLTranslateEntity(const char* ptr, const char* end,
                                      const char** next, char16_t* result);

class nsParserService : public nsIParserService {
  virtual ~nsParserService();

public:
  nsParserService();

  NS_DECL_ISUPPORTS

  int32_t HTMLAtomTagToId(nsIAtom* aAtom) const override;

  int32_t HTMLCaseSensitiveAtomTagToId(nsIAtom* aAtom) const override;

  int32_t HTMLStringTagToId(const nsAString& aTag) const override;

  const char16_t *HTMLIdToStringTag(int32_t aId) const override;
  
  nsIAtom *HTMLIdToAtomTag(int32_t aId) const override;

  NS_IMETHOD HTMLConvertEntityToUnicode(const nsAString& aEntity, 
                                        int32_t* aUnicode) const override;
  NS_IMETHOD HTMLConvertUnicodeToEntity(int32_t aUnicode,
                                        nsCString& aEntity) const override;
  NS_IMETHOD IsContainer(int32_t aId, bool& aIsContainer) const override;
  NS_IMETHOD IsBlock(int32_t aId, bool& aIsBlock) const override;
};

#endif

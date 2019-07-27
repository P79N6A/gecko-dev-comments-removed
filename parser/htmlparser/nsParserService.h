




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

  int32_t HTMLAtomTagToId(nsIAtom* aAtom) const MOZ_OVERRIDE;

  int32_t HTMLCaseSensitiveAtomTagToId(nsIAtom* aAtom) const MOZ_OVERRIDE;

  int32_t HTMLStringTagToId(const nsAString& aTag) const MOZ_OVERRIDE;

  const char16_t *HTMLIdToStringTag(int32_t aId) const MOZ_OVERRIDE;
  
  nsIAtom *HTMLIdToAtomTag(int32_t aId) const MOZ_OVERRIDE;

  NS_IMETHOD HTMLConvertEntityToUnicode(const nsAString& aEntity, 
                                        int32_t* aUnicode) const MOZ_OVERRIDE;
  NS_IMETHOD HTMLConvertUnicodeToEntity(int32_t aUnicode,
                                        nsCString& aEntity) const MOZ_OVERRIDE;
  NS_IMETHOD IsContainer(int32_t aId, bool& aIsContainer) const MOZ_OVERRIDE;
  NS_IMETHOD IsBlock(int32_t aId, bool& aIsBlock) const MOZ_OVERRIDE;
};

#endif

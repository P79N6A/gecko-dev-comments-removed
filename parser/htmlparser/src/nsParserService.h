




































#ifndef NS_PARSERSERVICE_H__
#define NS_PARSERSERVICE_H__

#include "nsIParserService.h"
#include "nsDTDUtils.h"
#include "nsVoidArray.h"

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
  NS_IMETHOD IsContainer(PRInt32 aId, PRBool& aIsContainer) const;
  NS_IMETHOD IsBlock(PRInt32 aId, PRBool& aIsBlock) const;

   
  NS_IMETHOD RegisterObserver(nsIElementObserver* aObserver,
                              const nsAString& aTopic,
                              const eHTMLTags* aTags = nsnull);

  NS_IMETHOD UnregisterObserver(nsIElementObserver* aObserver,
                                const nsAString& aTopic);
  NS_IMETHOD GetTopicObservers(const nsAString& aTopic,
                               nsIObserverEntry** aEntry);

  nsresult CheckQName(const nsAString& aQName,
                      PRBool aNamespaceAware, const PRUnichar** aColon);

  PRBool IsXMLLetter(PRUnichar aChar)
  {
    return MOZ_XMLIsLetter(reinterpret_cast<const char*>(&aChar));
  }
  PRBool IsXMLNCNameChar(PRUnichar aChar)
  {
    return MOZ_XMLIsNCNameChar(reinterpret_cast<const char*>(&aChar));
  }
  PRUint32 DecodeEntity(const PRUnichar* aStart, const PRUnichar* aEnd,
                        const PRUnichar** aNext, PRUnichar* aResult)
  {
    *aNext = nsnull;
    return MOZ_XMLTranslateEntity(reinterpret_cast<const char*>(aStart),
                                  reinterpret_cast<const char*>(aEnd),
                                  reinterpret_cast<const char**>(aNext),
                                  aResult);
  }

protected:
  nsObserverEntry* GetEntry(const nsAString& aTopic);
  nsresult CreateEntry(const nsAString& aTopic,
                       nsObserverEntry** aEntry);

  nsDeque  mEntries;  
  PRBool   mHaveNotifiedCategoryObservers;
};

#endif

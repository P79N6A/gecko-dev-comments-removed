





































#include "nsDOMError.h"
#include "nsIAtom.h"
#include "nsParserService.h"
#include "nsHTMLEntities.h"
#include "nsElementTable.h"
#include "nsICategoryManager.h"
#include "nsCategoryManagerUtils.h"

extern "C" int MOZ_XMLCheckQName(const char* ptr, const char* end,
                                 int ns_aware, const char** colon);

nsParserService::nsParserService() : mEntries(0)
{
  mHaveNotifiedCategoryObservers = PR_FALSE;
}

nsParserService::~nsParserService()
{
  nsObserverEntry *entry = nsnull;
  while( (entry = static_cast<nsObserverEntry*>(mEntries.Pop())) ) {
    NS_RELEASE(entry);
  }
}

NS_IMPL_ISUPPORTS1(nsParserService, nsIParserService)

PRInt32
nsParserService::HTMLAtomTagToId(nsIAtom* aAtom) const
{
  nsAutoString tagName;
  aAtom->ToString(tagName);

  return nsHTMLTags::LookupTag(tagName);
}

PRInt32
nsParserService::HTMLCaseSensitiveAtomTagToId(nsIAtom* aAtom) const
{
  return nsHTMLTags::CaseSensitiveLookupTag(aAtom);
}

PRInt32
nsParserService::HTMLStringTagToId(const nsAString& aTag) const
{
  return nsHTMLTags::LookupTag(aTag);
}

const PRUnichar*
nsParserService::HTMLIdToStringTag(PRInt32 aId) const
{
  return nsHTMLTags::GetStringValue((nsHTMLTag)aId);
}
  
nsIAtom*
nsParserService::HTMLIdToAtomTag(PRInt32 aId) const
{
  return nsHTMLTags::GetAtom((nsHTMLTag)aId);
}

NS_IMETHODIMP
nsParserService::HTMLConvertEntityToUnicode(const nsAString& aEntity,
                                            PRInt32* aUnicode) const
{
  *aUnicode = nsHTMLEntities::EntityToUnicode(aEntity);

  return NS_OK;
}

NS_IMETHODIMP
nsParserService::HTMLConvertUnicodeToEntity(PRInt32 aUnicode,
                                            nsCString& aEntity) const
{
  const char* str = nsHTMLEntities::UnicodeToEntity(aUnicode);
  if (str) {
    aEntity.Assign(str);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsParserService::IsContainer(PRInt32 aId, PRBool& aIsContainer) const
{
  aIsContainer = nsHTMLElement::IsContainer((eHTMLTags)aId);

  return NS_OK;
}

NS_IMETHODIMP
nsParserService::IsBlock(PRInt32 aId, PRBool& aIsBlock) const
{
  if((aId>eHTMLTag_unknown) && (aId<eHTMLTag_userdefined)) {
    aIsBlock=((gHTMLElements[aId].IsMemberOf(kBlock))       ||
              (gHTMLElements[aId].IsMemberOf(kBlockEntity)) ||
              (gHTMLElements[aId].IsMemberOf(kHeading))     ||
              (gHTMLElements[aId].IsMemberOf(kPreformatted))||
              (gHTMLElements[aId].IsMemberOf(kList)));
  }
  else {
    aIsBlock = PR_FALSE;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsParserService::RegisterObserver(nsIElementObserver* aObserver,
                                  const nsAString& aTopic,
                                  const eHTMLTags* aTags)
{
  nsresult result = NS_OK;
  nsObserverEntry* entry = GetEntry(aTopic);

  if(!entry) {
    result = CreateEntry(aTopic,&entry);
    NS_ENSURE_SUCCESS(result,result);
  }

  while (*aTags) {
    if (*aTags <= NS_HTML_TAG_MAX) {
      entry->AddObserver(aObserver,*aTags);
    }
    ++aTags;
  }

  return result;
}

NS_IMETHODIMP
nsParserService::UnregisterObserver(nsIElementObserver* aObserver,
                                    const nsAString& aTopic)
{
  PRInt32 count = mEntries.GetSize();

  for (PRInt32 i=0; i < count; ++i) {
    nsObserverEntry* entry = static_cast<nsObserverEntry*>(mEntries.ObjectAt(i));
    if (entry && entry->Matches(aTopic)) {
      entry->RemoveObserver(aObserver);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsParserService::GetTopicObservers(const nsAString& aTopic,
                                   nsIObserverEntry** aEntry) {
  nsresult result = NS_OK;
  nsObserverEntry* entry = GetEntry(aTopic);

  if (!entry) {
    return NS_ERROR_NULL_POINTER;
  }

  NS_ADDREF(*aEntry = entry);

  return result;
}

nsresult
nsParserService::CheckQName(const nsAString& aQName,
                            PRBool aNamespaceAware,
                            const PRUnichar** aColon)
{
  const char* colon;
  const PRUnichar *begin, *end;
  begin = aQName.BeginReading();
  end = aQName.EndReading();
  int result = MOZ_XMLCheckQName(reinterpret_cast<const char*>(begin),
                                 reinterpret_cast<const char*>(end),
                                 aNamespaceAware, &colon);
  *aColon = reinterpret_cast<const PRUnichar*>(colon);

  if (result == 0) {
    return NS_OK;
  }

  
  if (result & (1 << 0) || result & (1 << 1)) {
    return NS_ERROR_DOM_INVALID_CHARACTER_ERR;
  }

  return NS_ERROR_DOM_NAMESPACE_ERR;
}

class nsMatchesTopic : public nsDequeFunctor{
  const nsAString& mString;
public:
  PRBool matched;
  nsObserverEntry* entry;
  nsMatchesTopic(const nsAString& aString):mString(aString),matched(PR_FALSE){}
  virtual void* operator()(void* anObject){
    entry=static_cast<nsObserverEntry*>(anObject);
    matched=mString.Equals(entry->mTopic);
    return matched ? nsnull : anObject;
  }
};


nsObserverEntry*
nsParserService::GetEntry(const nsAString& aTopic)
{
  if (!mHaveNotifiedCategoryObservers) {
    mHaveNotifiedCategoryObservers = PR_TRUE;
    NS_CreateServicesFromCategory("parser-service-category",
                                  static_cast<nsISupports*>(static_cast<void*>(this)),
                                  "parser-service-start"); 
  }

  nsMatchesTopic matchesTopic(aTopic);
  mEntries.FirstThat(*&matchesTopic);
  return matchesTopic.matched?matchesTopic.entry:nsnull;
}

nsresult
nsParserService::CreateEntry(const nsAString& aTopic, nsObserverEntry** aEntry)
{
  *aEntry = new nsObserverEntry(aTopic);

  if (!*aEntry) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aEntry);
  mEntries.Push(*aEntry);

  return NS_OK;
}

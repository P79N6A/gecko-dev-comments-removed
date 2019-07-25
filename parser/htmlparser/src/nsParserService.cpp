





































#include "nsDOMError.h"
#include "nsIAtom.h"
#include "nsParserService.h"
#include "nsHTMLEntities.h"
#include "nsElementTable.h"
#include "nsICategoryManager.h"
#include "nsCategoryManagerUtils.h"

extern "C" int MOZ_XMLCheckQName(const char* ptr, const char* end,
                                 int ns_aware, const char** colon);

nsParserService::nsParserService()
{
}

nsParserService::~nsParserService()
{
}

NS_IMPL_ISUPPORTS1(nsParserService, nsIParserService)

PRInt32
nsParserService::HTMLAtomTagToId(nsIAtom* aAtom) const
{
  return nsHTMLTags::LookupTag(nsDependentAtomString(aAtom));
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
nsParserService::IsContainer(PRInt32 aId, bool& aIsContainer) const
{
  aIsContainer = nsHTMLElement::IsContainer((eHTMLTags)aId);

  return NS_OK;
}

NS_IMETHODIMP
nsParserService::IsBlock(PRInt32 aId, bool& aIsBlock) const
{
  if((aId>eHTMLTag_unknown) && (aId<eHTMLTag_userdefined)) {
    aIsBlock=((gHTMLElements[aId].IsMemberOf(kBlock))       ||
              (gHTMLElements[aId].IsMemberOf(kBlockEntity)) ||
              (gHTMLElements[aId].IsMemberOf(kHeading))     ||
              (gHTMLElements[aId].IsMemberOf(kPreformatted))||
              (gHTMLElements[aId].IsMemberOf(kList)));
  }
  else {
    aIsBlock = false;
  }

  return NS_OK;
}

nsresult
nsParserService::CheckQName(const nsAString& aQName,
                            bool aNamespaceAware,
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

  
  if (result == (1 << 0) || result == (1 << 1)) {
    return NS_ERROR_DOM_INVALID_CHARACTER_ERR;
  }

  return NS_ERROR_DOM_NAMESPACE_ERR;
}











































#include "nsXMLNameSpaceMap.h"
#include "nsIAtom.h"
#include "nsCOMPtr.h"
#include "nsINameSpaceManager.h"
#include "nsContentUtils.h"
#include "nsGkAtoms.h"

struct nsNameSpaceEntry
{
  nsNameSpaceEntry(nsIAtom *aPrefix)
    : prefix(aPrefix) {}

  nsCOMPtr<nsIAtom> prefix;
  PRInt32 nameSpaceID;
};

 nsXMLNameSpaceMap*
nsXMLNameSpaceMap::Create()
{
  nsXMLNameSpaceMap *map = new nsXMLNameSpaceMap();
  NS_ENSURE_TRUE(map, nsnull);

  nsresult rv = map->AddPrefix(nsGkAtoms::xmlns,
                               kNameSpaceID_XMLNS);
  rv |= map->AddPrefix(nsGkAtoms::xml, kNameSpaceID_XML);

  if (NS_FAILED(rv)) {
    delete map;
    map = nsnull;
  }

  return map;
}

nsXMLNameSpaceMap::nsXMLNameSpaceMap()
  : mNameSpaces(4)
{
}

nsresult
nsXMLNameSpaceMap::AddPrefix(nsIAtom *aPrefix, PRInt32 aNameSpaceID)
{
  PRInt32 count = mNameSpaces.Count();
  nsNameSpaceEntry *foundEntry = nsnull;

  for (PRInt32 i = 0; i < count; ++i) {
    nsNameSpaceEntry *entry = NS_STATIC_CAST(nsNameSpaceEntry*,
                                             mNameSpaces.FastElementAt(i));

    NS_ASSERTION(entry, "null entry in namespace map!");

    if (entry->prefix == aPrefix) {
      foundEntry = entry;
      break;
    }
  }

  if (!foundEntry) {
    foundEntry = new nsNameSpaceEntry(aPrefix);
    NS_ENSURE_TRUE(foundEntry, NS_ERROR_OUT_OF_MEMORY);

    if (!mNameSpaces.AppendElement(foundEntry)) {
      delete foundEntry;
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  foundEntry->nameSpaceID = aNameSpaceID;
  return NS_OK;
}

nsresult
nsXMLNameSpaceMap::AddPrefix(nsIAtom *aPrefix, nsString &aURI)
{
  PRInt32 id;
  nsresult rv = nsContentUtils::NameSpaceManager()->RegisterNameSpace(aURI,
                                                                      id);

  NS_ENSURE_SUCCESS(rv, rv);

  return AddPrefix(aPrefix, id);
}

void
nsXMLNameSpaceMap::RemovePrefix(nsIAtom *aPrefix)
{
  PRInt32 count = mNameSpaces.Count();

  for (PRInt32 i = 0; i < count; ++i) {
    nsNameSpaceEntry *entry = NS_STATIC_CAST(nsNameSpaceEntry*,
                                             mNameSpaces.FastElementAt(i));

    NS_ASSERTION(entry, "null entry in namespace map!");

    if (entry->prefix == aPrefix) {
      mNameSpaces.RemoveElementAt(i);
      return;
    }
  }
}

PRInt32
nsXMLNameSpaceMap::FindNameSpaceID(nsIAtom *aPrefix) const
{
  PRInt32 count = mNameSpaces.Count();

  for (PRInt32 i = 0; i < count; ++i) {
    nsNameSpaceEntry *entry = NS_STATIC_CAST(nsNameSpaceEntry*,
                                             mNameSpaces.FastElementAt(i));

    NS_ASSERTION(entry, "null entry in namespace map!");

    if (entry->prefix == aPrefix) {
      return entry->nameSpaceID;
    }
  }

  
  

  return aPrefix ? kNameSpaceID_Unknown : kNameSpaceID_None;
}

nsIAtom*
nsXMLNameSpaceMap::FindPrefix(PRInt32 aNameSpaceID) const
{
  PRInt32 count = mNameSpaces.Count();

  for (PRInt32 i = 0; i < count; ++i) {
    nsNameSpaceEntry *entry = NS_STATIC_CAST(nsNameSpaceEntry*,
                                             mNameSpaces.FastElementAt(i));

    NS_ASSERTION(entry, "null entry in namespace map!");

    if (entry->nameSpaceID == aNameSpaceID) {
      return entry->prefix;
    }
  }

  return nsnull;
}

PR_STATIC_CALLBACK(PRBool) DeleteEntry(void *aElement, void *aData)
{
  delete NS_STATIC_CAST(nsNameSpaceEntry*, aElement);
  return PR_TRUE;
}

void
nsXMLNameSpaceMap::Clear()
{
  mNameSpaces.EnumerateForwards(DeleteEntry, nsnull);
}

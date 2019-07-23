









































#include "nsXMLNameSpaceMap.h"
#include "nsINameSpaceManager.h"
#include "nsContentUtils.h"
#include "nsGkAtoms.h"

NS_SPECIALIZE_TEMPLATE
class nsDefaultComparator <nsNameSpaceEntry, nsIAtom*> {
  public:
    PRBool Equals(const nsNameSpaceEntry& aEntry, nsIAtom* const& aPrefix) const {
      return aEntry.prefix == aPrefix;
    }
};

NS_SPECIALIZE_TEMPLATE
class nsDefaultComparator <nsNameSpaceEntry, PRInt32> {
  public:
    PRBool Equals(const nsNameSpaceEntry& aEntry, const PRInt32& aNameSpace) const {
      return aEntry.nameSpaceID == aNameSpace;
    }
};


 nsXMLNameSpaceMap*
nsXMLNameSpaceMap::Create(PRBool aForXML)
{
  nsXMLNameSpaceMap *map = new nsXMLNameSpaceMap();
  NS_ENSURE_TRUE(map, nsnull);

  if (aForXML) {
    nsresult rv = map->AddPrefix(nsGkAtoms::xmlns,
                                 kNameSpaceID_XMLNS);
    rv |= map->AddPrefix(nsGkAtoms::xml, kNameSpaceID_XML);

    if (NS_FAILED(rv)) {
      delete map;
      map = nsnull;
    }
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
  if (!mNameSpaces.Contains(aPrefix) && !mNameSpaces.AppendElement(aPrefix)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mNameSpaces[mNameSpaces.IndexOf(aPrefix)].nameSpaceID = aNameSpaceID;
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
  mNameSpaces.RemoveElement(aPrefix);
}

PRInt32
nsXMLNameSpaceMap::FindNameSpaceID(nsIAtom *aPrefix) const
{
  PRUint32 index = mNameSpaces.IndexOf(aPrefix);
  if (index != mNameSpaces.NoIndex) {
    return mNameSpaces[index].nameSpaceID;
  }

  
  

  return aPrefix ? kNameSpaceID_Unknown : kNameSpaceID_None;
}

nsIAtom*
nsXMLNameSpaceMap::FindPrefix(PRInt32 aNameSpaceID) const
{
  PRUint32 index = mNameSpaces.IndexOf(aNameSpaceID);
  if (index != mNameSpaces.NoIndex) {
    return mNameSpaces[index].prefix;
  }

  return nsnull;
}

void
nsXMLNameSpaceMap::Clear()
{
  mNameSpaces.Clear();
}

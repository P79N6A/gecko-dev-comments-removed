








#include "nsXMLNameSpaceMap.h"
#include "nsINameSpaceManager.h"
#include "nsContentUtils.h"
#include "nsGkAtoms.h"

template <>
class nsDefaultComparator <nsNameSpaceEntry, nsIAtom*> {
  public:
    bool Equals(const nsNameSpaceEntry& aEntry, nsIAtom* const& aPrefix) const {
      return aEntry.prefix == aPrefix;
    }
};

template <>
class nsDefaultComparator <nsNameSpaceEntry, int32_t> {
  public:
    bool Equals(const nsNameSpaceEntry& aEntry, const int32_t& aNameSpace) const {
      return aEntry.nameSpaceID == aNameSpace;
    }
};


 nsXMLNameSpaceMap*
nsXMLNameSpaceMap::Create(bool aForXML)
{
  nsXMLNameSpaceMap *map = new nsXMLNameSpaceMap();
  NS_ENSURE_TRUE(map, nullptr);

  if (aForXML) {
    nsresult rv1 = map->AddPrefix(nsGkAtoms::xmlns,
                                  kNameSpaceID_XMLNS);
    nsresult rv2 = map->AddPrefix(nsGkAtoms::xml, kNameSpaceID_XML);

    if (NS_FAILED(rv1) || NS_FAILED(rv2)) {
      delete map;
      map = nullptr;
    }
  }

  return map;
}

nsXMLNameSpaceMap::nsXMLNameSpaceMap()
  : mNameSpaces(4)
{
}

nsresult
nsXMLNameSpaceMap::AddPrefix(nsIAtom *aPrefix, int32_t aNameSpaceID)
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
  int32_t id;
  nsresult rv = nsContentUtils::NameSpaceManager()->RegisterNameSpace(aURI,
                                                                      id);

  NS_ENSURE_SUCCESS(rv, rv);

  return AddPrefix(aPrefix, id);
}

int32_t
nsXMLNameSpaceMap::FindNameSpaceID(nsIAtom *aPrefix) const
{
  uint32_t index = mNameSpaces.IndexOf(aPrefix);
  if (index != mNameSpaces.NoIndex) {
    return mNameSpaces[index].nameSpaceID;
  }

  
  

  return aPrefix ? kNameSpaceID_Unknown : kNameSpaceID_None;
}

nsIAtom*
nsXMLNameSpaceMap::FindPrefix(int32_t aNameSpaceID) const
{
  uint32_t index = mNameSpaces.IndexOf(aNameSpaceID);
  if (index != mNameSpaces.NoIndex) {
    return mNameSpaces[index].prefix;
  }

  return nullptr;
}

void
nsXMLNameSpaceMap::Clear()
{
  mNameSpaces.Clear();
}

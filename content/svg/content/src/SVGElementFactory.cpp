




#include "SVGElementFactory.h"
#include "nsGkAtoms.h"
#include "nsIContent.h"
#include "mozilla/dom/NodeInfo.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/FromParser.h"

using namespace mozilla;
using namespace mozilla::dom;



static PLHashTable* sTagAtomTable = nullptr;



#define TABLE_VALUE_OFFSET 1

#define SVG_TAG(_tag, _classname) \
nsresult \
NS_NewSVG##_classname##Element(nsIContent** aResult, \
                               already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo); \
\
static inline nsresult \
Create##_classname##Element(nsIContent** aResult, \
                            already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo, \
                            FromParser aFromParser) \
{ \
  return NS_NewSVG##_classname##Element(aResult, mozilla::Move(aNodeInfo)); \
}

#define SVG_FROM_PARSER_TAG(_tag, _classname) \
nsresult \
NS_NewSVG##_classname##Element(nsIContent** aResult, \
                               already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo, \
                               FromParser aFromParser);
#include "SVGTagList.h"
#undef SVG_TAG
#undef SVG_FROM_PARSER_TAG

nsresult
NS_NewSVGElement(Element** aResult,
                 already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo);

typedef nsresult
  (*contentCreatorCallback)(nsIContent** aResult,
                            already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo,
                            FromParser aFromParser);

static const contentCreatorCallback sContentCreatorCallbacks[] = {
#define SVG_TAG(_tag, _classname) Create##_classname##Element,
#define SVG_FROM_PARSER_TAG(_tag, _classname)  NS_NewSVG##_classname##Element,
#include "SVGTagList.h"
#undef SVG_TAG
#undef SVG_FROM_PARSER_TAG
};

enum SVGTag {
#define SVG_TAG(_tag, _classname) eSVGTag_##_tag,
#define SVG_FROM_PARSER_TAG(_tag, _classname) eSVGTag_##_tag,
#include "SVGTagList.h"
#undef SVG_TAG
#undef SVG_FROM_PARSER_TAG
  eSVGTag_Count
};


static PLHashNumber
SVGTagsHashCodeAtom(const void* key)
{
  return NS_PTR_TO_INT32(key) >> 2;
}

void
SVGElementFactory::Init()
{
  sTagAtomTable = PL_NewHashTable(64, SVGTagsHashCodeAtom,
                                  PL_CompareValues, PL_CompareValues,
                                  nullptr, nullptr);

#define SVG_TAG(_tag, _classname) \
  PL_HashTableAdd(sTagAtomTable, nsGkAtoms::_tag,\
                  NS_INT32_TO_PTR(eSVGTag_##_tag + TABLE_VALUE_OFFSET));
#define SVG_FROM_PARSER_TAG(_tag, _classname) \
  PL_HashTableAdd(sTagAtomTable, nsGkAtoms::_tag,\
                  NS_INT32_TO_PTR(eSVGTag_##_tag + TABLE_VALUE_OFFSET));
#include "SVGTagList.h"
#undef SVG_TAG
#undef SVG_FROM_PARSER_TAG
}

void
SVGElementFactory::Shutdown()
{
  if (sTagAtomTable) {
    PL_HashTableDestroy(sTagAtomTable);
    sTagAtomTable = nullptr;
  }
}

bool
SVGElementFactory::Exists(nsIAtom *aTag)
{
  MOZ_ASSERT(sTagAtomTable, "no lookup table, needs SVGElementFactory::Init");
  void* tag = PL_HashTableLookupConst(sTagAtomTable, aTag);
  return tag != nullptr;
}

nsresult
NS_NewSVGElement(Element** aResult, already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo,
                 FromParser aFromParser)
{
  NS_ASSERTION(sTagAtomTable, "no lookup table, needs SVGElementFactory::Init");

  nsRefPtr<mozilla::dom::NodeInfo> ni = aNodeInfo;
  nsIAtom* name = ni->NameAtom();

  NS_ASSERTION(ni->NamespaceEquals(kNameSpaceID_SVG),
               "Trying to create SVG elements that aren't in the SVG namespace");

  void* tag = PL_HashTableLookupConst(sTagAtomTable, name);
  if (tag) {
    int32_t index = NS_PTR_TO_INT32(tag) - TABLE_VALUE_OFFSET;
    if (index < 0 || index >= eSVGTag_Count) {
      NS_WARNING("About to index out of array bounds - crashing instead");
      MOZ_CRASH();
    }

    contentCreatorCallback cb = sContentCreatorCallbacks[index];

    nsCOMPtr<nsIContent> content;
    nsresult rv = cb(getter_AddRefs(content), ni.forget(), aFromParser);
    *aResult = content.forget().take()->AsElement();
    return rv;
  }

  
  return NS_NewSVGElement(aResult, ni.forget());
}

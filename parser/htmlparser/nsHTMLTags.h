




#ifndef nsHTMLTags_h___
#define nsHTMLTags_h___

#include "nsString.h"
#include "plhash.h"

class nsIAtom;








#define HTML_TAG(_tag, _classname) eHTMLTag_##_tag,
#define HTML_HTMLELEMENT_TAG(_tag) eHTMLTag_##_tag,
#define HTML_OTHER(_tag) eHTMLTag_##_tag,
enum nsHTMLTag {
  
  eHTMLTag_unknown = 0,
#include "nsHTMLTagList.h"

  

  eHTMLTag_userdefined
};
#undef HTML_TAG
#undef HTML_HTMLELEMENT_TAG
#undef HTML_OTHER


#define NS_HTML_TAG_MAX int32_t(eHTMLTag_text - 1)

class nsHTMLTags {
public:
  static nsresult AddRefTable(void);
  static void ReleaseTable(void);

  
  static nsHTMLTag LookupTag(const nsAString& aTagName);
  static nsHTMLTag CaseSensitiveLookupTag(const char16_t* aTagName)
  {
    NS_ASSERTION(gTagTable, "no lookup table, needs addref");
    NS_ASSERTION(aTagName, "null tagname!");

    void* tag = PL_HashTableLookupConst(gTagTable, aTagName);

    return tag ? (nsHTMLTag)NS_PTR_TO_INT32(tag) : eHTMLTag_userdefined;
  }
  static nsHTMLTag CaseSensitiveLookupTag(nsIAtom* aTagName)
  {
    NS_ASSERTION(gTagAtomTable, "no lookup table, needs addref");
    NS_ASSERTION(aTagName, "null tagname!");

    void* tag = PL_HashTableLookupConst(gTagAtomTable, aTagName);

    return tag ? (nsHTMLTag)NS_PTR_TO_INT32(tag) : eHTMLTag_userdefined;
  }

  
  static const char16_t *GetStringValue(nsHTMLTag aEnum)
  {
    return aEnum <= eHTMLTag_unknown || aEnum > NS_HTML_TAG_MAX ?
      nullptr : sTagUnicodeTable[aEnum - 1];
  }
  static nsIAtom *GetAtom(nsHTMLTag aEnum)
  {
    return aEnum <= eHTMLTag_unknown || aEnum > NS_HTML_TAG_MAX ?
      nullptr : sTagAtomTable[aEnum - 1];
  }

#ifdef DEBUG
  static void TestTagTable();
#endif

private:
  static nsIAtom* sTagAtomTable[eHTMLTag_userdefined - 1];
  static const char16_t* const sTagUnicodeTable[];

  static int32_t gTableRefCount;
  static PLHashTable* gTagTable;
  static PLHashTable* gTagAtomTable;
};

#define eHTMLTags nsHTMLTag

#endif 

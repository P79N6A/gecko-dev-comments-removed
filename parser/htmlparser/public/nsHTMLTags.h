




































#ifndef nsHTMLTags_h___
#define nsHTMLTags_h___

#include "nsStringGlue.h"
#include "plhash.h"

class nsIAtom;








#define HTML_TAG(_tag, _classname) eHTMLTag_##_tag,
#define HTML_OTHER(_tag) eHTMLTag_##_tag,
enum nsHTMLTag {
  
  eHTMLTag_unknown = 0,
#include "nsHTMLTagList.h"

  

  eHTMLTag_userdefined
};
#undef HTML_TAG
#undef HTML_OTHER


#define NS_HTML_TAG_MAX PRInt32(eHTMLTag_text - 1)

class nsHTMLTags {
public:
  static nsresult AddRefTable(void);
  static void ReleaseTable(void);

  
  static nsHTMLTag LookupTag(const nsAString& aTagName);
  static nsHTMLTag CaseSensitiveLookupTag(const PRUnichar* aTagName)
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

  
  static const PRUnichar *GetStringValue(nsHTMLTag aEnum)
  {
    return aEnum <= eHTMLTag_unknown || aEnum > NS_HTML_TAG_MAX ?
      nsnull : sTagUnicodeTable[aEnum - 1];
  }
  static nsIAtom *GetAtom(nsHTMLTag aEnum)
  {
    return aEnum <= eHTMLTag_unknown || aEnum > NS_HTML_TAG_MAX ?
      nsnull : sTagAtomTable[aEnum - 1];
  }

#ifdef DEBUG
  static void TestTagTable();
#endif

private:
  static nsIAtom* sTagAtomTable[eHTMLTag_userdefined - 1];
  static const PRUnichar* const sTagUnicodeTable[];

  static PRInt32 gTableRefCount;
  static PLHashTable* gTagTable;
  static PLHashTable* gTagAtomTable;
};

#define eHTMLTags nsHTMLTag

#endif 





























#ifndef nsHtml5HtmlAttributes_h__
#define nsHtml5HtmlAttributes_h__

#include "prtypes.h"
#include "nsIAtom.h"
#include "nsHtml5AtomTable.h"
#include "nsString.h"
#include "nsINameSpaceManager.h"
#include "nsIContent.h"
#include "nsTraceRefcnt.h"
#include "jArray.h"
#include "nsHtml5ArrayCopy.h"
#include "nsAHtml5TreeBuilderState.h"
#include "nsHtml5Atoms.h"
#include "nsHtml5ByteReadable.h"
#include "nsIUnicodeDecoder.h"
#include "nsHtml5Macros.h"

class nsHtml5StreamParser;

class nsHtml5Tokenizer;
class nsHtml5TreeBuilder;
class nsHtml5MetaScanner;
class nsHtml5AttributeName;
class nsHtml5ElementName;
class nsHtml5UTF16Buffer;
class nsHtml5StateSnapshot;
class nsHtml5Portability;


class nsHtml5HtmlAttributes
{
  public:
    static nsHtml5HtmlAttributes* EMPTY_ATTRIBUTES;
  private:
    int32_t mode;
    int32_t length;
    autoJArray<nsHtml5AttributeName*,int32_t> names;
    autoJArray<nsString*,int32_t> values;
  public:
    nsHtml5HtmlAttributes(int32_t mode);
    ~nsHtml5HtmlAttributes();
    int32_t getIndex(nsHtml5AttributeName* name);
    int32_t getLength();
    nsIAtom* getLocalName(int32_t index);
    nsHtml5AttributeName* getAttributeName(int32_t index);
    int32_t getURI(int32_t index);
    nsIAtom* getPrefix(int32_t index);
    nsString* getValue(int32_t index);
    nsString* getValue(nsHtml5AttributeName* name);
    void addAttribute(nsHtml5AttributeName* name, nsString* value);
    void clear(int32_t m);
    void releaseValue(int32_t i);
    void clearWithoutReleasingContents();
    bool contains(nsHtml5AttributeName* name);
    void adjustForMath();
    void adjustForSvg();
    nsHtml5HtmlAttributes* cloneAttributes(nsHtml5AtomTable* interner);
    bool equalsAnother(nsHtml5HtmlAttributes* other);
    static void initializeStatics();
    static void releaseStatics();
};



#endif


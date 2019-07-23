



























#ifndef nsHtml5HtmlAttributes_h__
#define nsHtml5HtmlAttributes_h__

#include "prtypes.h"
#include "nsIAtom.h"
#include "nsString.h"
#include "nsINameSpaceManager.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsTraceRefcnt.h"
#include "jArray.h"
#include "nsHtml5DocumentMode.h"
#include "nsHtml5ArrayCopy.h"
#include "nsHtml5NamedCharacters.h"
#include "nsHtml5Atoms.h"
#include "nsHtml5ByteReadable.h"
#include "nsIUnicodeDecoder.h"

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
    PRInt32 mode;
    PRInt32 length;
    jArray<nsHtml5AttributeName*,PRInt32> names;
    jArray<nsString*,PRInt32> values;
  public:
    nsHtml5HtmlAttributes(PRInt32 mode);
    ~nsHtml5HtmlAttributes();
    PRInt32 getIndex(nsHtml5AttributeName* name);
    PRInt32 getLength();
    nsIAtom* getLocalName(PRInt32 index);
    nsHtml5AttributeName* getAttributeName(PRInt32 index);
    PRInt32 getURI(PRInt32 index);
    nsIAtom* getPrefix(PRInt32 index);
    nsString* getValue(PRInt32 index);
    nsString* getValue(nsHtml5AttributeName* name);
    void addAttribute(nsHtml5AttributeName* name, nsString* value);
    void clear(PRInt32 m);
    void releaseValue(PRInt32 i);
    void clearWithoutReleasingContents();
    PRBool contains(nsHtml5AttributeName* name);
    void adjustForMath();
    void adjustForSvg();
    static void initializeStatics();
    static void releaseStatics();
};

#ifdef nsHtml5HtmlAttributes_cpp__
nsHtml5HtmlAttributes* nsHtml5HtmlAttributes::EMPTY_ATTRIBUTES = nsnull;
#endif



#endif


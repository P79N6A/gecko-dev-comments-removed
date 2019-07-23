



























#ifndef nsHtml5StackNode_h__
#define nsHtml5StackNode_h__

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
class nsHtml5HtmlAttributes;
class nsHtml5UTF16Buffer;
class nsHtml5StateSnapshot;
class nsHtml5Portability;


class nsHtml5StackNode
{
  public:
    PRInt32 group;
    nsIAtom* name;
    nsIAtom* popName;
    PRInt32 ns;
    nsIContent* node;
    PRBool scoping;
    PRBool special;
    PRBool fosterParenting;
    nsHtml5HtmlAttributes* attributes;
  private:
    PRInt32 refcount;
  public:
    nsHtml5StackNode(PRInt32 group, PRInt32 ns, nsIAtom* name, nsIContent* node, PRBool scoping, PRBool special, PRBool fosterParenting, nsIAtom* popName, nsHtml5HtmlAttributes* attributes);
    nsHtml5StackNode(PRInt32 ns, nsHtml5ElementName* elementName, nsIContent* node);
    nsHtml5StackNode(PRInt32 ns, nsHtml5ElementName* elementName, nsIContent* node, nsHtml5HtmlAttributes* attributes);
    nsHtml5StackNode(PRInt32 ns, nsHtml5ElementName* elementName, nsIContent* node, nsIAtom* popName);
    nsHtml5StackNode(PRInt32 ns, nsHtml5ElementName* elementName, nsIContent* node, nsIAtom* popName, PRBool scoping);
    ~nsHtml5StackNode();
    void dropAttributes();
    void retain();
    void release();
    static void initializeStatics();
    static void releaseStatics();

#include "nsHtml5StackNodeHSupplement.h"
};

#ifdef nsHtml5StackNode_cpp__
#endif



#endif


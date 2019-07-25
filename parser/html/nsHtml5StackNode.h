



























#ifndef nsHtml5StackNode_h__
#define nsHtml5StackNode_h__

#include "prtypes.h"
#include "nsIAtom.h"
#include "nsHtml5AtomTable.h"
#include "nsString.h"
#include "nsINameSpaceManager.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsTraceRefcnt.h"
#include "jArray.h"
#include "nsHtml5DocumentMode.h"
#include "nsHtml5ArrayCopy.h"
#include "nsHtml5NamedCharacters.h"
#include "nsHtml5NamedCharactersAccel.h"
#include "nsHtml5Atoms.h"
#include "nsHtml5ByteReadable.h"
#include "nsIUnicodeDecoder.h"
#include "nsAHtml5TreeBuilderState.h"
#include "nsHtml5Macros.h"

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
    PRInt32 flags;
    nsIAtom* name;
    nsIAtom* popName;
    PRInt32 ns;
    nsIContent** node;
    nsHtml5HtmlAttributes* attributes;
  private:
    PRInt32 refcount;
  public:
    inline PRInt32 getFlags()
    {
      return flags;
    }

    PRInt32 getGroup();
    bool isScoping();
    bool isSpecial();
    bool isFosterParenting();
    bool isHtmlIntegrationPoint();
    nsHtml5StackNode(PRInt32 flags, PRInt32 ns, nsIAtom* name, nsIContent** node, nsIAtom* popName, nsHtml5HtmlAttributes* attributes);
    nsHtml5StackNode(nsHtml5ElementName* elementName, nsIContent** node);
    nsHtml5StackNode(nsHtml5ElementName* elementName, nsIContent** node, nsHtml5HtmlAttributes* attributes);
    nsHtml5StackNode(nsHtml5ElementName* elementName, nsIContent** node, nsIAtom* popName);
    nsHtml5StackNode(nsHtml5ElementName* elementName, nsIAtom* popName, nsIContent** node);
    nsHtml5StackNode(nsHtml5ElementName* elementName, nsIContent** node, nsIAtom* popName, bool markAsIntegrationPoint);
  private:
    static PRInt32 prepareSvgFlags(PRInt32 flags);
    static PRInt32 prepareMathFlags(PRInt32 flags, bool markAsIntegrationPoint);
  public:
    ~nsHtml5StackNode();
    void dropAttributes();
    void retain();
    void release();
    static void initializeStatics();
    static void releaseStatics();
};



#endif


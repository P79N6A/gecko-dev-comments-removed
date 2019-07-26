



























#ifndef nsHtml5StackNode_h__
#define nsHtml5StackNode_h__

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
class nsHtml5HtmlAttributes;
class nsHtml5UTF16Buffer;
class nsHtml5StateSnapshot;
class nsHtml5Portability;


class nsHtml5StackNode
{
  public:
    int32_t flags;
    nsIAtom* name;
    nsIAtom* popName;
    int32_t ns;
    nsIContent** node;
    nsHtml5HtmlAttributes* attributes;
  private:
    int32_t refcount;
  public:
    inline int32_t getFlags()
    {
      return flags;
    }

    int32_t getGroup();
    bool isScoping();
    bool isSpecial();
    bool isFosterParenting();
    bool isHtmlIntegrationPoint();
    nsHtml5StackNode(int32_t flags, int32_t ns, nsIAtom* name, nsIContent** node, nsIAtom* popName, nsHtml5HtmlAttributes* attributes);
    nsHtml5StackNode(nsHtml5ElementName* elementName, nsIContent** node);
    nsHtml5StackNode(nsHtml5ElementName* elementName, nsIContent** node, nsHtml5HtmlAttributes* attributes);
    nsHtml5StackNode(nsHtml5ElementName* elementName, nsIContent** node, nsIAtom* popName);
    nsHtml5StackNode(nsHtml5ElementName* elementName, nsIAtom* popName, nsIContent** node);
    nsHtml5StackNode(nsHtml5ElementName* elementName, nsIContent** node, nsIAtom* popName, bool markAsIntegrationPoint);
  private:
    static int32_t prepareSvgFlags(int32_t flags);
    static int32_t prepareMathFlags(int32_t flags, bool markAsIntegrationPoint);
  public:
    ~nsHtml5StackNode();
    void dropAttributes();
    void retain();
    void release();
    static void initializeStatics();
    static void releaseStatics();
};



#endif


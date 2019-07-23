



























#ifndef nsHtml5StackNode_h__
#define nsHtml5StackNode_h__

#include "prtypes.h"
#include "nsIAtom.h"
#include "nsString.h"
#include "nsINameSpaceManager.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "jArray.h"
#include "nsHtml5DocumentMode.h"
#include "nsHtml5ArrayCopy.h"
#include "nsHtml5NamedCharacters.h"
#include "nsHtml5Parser.h"
#include "nsHtml5StringLiterals.h"
#include "nsHtml5Atoms.h"

class nsHtml5Parser;

class nsHtml5Tokenizer;
class nsHtml5TreeBuilder;
class nsHtml5AttributeName;
class nsHtml5ElementName;
class nsHtml5HtmlAttributes;
class nsHtml5UTF16Buffer;
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
    PRBool tainted;
    nsHtml5StackNode(PRInt32 group, PRInt32 ns, nsIAtom* name, nsIContent* node, PRBool scoping, PRBool special, PRBool fosterParenting, nsIAtom* popName);
    nsHtml5StackNode(PRInt32 ns, nsHtml5ElementName* elementName, nsIContent* node);
    nsHtml5StackNode(PRInt32 ns, nsHtml5ElementName* elementName, nsIContent* node, nsIAtom* popName);
    nsHtml5StackNode(PRInt32 ns, nsHtml5ElementName* elementName, nsIContent* node, nsIAtom* popName, PRBool scoping);
  private:
    void destructor();
  public:
    static void initializeStatics();
    static void releaseStatics();
};

#ifdef nsHtml5StackNode_cpp__
#endif



#endif






























#define nsHtml5StackNode_cpp__

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
#include "nsHtml5ByteReadable.h"

#include "nsHtml5Tokenizer.h"
#include "nsHtml5TreeBuilder.h"
#include "nsHtml5MetaScanner.h"
#include "nsHtml5AttributeName.h"
#include "nsHtml5ElementName.h"
#include "nsHtml5HtmlAttributes.h"
#include "nsHtml5UTF16Buffer.h"
#include "nsHtml5Portability.h"

#include "nsHtml5StackNode.h"


nsHtml5StackNode::nsHtml5StackNode(PRInt32 group, PRInt32 ns, nsIAtom* name, nsIContent* node, PRBool scoping, PRBool special, PRBool fosterParenting, nsIAtom* popName)
  : group(group),
    name(name),
    popName(popName),
    ns(ns),
    node(node),
    scoping(scoping),
    special(special),
    fosterParenting(fosterParenting),
    tainted(PR_FALSE)
{
  nsHtml5Portability::retainLocal(name);
  nsHtml5Portability::retainLocal(popName);
  nsHtml5Portability::retainElement(node);
}


nsHtml5StackNode::nsHtml5StackNode(PRInt32 ns, nsHtml5ElementName* elementName, nsIContent* node)
  : group(elementName->group),
    name(elementName->name),
    popName(elementName->name),
    ns(ns),
    node(node),
    scoping(elementName->scoping),
    special(elementName->special),
    fosterParenting(elementName->fosterParenting),
    tainted(PR_FALSE)
{
  nsHtml5Portability::retainLocal(name);
  nsHtml5Portability::retainLocal(popName);
  nsHtml5Portability::retainElement(node);
}


nsHtml5StackNode::nsHtml5StackNode(PRInt32 ns, nsHtml5ElementName* elementName, nsIContent* node, nsIAtom* popName)
  : group(elementName->group),
    name(elementName->name),
    popName(popName),
    ns(ns),
    node(node),
    scoping(elementName->scoping),
    special(elementName->special),
    fosterParenting(elementName->fosterParenting),
    tainted(PR_FALSE)
{
  nsHtml5Portability::retainLocal(name);
  nsHtml5Portability::retainLocal(popName);
  nsHtml5Portability::retainElement(node);
}


nsHtml5StackNode::nsHtml5StackNode(PRInt32 ns, nsHtml5ElementName* elementName, nsIContent* node, nsIAtom* popName, PRBool scoping)
  : group(elementName->group),
    name(elementName->name),
    popName(popName),
    ns(ns),
    node(node),
    scoping(scoping),
    special(PR_FALSE),
    fosterParenting(PR_FALSE),
    tainted(PR_FALSE)
{
  nsHtml5Portability::retainLocal(name);
  nsHtml5Portability::retainLocal(popName);
  nsHtml5Portability::retainElement(node);
}


nsHtml5StackNode::~nsHtml5StackNode()
{
  nsHtml5Portability::releaseLocal(name);
  nsHtml5Portability::releaseLocal(popName);
  nsHtml5Portability::releaseElement(node);
}

void 
nsHtml5StackNode::retain()
{
  refcount++;
}

void 
nsHtml5StackNode::release()
{
  refcount--;
  if (!refcount) {
    delete this;
  }
}

void
nsHtml5StackNode::initializeStatics()
{
}

void
nsHtml5StackNode::releaseStatics()
{
}



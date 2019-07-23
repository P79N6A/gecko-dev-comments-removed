


























#define nsHtml5StateSnapshot_cpp__

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

#include "nsHtml5Tokenizer.h"
#include "nsHtml5TreeBuilder.h"
#include "nsHtml5MetaScanner.h"
#include "nsHtml5AttributeName.h"
#include "nsHtml5ElementName.h"
#include "nsHtml5HtmlAttributes.h"
#include "nsHtml5StackNode.h"
#include "nsHtml5UTF16Buffer.h"
#include "nsHtml5Portability.h"

#include "nsHtml5StateSnapshot.h"


nsHtml5StateSnapshot::nsHtml5StateSnapshot(jArray<nsHtml5StackNode*,PRInt32> stack, jArray<nsHtml5StackNode*,PRInt32> listOfActiveFormattingElements, nsIContent* formPointer)
  : stack(stack),
    listOfActiveFormattingElements(listOfActiveFormattingElements),
    formPointer(formPointer)
{
  MOZ_COUNT_CTOR(nsHtml5StateSnapshot);
}


nsHtml5StateSnapshot::~nsHtml5StateSnapshot()
{
  MOZ_COUNT_DTOR(nsHtml5StateSnapshot);
  for (PRInt32 i = 0; i < stack.length; i++) {
    stack[i]->release();
  }
  stack.release();
  for (PRInt32 i = 0; i < listOfActiveFormattingElements.length; i++) {
    if (!!listOfActiveFormattingElements[i]) {
      listOfActiveFormattingElements[i]->release();
    }
  }
  listOfActiveFormattingElements.release();
  nsHtml5Portability::retainElement(formPointer);
}

void
nsHtml5StateSnapshot::initializeStatics()
{
}

void
nsHtml5StateSnapshot::releaseStatics()
{
}



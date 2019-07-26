


























#define nsHtml5UTF16Buffer_cpp__

#include "nsIAtom.h"
#include "nsHtml5AtomTable.h"
#include "nsString.h"
#include "nsNameSpaceManager.h"
#include "nsIContent.h"
#include "nsISupportsImpl.h"
#include "jArray.h"
#include "nsHtml5ArrayCopy.h"
#include "nsAHtml5TreeBuilderState.h"
#include "nsHtml5Atoms.h"
#include "nsHtml5ByteReadable.h"
#include "nsIUnicodeDecoder.h"
#include "nsHtml5Macros.h"

#include "nsHtml5Tokenizer.h"
#include "nsHtml5TreeBuilder.h"
#include "nsHtml5MetaScanner.h"
#include "nsHtml5AttributeName.h"
#include "nsHtml5ElementName.h"
#include "nsHtml5HtmlAttributes.h"
#include "nsHtml5StackNode.h"
#include "nsHtml5StateSnapshot.h"
#include "nsHtml5Portability.h"

#include "nsHtml5UTF16Buffer.h"

int32_t 
nsHtml5UTF16Buffer::getStart()
{
  return start;
}

void 
nsHtml5UTF16Buffer::setStart(int32_t start)
{
  this->start = start;
}

char16_t* 
nsHtml5UTF16Buffer::getBuffer()
{
  return buffer;
}

int32_t 
nsHtml5UTF16Buffer::getEnd()
{
  return end;
}

bool 
nsHtml5UTF16Buffer::hasMore()
{
  return start < end;
}

void 
nsHtml5UTF16Buffer::adjust(bool lastWasCR)
{
  if (lastWasCR && buffer[start] == '\n') {
    start++;
  }
}

void 
nsHtml5UTF16Buffer::setEnd(int32_t end)
{
  this->end = end;
}

void
nsHtml5UTF16Buffer::initializeStatics()
{
}

void
nsHtml5UTF16Buffer::releaseStatics()
{
}


#include "nsHtml5UTF16BufferCppSupplement.h"


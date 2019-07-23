


























#define nsHtml5UTF16Buffer_cpp__

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
#include "nsHtml5StateSnapshot.h"
#include "nsHtml5Portability.h"

#include "nsHtml5UTF16Buffer.h"


nsHtml5UTF16Buffer::nsHtml5UTF16Buffer(PRUnichar* buffer, PRInt32 start, PRInt32 end)
  : buffer(buffer),
    start(start),
    end(end)
{
  MOZ_COUNT_CTOR(nsHtml5UTF16Buffer);
}

PRInt32 
nsHtml5UTF16Buffer::getStart()
{
  return start;
}

void 
nsHtml5UTF16Buffer::setStart(PRInt32 start)
{
  this->start = start;
}

PRUnichar* 
nsHtml5UTF16Buffer::getBuffer()
{
  return buffer;
}

PRInt32 
nsHtml5UTF16Buffer::getEnd()
{
  return end;
}

PRBool 
nsHtml5UTF16Buffer::hasMore()
{
  return start < end;
}

void 
nsHtml5UTF16Buffer::adjust(PRBool lastWasCR)
{
  if (lastWasCR && buffer[start] == '\n') {
    start++;
  }
}

void 
nsHtml5UTF16Buffer::setEnd(PRInt32 end)
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


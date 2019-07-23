


























#ifndef nsHtml5UTF16Buffer_h__
#define nsHtml5UTF16Buffer_h__

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
#include "nsHtml5Atoms.h"
#include "nsHtml5ByteReadable.h"
#include "nsIUnicodeDecoder.h"
#include "nsAHtml5TreeBuilderState.h"

class nsHtml5StreamParser;
class nsHtml5SpeculativeLoader;

class nsHtml5Tokenizer;
class nsHtml5TreeBuilder;
class nsHtml5MetaScanner;
class nsHtml5AttributeName;
class nsHtml5ElementName;
class nsHtml5HtmlAttributes;
class nsHtml5StateSnapshot;
class nsHtml5Portability;


class nsHtml5UTF16Buffer
{
  private:
    PRUnichar* buffer;
    PRInt32 start;
    PRInt32 end;
  public:
    nsHtml5UTF16Buffer(PRUnichar* buffer, PRInt32 start, PRInt32 end);
    PRInt32 getStart();
    void setStart(PRInt32 start);
    PRUnichar* getBuffer();
    PRInt32 getEnd();
    PRBool hasMore();
    void adjust(PRBool lastWasCR);
    void setEnd(PRInt32 end);
    static void initializeStatics();
    static void releaseStatics();

#include "nsHtml5UTF16BufferHSupplement.h"
};

#ifdef nsHtml5UTF16Buffer_cpp__
#endif



#endif


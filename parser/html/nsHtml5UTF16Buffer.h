


























#ifndef nsHtml5UTF16Buffer_h
#define nsHtml5UTF16Buffer_h

#include "nsIAtom.h"
#include "nsHtml5AtomTable.h"
#include "nsString.h"
#include "nsNameSpaceManager.h"
#include "nsIContent.h"
#include "nsTraceRefcnt.h"
#include "jArray.h"
#include "nsHtml5ArrayCopy.h"
#include "nsAHtml5TreeBuilderState.h"
#include "nsHtml5Atoms.h"
#include "nsHtml5ByteReadable.h"
#include "nsIUnicodeDecoder.h"
#include "nsHtml5Macros.h"
#include "nsIContentHandle.h"

class nsHtml5StreamParser;

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
    char16_t* buffer;
    int32_t start;
    int32_t end;
  public:
    int32_t getStart();
    void setStart(int32_t start);
    char16_t* getBuffer();
    int32_t getEnd();
    bool hasMore();
    void adjust(bool lastWasCR);
    void setEnd(int32_t end);
    static void initializeStatics();
    static void releaseStatics();

#include "nsHtml5UTF16BufferHSupplement.h"
};



#endif


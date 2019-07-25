


























#ifndef nsHtml5StateSnapshot_h__
#define nsHtml5StateSnapshot_h__

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
class nsHtml5Portability;


class nsHtml5StateSnapshot : public nsAHtml5TreeBuilderState
{
  private:
    autoJArray<nsHtml5StackNode*,int32_t> stack;
    autoJArray<nsHtml5StackNode*,int32_t> listOfActiveFormattingElements;
    nsIContent** formPointer;
    nsIContent** headPointer;
    nsIContent** deepTreeSurrogateParent;
    int32_t mode;
    int32_t originalMode;
    bool framesetOk;
    bool needToDropLF;
    bool quirks;
  public:
    nsHtml5StateSnapshot(jArray<nsHtml5StackNode*,int32_t> stack, jArray<nsHtml5StackNode*,int32_t> listOfActiveFormattingElements, nsIContent** formPointer, nsIContent** headPointer, nsIContent** deepTreeSurrogateParent, int32_t mode, int32_t originalMode, bool framesetOk, bool needToDropLF, bool quirks);
    jArray<nsHtml5StackNode*,int32_t> getStack();
    jArray<nsHtml5StackNode*,int32_t> getListOfActiveFormattingElements();
    nsIContent** getFormPointer();
    nsIContent** getHeadPointer();
    nsIContent** getDeepTreeSurrogateParent();
    int32_t getMode();
    int32_t getOriginalMode();
    bool isFramesetOk();
    bool isNeedToDropLF();
    bool isQuirks();
    int32_t getListOfActiveFormattingElementsLength();
    int32_t getStackLength();
    ~nsHtml5StateSnapshot();
    static void initializeStatics();
    static void releaseStatics();
};



#endif


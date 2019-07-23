


























#ifndef nsHtml5StateSnapshot_h__
#define nsHtml5StateSnapshot_h__

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
class nsHtml5UTF16Buffer;
class nsHtml5Portability;


class nsHtml5StateSnapshot : public nsAHtml5TreeBuilderState
{
  private:
    jArray<nsHtml5StackNode*,PRInt32> stack;
    jArray<nsHtml5StackNode*,PRInt32> listOfActiveFormattingElements;
    nsIContent** formPointer;
    nsIContent** headPointer;
    PRInt32 mode;
    PRInt32 originalMode;
    PRInt32 foreignFlag;
    PRBool needToDropLF;
    PRBool quirks;
  public:
    nsHtml5StateSnapshot(jArray<nsHtml5StackNode*,PRInt32> stack, jArray<nsHtml5StackNode*,PRInt32> listOfActiveFormattingElements, nsIContent** formPointer, nsIContent** headPointer, PRInt32 mode, PRInt32 originalMode, PRInt32 foreignFlag, PRBool needToDropLF, PRBool quirks);
    jArray<nsHtml5StackNode*,PRInt32> getStack();
    jArray<nsHtml5StackNode*,PRInt32> getListOfActiveFormattingElements();
    nsIContent** getFormPointer();
    nsIContent** getHeadPointer();
    PRInt32 getMode();
    PRInt32 getOriginalMode();
    PRInt32 getForeignFlag();
    PRBool isNeedToDropLF();
    PRBool isQuirks();
    PRInt32 getListLength();
    PRInt32 getStackLength();
    ~nsHtml5StateSnapshot();
    static void initializeStatics();
    static void releaseStatics();
};

#ifdef nsHtml5StateSnapshot_cpp__
#endif



#endif


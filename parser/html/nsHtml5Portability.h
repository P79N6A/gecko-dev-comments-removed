


























#ifndef nsHtml5Portability_h__
#define nsHtml5Portability_h__

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
class nsHtml5StateSnapshot;


class nsHtml5Portability
{
  public:
    static nsIAtom* newLocalNameFromBuffer(PRUnichar* buf, PRInt32 offset, PRInt32 length, nsHtml5AtomTable* interner);
    static nsString* newStringFromBuffer(PRUnichar* buf, PRInt32 offset, PRInt32 length);
    static nsString* newEmptyString();
    static nsString* newStringFromLiteral(const char* literal);
    static nsString* newStringFromString(nsString* string);
    static jArray<PRUnichar,PRInt32> newCharArrayFromLocal(nsIAtom* local);
    static jArray<PRUnichar,PRInt32> newCharArrayFromString(nsString* string);
    static nsIAtom* newLocalFromLocal(nsIAtom* local, nsHtml5AtomTable* interner);
    static void releaseString(nsString* str);
    static void retainLocal(nsIAtom* local);
    static void releaseLocal(nsIAtom* local);
    static PRBool localEqualsBuffer(nsIAtom* local, PRUnichar* buf, PRInt32 offset, PRInt32 length);
    static PRBool lowerCaseLiteralIsPrefixOfIgnoreAsciiCaseString(const char* lowerCaseLiteral, nsString* string);
    static PRBool lowerCaseLiteralEqualsIgnoreAsciiCaseString(const char* lowerCaseLiteral, nsString* string);
    static PRBool literalEqualsString(const char* literal, nsString* string);
    static jArray<PRUnichar,PRInt32> isIndexPrompt();
    static void initializeStatics();
    static void releaseStatics();
};

#ifdef nsHtml5Portability_cpp__
#endif



#endif


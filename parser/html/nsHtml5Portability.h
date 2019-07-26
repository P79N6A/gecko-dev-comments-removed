


























#ifndef nsHtml5Portability_h__
#define nsHtml5Portability_h__

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
class nsHtml5StateSnapshot;


class nsHtml5Portability
{
  public:
    static nsIAtom* newLocalNameFromBuffer(PRUnichar* buf, int32_t offset, int32_t length, nsHtml5AtomTable* interner);
    static nsString* newStringFromBuffer(PRUnichar* buf, int32_t offset, int32_t length);
    static nsString* newEmptyString();
    static nsString* newStringFromLiteral(const char* literal);
    static nsString* newStringFromString(nsString* string);
    static jArray<PRUnichar,int32_t> newCharArrayFromLocal(nsIAtom* local);
    static jArray<PRUnichar,int32_t> newCharArrayFromString(nsString* string);
    static nsIAtom* newLocalFromLocal(nsIAtom* local, nsHtml5AtomTable* interner);
    static void releaseString(nsString* str);
    static bool localEqualsBuffer(nsIAtom* local, PRUnichar* buf, int32_t offset, int32_t length);
    static bool lowerCaseLiteralIsPrefixOfIgnoreAsciiCaseString(const char* lowerCaseLiteral, nsString* string);
    static bool lowerCaseLiteralEqualsIgnoreAsciiCaseString(const char* lowerCaseLiteral, nsString* string);
    static bool literalEqualsString(const char* literal, nsString* string);
    static bool stringEqualsString(nsString* one, nsString* other);
    static void initializeStatics();
    static void releaseStatics();
};



#endif


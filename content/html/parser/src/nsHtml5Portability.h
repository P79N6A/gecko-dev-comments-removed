


























#ifndef nsHtml5Portability_h__
#define nsHtml5Portability_h__

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

class nsHtml5Parser;

class nsHtml5Tokenizer;
class nsHtml5TreeBuilder;
class nsHtml5AttributeName;
class nsHtml5ElementName;
class nsHtml5HtmlAttributes;
class nsHtml5StackNode;
class nsHtml5UTF16Buffer;

class nsHtml5Portability
{
  public:
    static nsIAtom* newLocalNameFromBuffer(PRUnichar* buf, PRInt32 offset, PRInt32 length);
    static nsString* newStringFromBuffer(PRUnichar* buf, PRInt32 offset, PRInt32 length);
    static nsString* newEmptyString();
    static jArray<PRUnichar,PRInt32> newCharArrayFromLocal(nsIAtom* local);
    static jArray<PRUnichar,PRInt32> newCharArrayFromString(nsString* string);
    static void releaseString(nsString* str);
    static void retainLocal(nsIAtom* local);
    static void releaseLocal(nsIAtom* local);
    static void retainElement(nsIContent* elt);
    static void releaseElement(nsIContent* elt);
    static PRBool localEqualsBuffer(nsIAtom* local, PRUnichar* buf, PRInt32 offset, PRInt32 length);
    static PRBool lowerCaseLiteralIsPrefixOfIgnoreAsciiCaseString(nsString* lowerCaseLiteral, nsString* string);
    static PRBool lowerCaseLiteralEqualsIgnoreAsciiCaseString(nsString* lowerCaseLiteral, nsString* string);
    static PRBool literalEqualsString(nsString* literal, nsString* string);
    static jArray<PRUnichar,PRInt32> isIndexPrompt();
    static void initializeStatics();
    static void releaseStatics();
};

#ifdef nsHtml5Portability_cpp__
#endif



#endif


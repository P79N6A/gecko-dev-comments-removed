











































#include "nsHTMLContentSerializer.h"

#include "nsIDOMElement.h"
#include "nsIDOMText.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsINameSpaceManager.h"
#include "nsString.h"
#include "nsUnicharUtils.h"
#include "nsXPIDLString.h"
#include "nsIServiceManager.h"
#include "nsIDocumentEncoder.h"
#include "nsGkAtoms.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsEscape.h"
#include "nsITextToSubURI.h"
#include "nsCRT.h"
#include "nsIParserService.h"
#include "nsContentUtils.h"
#include "nsLWBrkCIID.h"
#include "nsIScriptElement.h"
#include "nsAttrName.h"

#define kIndentStr NS_LITERAL_STRING("  ")
#define kLessThan NS_LITERAL_STRING("<")
#define kGreaterThan NS_LITERAL_STRING(">")
#define kEndTag NS_LITERAL_STRING("</")

static const char kMozStr[] = "moz";

static const PRInt32 kLongLineLen = 128;

nsresult NS_NewHTMLContentSerializer(nsIContentSerializer** aSerializer)
{
  nsHTMLContentSerializer* it = new nsHTMLContentSerializer();
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return CallQueryInterface(it, aSerializer);
}

nsHTMLContentSerializer::nsHTMLContentSerializer()
: mIndent(0),
  mColPos(0),
  mInBody(PR_FALSE),
  mAddSpace(PR_FALSE),
  mMayIgnoreLineBreakSequence(PR_FALSE),
  mInCDATA(PR_FALSE),
  mNeedLineBreaker(PR_TRUE)
{
}

nsHTMLContentSerializer::~nsHTMLContentSerializer()
{
  NS_ASSERTION(mOLStateStack.Count() == 0, "Expected OL State stack to be empty");
  if (mOLStateStack.Count() > 0){
    for (PRInt32 i = 0; i < mOLStateStack.Count(); i++){
      olState* state = (olState*)mOLStateStack[i];
      delete state;
      mOLStateStack.RemoveElementAt(i);
    }
  }
}

NS_IMETHODIMP 
nsHTMLContentSerializer::Init(PRUint32 aFlags, PRUint32 aWrapColumn,
                              const char* aCharSet, PRBool aIsCopying)
{
  mFlags = aFlags;
  if (!aWrapColumn) {
    mMaxColumn = 72;
  }
  else {
    mMaxColumn = aWrapColumn;
  }

  mIsCopying = aIsCopying;
  mIsFirstChildOfOL = PR_FALSE;
  mDoFormat = (mFlags & nsIDocumentEncoder::OutputFormatted) ? PR_TRUE
                                                             : PR_FALSE;
  mBodyOnly = (mFlags & nsIDocumentEncoder::OutputBodyOnly) ? PR_TRUE
                                                            : PR_FALSE;
  
  if ((mFlags & nsIDocumentEncoder::OutputCRLineBreak)
      && (mFlags & nsIDocumentEncoder::OutputLFLineBreak)) { 
    mLineBreak.AssignLiteral("\r\n");
  }
  else if (mFlags & nsIDocumentEncoder::OutputCRLineBreak) { 
    mLineBreak.AssignLiteral("\r");
  }
  else if (mFlags & nsIDocumentEncoder::OutputLFLineBreak) { 
    mLineBreak.AssignLiteral("\n");
  }
  else {
    mLineBreak.AssignLiteral(NS_LINEBREAK);         
  }

  mPreLevel = 0;

  mCharset = aCharSet;

  
  if (mFlags & nsIDocumentEncoder::OutputEncodeW3CEntities) {
    mEntityConverter = do_CreateInstance(NS_ENTITYCONVERTER_CONTRACTID);
  }

  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLContentSerializer::AppendText(nsIDOMText* aText, 
                                    PRInt32 aStartOffset,
                                    PRInt32 aEndOffset,
                                    nsAString& aStr)
{
  NS_ENSURE_ARG(aText);

  if (mNeedLineBreaker) {
    mNeedLineBreaker = PR_FALSE;

    nsCOMPtr<nsIDOMDocument> domDoc;
    aText->GetOwnerDocument(getter_AddRefs(domDoc));
    nsCOMPtr<nsIDocument> document = do_QueryInterface(domDoc);
  }

  nsAutoString data;

  nsresult rv;
  rv = AppendTextData((nsIDOMNode*)aText, aStartOffset, 
                      aEndOffset, data, PR_TRUE, PR_FALSE);
  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;

  if (mPreLevel > 0) {
    AppendToStringConvertLF(data, aStr);
  }
  else if (mFlags & nsIDocumentEncoder::OutputRaw) {
    PRInt32 lastNewlineOffset = data.RFindChar('\n');
    AppendToString(data, aStr);
    if (lastNewlineOffset != kNotFound)
      mColPos = data.Length() - lastNewlineOffset;
  }
  else if (!mDoFormat) {
    PRInt32 lastNewlineOffset = kNotFound;
    PRBool hasLongLines = HasLongLines(data, lastNewlineOffset);
    if (hasLongLines) {
      
      AppendToStringWrapped(data, aStr, PR_FALSE);
      if (lastNewlineOffset != kNotFound)
        mColPos = data.Length() - lastNewlineOffset;
    }
    else {
      AppendToStringConvertLF(data, aStr);
    }
  }
  else {
    AppendToStringWrapped(data, aStr, PR_FALSE);
  }

  return NS_OK;
}

void nsHTMLContentSerializer::AppendWrapped_WhitespaceSequence(
                               nsASingleFragmentString::const_char_iterator &aPos,
                               const nsASingleFragmentString::const_char_iterator aEnd,
                               const nsASingleFragmentString::const_char_iterator aSequenceStart,
                               PRBool &aMayIgnoreStartOfLineWhitespaceSequence,
                               nsAString &aOutputStr)
{
  
  
  
  
  

  PRBool sawBlankOrTab = PR_FALSE;
  PRBool leaveLoop = PR_FALSE;

  do {
    switch (*aPos) {
      case ' ':
      case '\t':
        sawBlankOrTab = PR_TRUE;
        
      case '\n':
        ++aPos;
        
        
        break;
      default:
        leaveLoop = PR_TRUE;
        break;
    }
  } while (!leaveLoop && aPos < aEnd);

  if (mAddSpace) {
    
    
  }
  else if (!sawBlankOrTab && mMayIgnoreLineBreakSequence) {
    
    mMayIgnoreLineBreakSequence = PR_FALSE;
  }
  else if (aMayIgnoreStartOfLineWhitespaceSequence) {
    
    aMayIgnoreStartOfLineWhitespaceSequence = PR_FALSE;
  }
  else {
    if (sawBlankOrTab) {
      if (mColPos + 1 >= mMaxColumn) {
        
        
        aOutputStr.Append(mLineBreak);
        mColPos = 0;
      }
      else {
        
        

        mAddSpace = PR_TRUE;
        ++mColPos; 
      }
    }
    else {
      
      
      
      
      aOutputStr.Append(mLineBreak);
      mMayIgnoreLineBreakSequence = PR_TRUE;
      mColPos = 0;
    }
  }
}

void nsHTMLContentSerializer::AppendWrapped_NonWhitespaceSequence(
                               nsASingleFragmentString::const_char_iterator &aPos,
                               const nsASingleFragmentString::const_char_iterator aEnd,
                               const nsASingleFragmentString::const_char_iterator aSequenceStart,
                               PRBool &aMayIgnoreStartOfLineWhitespaceSequence,
                               nsAString& aOutputStr)
{
  mMayIgnoreLineBreakSequence = PR_FALSE;
  aMayIgnoreStartOfLineWhitespaceSequence = PR_FALSE;

  
  
  
  
  

  PRBool thisSequenceStartsAtBeginningOfLine = !mColPos;
  PRBool onceAgainBecauseWeAddedBreakInFront;
  PRBool foundWhitespaceInLoop;

  do {
    onceAgainBecauseWeAddedBreakInFront = PR_FALSE;
    foundWhitespaceInLoop = PR_FALSE;

    do {
      if (*aPos == ' ' || *aPos == '\t' || *aPos == '\n') {
        foundWhitespaceInLoop = PR_TRUE;
        break;
      }

      ++aPos;
      ++mColPos;
    } while (mColPos < mMaxColumn && aPos < aEnd);

    if (aPos == aEnd || foundWhitespaceInLoop) {
      

      if (mAddSpace) {
        aOutputStr.Append(PRUnichar(' '));
        mAddSpace = PR_FALSE;
      }

      aOutputStr.Append(aSequenceStart, aPos - aSequenceStart);
      
      
    }
    else { 
      if (!thisSequenceStartsAtBeginningOfLine && mAddSpace) {
        

        aOutputStr.Append(mLineBreak);
        mAddSpace = PR_FALSE;
        aPos = aSequenceStart;
        mColPos = 0;
        thisSequenceStartsAtBeginningOfLine = PR_TRUE;
        onceAgainBecauseWeAddedBreakInFront = PR_TRUE;
      }
      else {
        

        PRBool foundWrapPosition = PR_FALSE;
        nsILineBreaker *lineBreaker = nsContentUtils::LineBreaker();

        PRInt32 wrapPosition;

        wrapPosition = lineBreaker->Prev(aSequenceStart,
                                         (aEnd - aSequenceStart),
                                         (aPos - aSequenceStart) + 1);
        if (wrapPosition != NS_LINEBREAKER_NEED_MORE_TEXT) {
          foundWrapPosition = PR_TRUE;
        }
        else {
          wrapPosition = lineBreaker->Next(aSequenceStart,
                                           (aEnd - aSequenceStart),
                                           (aPos - aSequenceStart));
          if (wrapPosition != NS_LINEBREAKER_NEED_MORE_TEXT) {
            foundWrapPosition = PR_TRUE;
          }
        }

        if (foundWrapPosition) {
          if (mAddSpace) {
            aOutputStr.Append(PRUnichar(' '));
            mAddSpace = PR_FALSE;
          }

          aOutputStr.Append(aSequenceStart, wrapPosition);
          aOutputStr.Append(mLineBreak);
          aPos = aSequenceStart + wrapPosition;
          mColPos = 0;
          aMayIgnoreStartOfLineWhitespaceSequence = PR_TRUE;
          mMayIgnoreLineBreakSequence = PR_TRUE;
        }
        else {
          
          
          

          do {
            if (*aPos == ' ' || *aPos == '\t' || *aPos == '\n') {
              break;
            }

            ++aPos;
            ++mColPos;
          } while (aPos < aEnd);

          if (mAddSpace) {
            aOutputStr.Append(PRUnichar(' '));
            mAddSpace = PR_FALSE;
          }

          aOutputStr.Append(aSequenceStart, aPos - aSequenceStart);
        }
      }
    }
  } while (onceAgainBecauseWeAddedBreakInFront);
}

void 
nsHTMLContentSerializer::AppendToStringWrapped(const nsASingleFragmentString& aStr,
                                               nsAString& aOutputStr,
                                               PRBool aTranslateEntities)
{
  nsASingleFragmentString::const_char_iterator pos, end, sequenceStart;

  aStr.BeginReading(pos);
  aStr.EndReading(end);

  
  

  PRBool mayIgnoreStartOfLineWhitespaceSequence = !mColPos;

  while (pos < end) {
    sequenceStart = pos;

    
    if (*pos == ' ' || *pos == '\n' || *pos == '\t') {
      AppendWrapped_WhitespaceSequence(pos, end, sequenceStart, 
        mayIgnoreStartOfLineWhitespaceSequence, aOutputStr);
    }
    else { 
      AppendWrapped_NonWhitespaceSequence(pos, end, sequenceStart, 
        mayIgnoreStartOfLineWhitespaceSequence, aOutputStr);
    }
  }
}

NS_IMETHODIMP
nsHTMLContentSerializer::AppendDocumentStart(nsIDOMDocument *aDocument,
                                             nsAString& aStr)
{
  return NS_OK;
}

PRBool
nsHTMLContentSerializer::IsJavaScript(nsIAtom* aAttrNameAtom, const nsAString& aValueString)
{
  if (aAttrNameAtom == nsGkAtoms::href ||
      aAttrNameAtom == nsGkAtoms::src) {
    static const char kJavaScript[] = "javascript";
    PRInt32 pos = aValueString.FindChar(':');
    if (pos < (PRInt32)(sizeof kJavaScript - 1))
        return PR_FALSE;
    nsAutoString scheme(Substring(aValueString, 0, pos));
    scheme.StripWhitespace();
    if ((scheme.Length() == (sizeof kJavaScript - 1)) &&
        scheme.EqualsIgnoreCase(kJavaScript))
      return PR_TRUE;
    else
      return PR_FALSE;  
  }

  PRBool result = 
                 (aAttrNameAtom == nsGkAtoms::onblur)      || (aAttrNameAtom == nsGkAtoms::onchange)
              || (aAttrNameAtom == nsGkAtoms::onclick)     || (aAttrNameAtom == nsGkAtoms::ondblclick)
              || (aAttrNameAtom == nsGkAtoms::onfocus)     || (aAttrNameAtom == nsGkAtoms::onkeydown)
              || (aAttrNameAtom == nsGkAtoms::onkeypress)  || (aAttrNameAtom == nsGkAtoms::onkeyup)
              || (aAttrNameAtom == nsGkAtoms::onload)      || (aAttrNameAtom == nsGkAtoms::onmousedown)
              || (aAttrNameAtom == nsGkAtoms::onpageshow)  || (aAttrNameAtom == nsGkAtoms::onpagehide)
              || (aAttrNameAtom == nsGkAtoms::onmousemove) || (aAttrNameAtom == nsGkAtoms::onmouseout)
              || (aAttrNameAtom == nsGkAtoms::onmouseover) || (aAttrNameAtom == nsGkAtoms::onmouseup)
              || (aAttrNameAtom == nsGkAtoms::onreset)     || (aAttrNameAtom == nsGkAtoms::onselect)
              || (aAttrNameAtom == nsGkAtoms::onsubmit)    || (aAttrNameAtom == nsGkAtoms::onunload)
              || (aAttrNameAtom == nsGkAtoms::onabort)     || (aAttrNameAtom == nsGkAtoms::onerror)
              || (aAttrNameAtom == nsGkAtoms::onpaint)     || (aAttrNameAtom == nsGkAtoms::onresize)
              || (aAttrNameAtom == nsGkAtoms::onscroll)    || (aAttrNameAtom == nsGkAtoms::onbroadcast)
              || (aAttrNameAtom == nsGkAtoms::onclose)     || (aAttrNameAtom == nsGkAtoms::oncontextmenu)
              || (aAttrNameAtom == nsGkAtoms::oncommand)   || (aAttrNameAtom == nsGkAtoms::oncommandupdate)
              || (aAttrNameAtom == nsGkAtoms::ondragdrop)  || (aAttrNameAtom == nsGkAtoms::ondragenter)
              || (aAttrNameAtom == nsGkAtoms::ondragexit)  || (aAttrNameAtom == nsGkAtoms::ondraggesture)
              || (aAttrNameAtom == nsGkAtoms::ondragover)  || (aAttrNameAtom == nsGkAtoms::ondragstart)
              || (aAttrNameAtom == nsGkAtoms::ondragleave) || (aAttrNameAtom == nsGkAtoms::ondrop)
              || (aAttrNameAtom == nsGkAtoms::ondragend)   || (aAttrNameAtom == nsGkAtoms::ondrag)
              || (aAttrNameAtom == nsGkAtoms::oninput);
  return result;
}

nsresult 
nsHTMLContentSerializer::EscapeURI(const nsAString& aURI, nsAString& aEscapedURI)
{
  
  
  if (IsJavaScript(nsGkAtoms::href, aURI)) {
    aEscapedURI = aURI;
    return NS_OK;
  }

  
  
  
  
  nsCOMPtr<nsITextToSubURI> textToSubURI;
  nsAutoString uri(aURI); 
  nsresult rv = NS_OK;


  if (!mCharset.IsEmpty() && !IsASCII(uri)) {
    textToSubURI = do_GetService(NS_ITEXTTOSUBURI_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRInt32 start = 0;
  PRInt32 end;
  nsAutoString part;
  nsXPIDLCString escapedURI;
  aEscapedURI.Truncate(0);

  
  while ((end = uri.FindCharInSet("%#;/?:@&=+$,", start)) != -1) {
    part = Substring(aURI, start, (end-start));
    if (textToSubURI && !IsASCII(part)) {
      rv = textToSubURI->ConvertAndEscape(mCharset.get(), part.get(), getter_Copies(escapedURI));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      escapedURI.Adopt(nsEscape(NS_ConvertUTF16toUTF8(part).get(), url_Path));
    }
    AppendASCIItoUTF16(escapedURI, aEscapedURI);

    
    part = Substring(aURI, end, 1);
    aEscapedURI.Append(part);
    start = end + 1;
  }

  if (start < (PRInt32) aURI.Length()) {
    
    part = Substring(aURI, start, aURI.Length()-start);
    if (textToSubURI) {
      rv = textToSubURI->ConvertAndEscape(mCharset.get(), part.get(), getter_Copies(escapedURI));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      escapedURI.Adopt(nsEscape(NS_ConvertUTF16toUTF8(part).get(), url_Path));
    }
    AppendASCIItoUTF16(escapedURI, aEscapedURI);
  }

  return rv;
}

void 
nsHTMLContentSerializer::SerializeAttributes(nsIContent* aContent,
                                             nsIAtom* aTagName,
                                             nsAString& aStr)
{
  nsresult rv;
  PRUint32 index, count;
  nsAutoString nameStr, valueStr;

  count = aContent->GetAttrCount();

  NS_NAMED_LITERAL_STRING(_mozStr, "_moz");

  
  
  
  for (index = count; index > 0; ) {
    --index;
    const nsAttrName* name = aContent->GetAttrNameAt(index);
    PRInt32 namespaceID = name->NamespaceID();
    nsIAtom* attrName = name->LocalName();

    
    const char* sharedName;
    attrName->GetUTF8String(&sharedName);
    if ((('_' == *sharedName) || ('-' == *sharedName)) &&
        !nsCRT::strncmp(sharedName+1, kMozStr, PRUint32(sizeof(kMozStr)-1))) {
      continue;
    }
    aContent->GetAttr(namespaceID, attrName, valueStr);

    
    
    
    
    if (aTagName == nsGkAtoms::br && attrName == nsGkAtoms::type &&
        StringBeginsWith(valueStr, _mozStr)) {
      continue;
    }

    if (mIsCopying && mIsFirstChildOfOL && (aTagName == nsGkAtoms::li) && 
        (attrName == nsGkAtoms::value)){
      
      continue;
    }
    PRBool isJS = IsJavaScript(attrName, valueStr);
    
    if (((attrName == nsGkAtoms::href) || 
         (attrName == nsGkAtoms::src))) {
      
      if (mFlags & nsIDocumentEncoder::OutputAbsoluteLinks) {
        
        
        
        
        nsCOMPtr<nsIURI> uri = aContent->GetBaseURI();
        if (uri) {
          nsAutoString absURI;
          rv = NS_MakeAbsoluteURI(absURI, valueStr, uri);
          if (NS_SUCCEEDED(rv)) {
            valueStr = absURI;
          }
        }
      }
      
      nsAutoString tempURI(valueStr);
      if (!isJS && NS_FAILED(EscapeURI(tempURI, valueStr)))
        valueStr = tempURI;
    }

    attrName->ToString(nameStr);
    
    




    if (mDoFormat
        && (mColPos >= mMaxColumn
            || ((PRInt32)(mColPos + nameStr.Length() +
                          valueStr.Length() + 4) > mMaxColumn))) {
        aStr.Append(mLineBreak);
        mColPos = 0;
    }

    
    if (IsShorthandAttr(attrName, aTagName) && valueStr.IsEmpty()) {
      valueStr = nameStr;
    }
    SerializeAttr(EmptyString(), nameStr, valueStr, aStr, !isJS);
  }
}

NS_IMETHODIMP
nsHTMLContentSerializer::AppendElementStart(nsIDOMElement *aElement,
                                            nsIDOMElement *aOriginalElement,
                                            nsAString& aStr)
{
  NS_ENSURE_ARG(aElement);

  nsCOMPtr<nsIContent> content = do_QueryInterface(aElement);
  if (!content) return NS_ERROR_FAILURE;

  
  
  
  PRBool hasDirtyAttr = content->HasAttr(kNameSpaceID_None,
                                         nsGkAtoms::mozdirty);

  nsIAtom *name = content->Tag();

  if (name == nsGkAtoms::meta) {
    
    
    nsAutoString header;
    content->GetAttr(kNameSpaceID_None, nsGkAtoms::httpEquiv, header);
    if (header.LowerCaseEqualsLiteral("content-type")) {
      return NS_OK;
    }
  }

  if (name == nsGkAtoms::br && mPreLevel > 0
      && (mFlags & nsIDocumentEncoder::OutputNoFormattingInPre)) {
    AppendToString(mLineBreak, aStr);
    mMayIgnoreLineBreakSequence = PR_TRUE;
    mColPos = 0;
    return NS_OK;
  }

  if (name == nsGkAtoms::body) {
    mInBody = PR_TRUE;
  }

  if (LineBreakBeforeOpen(name, hasDirtyAttr)) {
    AppendToString(mLineBreak, aStr);
    mMayIgnoreLineBreakSequence = PR_TRUE;
    mColPos = 0;
    mAddSpace = PR_FALSE;
  }
  else if (mAddSpace) {
    AppendToString(PRUnichar(' '), aStr);
    mAddSpace = PR_FALSE;
  }
  else {
    MaybeAddNewline(aStr);
  }
  
  
  mAddNewline = PR_FALSE;

  StartIndentation(name, hasDirtyAttr, aStr);

  if (name == nsGkAtoms::pre ||
      name == nsGkAtoms::script ||
      name == nsGkAtoms::style) {
    mPreLevel++;
  }
  
  AppendToString(kLessThan, aStr);

  nsAutoString nameStr;
  name->ToString(nameStr);
  AppendToString(nameStr.get(), -1, aStr);

  
  
  if (mIsCopying && name == nsGkAtoms::ol){
    
    
    nsAutoString start;
    PRInt32 startAttrVal = 0;
    aElement->GetAttribute(NS_LITERAL_STRING("start"), start);
    if (!start.IsEmpty()){
      PRInt32 rv = 0;
      startAttrVal = start.ToInteger(&rv);
      
      
      
      if (NS_SUCCEEDED(rv))
        startAttrVal--; 
      else
        startAttrVal = 0;
    }
    olState* state = new olState(startAttrVal, PR_TRUE);
    if (state)
      mOLStateStack.AppendElement(state);
  }

  if (mIsCopying && name == nsGkAtoms::li) {
    mIsFirstChildOfOL = IsFirstChildOfOL(aOriginalElement);
    if (mIsFirstChildOfOL){
      
      SerializeLIValueAttribute(aElement, aStr);
    }
  }

  
  
  SerializeAttributes(content, name, aStr);

  AppendToString(kGreaterThan, aStr);

  if (LineBreakAfterOpen(name, hasDirtyAttr)) {
    AppendToString(mLineBreak, aStr);
    mMayIgnoreLineBreakSequence = PR_TRUE;
    mColPos = 0;
  }

  if (name == nsGkAtoms::script ||
      name == nsGkAtoms::style ||
      name == nsGkAtoms::noscript ||
      name == nsGkAtoms::noframes) {
    mInCDATA = PR_TRUE;
  }

  if (name == nsGkAtoms::head) {
    
    
    AppendToString(mLineBreak, aStr);
    AppendToString(NS_LITERAL_STRING("<meta http-equiv=\"content-type\""),
                   aStr);
    AppendToString(NS_LITERAL_STRING(" content=\"text/html; "), aStr);
    AppendToString(NS_ConvertASCIItoUTF16(mCharset), aStr);
    AppendToString(NS_LITERAL_STRING("\">"), aStr);
    AppendToString(mLineBreak, aStr);
  }  

  return NS_OK;
}
  
NS_IMETHODIMP 
nsHTMLContentSerializer::AppendElementEnd(nsIDOMElement *aElement,
                                          nsAString& aStr)
{
  NS_ENSURE_ARG(aElement);

  nsCOMPtr<nsIContent> content = do_QueryInterface(aElement);
  if (!content) return NS_ERROR_FAILURE;

  PRBool hasDirtyAttr = content->HasAttr(kNameSpaceID_None,
                                         nsGkAtoms::mozdirty);

  nsIAtom *name = content->Tag();

  if (name == nsGkAtoms::script) {
    nsCOMPtr<nsIScriptElement> script = do_QueryInterface(aElement);
    NS_ASSERTION(script, "What kind of weird script element is this?");

    if (script->IsMalformed()) {
      
      
      
      return NS_OK;
    }
  }

  if (name == nsGkAtoms::pre ||
      name == nsGkAtoms::script ||
      name == nsGkAtoms::style) {
    mPreLevel--;
  }

  if (mIsCopying && (name == nsGkAtoms::ol)){
    NS_ASSERTION((mOLStateStack.Count() > 0), "Cannot have an empty OL Stack");
    

    if (mOLStateStack.Count() > 0) {
      olState* state = (olState*)mOLStateStack.ElementAt(mOLStateStack.Count() -1);
      mOLStateStack.RemoveElementAt(mOLStateStack.Count() -1);
      delete state;
    }
  }
  
  nsIParserService* parserService = nsContentUtils::GetParserService();

  if (parserService && (name != nsGkAtoms::style)) {
    PRBool isContainer;

    parserService->IsContainer(parserService->HTMLAtomTagToId(name),
                               isContainer);
    if (!isContainer) return NS_OK;
  }

  if (LineBreakBeforeClose(name, hasDirtyAttr)) {
    AppendToString(mLineBreak, aStr);
    mMayIgnoreLineBreakSequence = PR_TRUE;
    mColPos = 0;
    mAddSpace = PR_FALSE;
  }
  else if (mAddSpace) {
    AppendToString(PRUnichar(' '), aStr);
    mAddSpace = PR_FALSE;
  }

  EndIndentation(name, hasDirtyAttr, aStr);

  nsAutoString nameStr;
  name->ToString(nameStr);

  AppendToString(kEndTag, aStr);
  AppendToString(nameStr.get(), -1, aStr);
  AppendToString(kGreaterThan, aStr);

  if (LineBreakAfterClose(name, hasDirtyAttr)) {
    AppendToString(mLineBreak, aStr);
    mMayIgnoreLineBreakSequence = PR_TRUE;
    mColPos = 0;
  }
  else {
    MaybeFlagNewline(aElement);
  }

  mInCDATA = PR_FALSE;

  return NS_OK;
}

void
nsHTMLContentSerializer::AppendToString(const PRUnichar* aStr,
                                        PRInt32 aLength,
                                        nsAString& aOutputStr)
{
  if (mBodyOnly && !mInBody) {
    return;
  }

  PRInt32 length = (aLength == -1) ? nsCRT::strlen(aStr) : aLength;
  
  mColPos += length;

  aOutputStr.Append(aStr, length);
}

void 
nsHTMLContentSerializer::AppendToString(const PRUnichar aChar,
                                        nsAString& aOutputStr)
{
  if (mBodyOnly && !mInBody) {
    return;
  }

  mColPos += 1;

  aOutputStr.Append(aChar);
}

static const PRUint16 kValNBSP = 160;
static const char kEntityNBSP[] = "nbsp";

static const PRUint16 kGTVal = 62;
static const char* kEntities[] = {
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "amp", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "lt", "", "gt"
};

static const char* kAttrEntities[] = {
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "quot", "", "", "", "amp", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "lt", "", "gt"
};

void
nsHTMLContentSerializer::AppendToString(const nsAString& aStr,
                                        nsAString& aOutputStr,
                                        PRBool aTranslateEntities,
                                        PRBool aIncrColumn)
{
  if (mBodyOnly && !mInBody) {
    return;
  }

  if (aIncrColumn) {
    mColPos += aStr.Length();
  }

  if (aTranslateEntities && !mInCDATA) {
    if (mFlags & (nsIDocumentEncoder::OutputEncodeBasicEntities  |
                  nsIDocumentEncoder::OutputEncodeLatin1Entities |
                  nsIDocumentEncoder::OutputEncodeHTMLEntities   |
                  nsIDocumentEncoder::OutputEncodeW3CEntities)) {
      nsIParserService* parserService = nsContentUtils::GetParserService();

      if (!parserService) {
        NS_ERROR("Can't get parser service");
        return;
      }

      nsReadingIterator<PRUnichar> done_reading;
      aStr.EndReading(done_reading);

      
      PRUint32 advanceLength = 0;
      nsReadingIterator<PRUnichar> iter;

      const char **entityTable = mInAttribute ? kAttrEntities : kEntities;

      for (aStr.BeginReading(iter); 
           iter != done_reading; 
           iter.advance(PRInt32(advanceLength))) {
        PRUint32 fragmentLength = iter.size_forward();
        PRUint32 lengthReplaced = 0; 
                                     
        const PRUnichar* c = iter.get();
        const PRUnichar* fragmentStart = c;
        const PRUnichar* fragmentEnd = c + fragmentLength;
        const char* entityText = nsnull;
        nsCAutoString entityReplacement;
        char* fullEntityText = nsnull;

        advanceLength = 0;
        
        
        for (; c < fragmentEnd; c++, advanceLength++) {
          PRUnichar val = *c;
          if (val == kValNBSP) {
            entityText = kEntityNBSP;
            break;
          }
          else if ((val <= kGTVal) && (entityTable[val][0] != 0)) {
            entityText = entityTable[val];
            break;
          } else if (val > 127 &&
                    ((val < 256 &&
                      mFlags & nsIDocumentEncoder::OutputEncodeLatin1Entities) ||
                      mFlags & nsIDocumentEncoder::OutputEncodeHTMLEntities)) {
            parserService->HTMLConvertUnicodeToEntity(val, entityReplacement);

            if (!entityReplacement.IsEmpty()) {
              entityText = entityReplacement.get();
              break;
            }
          }
          else if (val > 127 && 
                   mFlags & nsIDocumentEncoder::OutputEncodeW3CEntities &&
                   mEntityConverter) {
            if (NS_IS_HIGH_SURROGATE(val) &&
                c + 1 < fragmentEnd &&
                NS_IS_LOW_SURROGATE(*(c + 1))) {
              PRUint32 valUTF32 = SURROGATE_TO_UCS4(val, *(++c));
              if (NS_SUCCEEDED(mEntityConverter->ConvertUTF32ToEntity(valUTF32,
                               nsIEntityConverter::entityW3C, &fullEntityText))) {
                lengthReplaced = 2;
                break;
              }
              else {
                advanceLength++;
              }
            }
            else if (NS_SUCCEEDED(mEntityConverter->ConvertToEntity(val,
                                  nsIEntityConverter::entityW3C, 
                                  &fullEntityText))) {
              lengthReplaced = 1;
              break;
            }
          }
        }

        aOutputStr.Append(fragmentStart, advanceLength);
        if (entityText) {
          aOutputStr.Append(PRUnichar('&'));
          AppendASCIItoUTF16(entityText, aOutputStr);
          aOutputStr.Append(PRUnichar(';'));
          advanceLength++;
        }
        
        else if (fullEntityText) {
          AppendASCIItoUTF16(fullEntityText, aOutputStr);
          nsMemory::Free(fullEntityText);
          advanceLength += lengthReplaced;
        }
      }
    } else {
      nsXMLContentSerializer::AppendToString(aStr, aOutputStr, aTranslateEntities, aIncrColumn);
    }

    return;
  }

  aOutputStr.Append(aStr);
}

void
nsHTMLContentSerializer::AppendToStringConvertLF(const nsAString& aStr,
                                                 nsAString& aOutputStr)
{
  
  PRUint32 start = 0;
  PRUint32 theLen = aStr.Length();
  while (start < theLen) {
    PRInt32 eol = aStr.FindChar('\n', start);
    if (eol == kNotFound) {
      nsDependentSubstring dataSubstring(aStr, start, theLen - start);
      AppendToString(dataSubstring, aOutputStr);
      start = theLen;
    }
    else {
      nsDependentSubstring dataSubstring(aStr, start, eol - start);
      AppendToString(dataSubstring, aOutputStr);
      AppendToString(mLineBreak, aOutputStr);
      start = eol + 1;
      if (start == theLen)
        mColPos = 0;
    }
  }
}

PRBool
nsHTMLContentSerializer::LineBreakBeforeOpen(nsIAtom* aName, 
                                             PRBool aHasDirtyAttr)
{
  if ((!mDoFormat && !aHasDirtyAttr) || mPreLevel || !mColPos ||
      (mFlags & nsIDocumentEncoder::OutputRaw)) {
    return PR_FALSE;
  }
        
  if (aName == nsGkAtoms::title ||
      aName == nsGkAtoms::meta  ||
      aName == nsGkAtoms::link  ||
      aName == nsGkAtoms::style ||
      aName == nsGkAtoms::select ||
      aName == nsGkAtoms::option ||
      aName == nsGkAtoms::script ||
      aName == nsGkAtoms::html) {
    return PR_TRUE;
  }
  else {
    nsIParserService* parserService = nsContentUtils::GetParserService();
    
    if (parserService) {
      PRBool res;
      parserService->IsBlock(parserService->HTMLAtomTagToId(aName), res);
      return res;
    }
  }

  return PR_FALSE;
}

PRBool 
nsHTMLContentSerializer::LineBreakAfterOpen(nsIAtom* aName, 
                                            PRBool aHasDirtyAttr)
{
  if ((!mDoFormat && !aHasDirtyAttr) || mPreLevel ||
      (mFlags & nsIDocumentEncoder::OutputRaw)) {
    return PR_FALSE;
  }

  if ((aName == nsGkAtoms::html) ||
      (aName == nsGkAtoms::head) ||
      (aName == nsGkAtoms::body) ||
      (aName == nsGkAtoms::ul) ||
      (aName == nsGkAtoms::ol) ||
      (aName == nsGkAtoms::dl) ||
      (aName == nsGkAtoms::table) ||
      (aName == nsGkAtoms::tbody) ||
      (aName == nsGkAtoms::tr) ||
      (aName == nsGkAtoms::br) ||
      (aName == nsGkAtoms::meta) ||
      (aName == nsGkAtoms::link) ||
      (aName == nsGkAtoms::script) ||
      (aName == nsGkAtoms::select) ||
      (aName == nsGkAtoms::map) ||
      (aName == nsGkAtoms::area) ||
      (aName == nsGkAtoms::style)) {
    return PR_TRUE;
  }

  return PR_FALSE;
}

PRBool 
nsHTMLContentSerializer::LineBreakBeforeClose(nsIAtom* aName, 
                                              PRBool aHasDirtyAttr)
{
  if ((!mDoFormat && !aHasDirtyAttr) || mPreLevel || !mColPos ||
      (mFlags & nsIDocumentEncoder::OutputRaw)) {
    return PR_FALSE;
  }

  if ((aName == nsGkAtoms::html) ||
      (aName == nsGkAtoms::head) ||
      (aName == nsGkAtoms::body) ||
      (aName == nsGkAtoms::ul) ||
      (aName == nsGkAtoms::ol) ||
      (aName == nsGkAtoms::dl) ||
      (aName == nsGkAtoms::select) ||
      (aName == nsGkAtoms::table) ||
      (aName == nsGkAtoms::tbody)) {
    return PR_TRUE;
  }
  
  return PR_FALSE;
}

PRBool 
nsHTMLContentSerializer::LineBreakAfterClose(nsIAtom* aName, 
                                             PRBool aHasDirtyAttr)
{
  if ((!mDoFormat && !aHasDirtyAttr) || mPreLevel ||
      (mFlags & nsIDocumentEncoder::OutputRaw)) {
    return PR_FALSE;
  }

  if ((aName == nsGkAtoms::html) ||
      (aName == nsGkAtoms::head) ||
      (aName == nsGkAtoms::body) ||
      (aName == nsGkAtoms::tr) ||
      (aName == nsGkAtoms::th) ||
      (aName == nsGkAtoms::td) ||
      (aName == nsGkAtoms::pre) ||
      (aName == nsGkAtoms::title) ||
      (aName == nsGkAtoms::li) ||
      (aName == nsGkAtoms::dt) ||
      (aName == nsGkAtoms::dd) ||
      (aName == nsGkAtoms::blockquote) ||
      (aName == nsGkAtoms::select) ||
      (aName == nsGkAtoms::option) ||
      (aName == nsGkAtoms::p) ||
      (aName == nsGkAtoms::map) ||
      (aName == nsGkAtoms::div)) {
    return PR_TRUE;
  }
  else {
    nsIParserService* parserService = nsContentUtils::GetParserService();
    
    if (parserService) {
      PRBool res;
      parserService->IsBlock(parserService->HTMLAtomTagToId(aName), res);
      return res;
    }
  }

  return PR_FALSE;
}

void
nsHTMLContentSerializer::StartIndentation(nsIAtom* aName,
                                          PRBool aHasDirtyAttr,
                                          nsAString& aStr)
{
  if ((mDoFormat || aHasDirtyAttr) && !mPreLevel && !mColPos) {
    for (PRInt32 i = mIndent; --i >= 0; ) {
      AppendToString(kIndentStr, aStr);
    }
  }

  if ((aName == nsGkAtoms::head) ||
      (aName == nsGkAtoms::table) ||
      (aName == nsGkAtoms::tr) ||
      (aName == nsGkAtoms::ul) ||
      (aName == nsGkAtoms::ol) ||
      (aName == nsGkAtoms::dl) ||
      (aName == nsGkAtoms::tbody) ||
      (aName == nsGkAtoms::form) ||
      (aName == nsGkAtoms::frameset) ||
      (aName == nsGkAtoms::blockquote) ||
      (aName == nsGkAtoms::li) ||
      (aName == nsGkAtoms::dt) ||
      (aName == nsGkAtoms::dd)) {
    mIndent++;
  }
}

void
nsHTMLContentSerializer::EndIndentation(nsIAtom* aName,
                                        PRBool aHasDirtyAttr,
                                        nsAString& aStr)
{
  if ((aName == nsGkAtoms::head) ||
      (aName == nsGkAtoms::table) ||
      (aName == nsGkAtoms::tr) ||
      (aName == nsGkAtoms::ul) ||
      (aName == nsGkAtoms::ol) ||
      (aName == nsGkAtoms::dl) ||
      (aName == nsGkAtoms::li) ||
      (aName == nsGkAtoms::tbody) ||
      (aName == nsGkAtoms::form) ||
      (aName == nsGkAtoms::blockquote) ||
      (aName == nsGkAtoms::dt) ||
      (aName == nsGkAtoms::dd) ||
      (aName == nsGkAtoms::frameset)) {
    mIndent--;
  }

  if ((mDoFormat || aHasDirtyAttr) && !mPreLevel && !mColPos) {
    for (PRInt32 i = mIndent; --i >= 0; ) {
      AppendToString(kIndentStr, aStr);
    }
  }
}




PRBool 
nsHTMLContentSerializer::HasLongLines(const nsString& text, PRInt32& aLastNewlineOffset)
{
  PRUint32 start=0;
  PRUint32 theLen=text.Length();
  PRBool rv = PR_FALSE;
  aLastNewlineOffset = kNotFound;
  for (start = 0; start < theLen; )
  {
    PRInt32 eol = text.FindChar('\n', start);
    if (eol < 0) {
      eol = text.Length();
    }
    else {
      aLastNewlineOffset = eol;
    }
    if (PRInt32(eol - start) > kLongLineLen)
      rv = PR_TRUE;
    start = eol+1;
  }
  return rv;
}

void 
nsHTMLContentSerializer::SerializeLIValueAttribute(nsIDOMElement* aElement,
                                                   nsAString& aStr)
{
  
  
  
  nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aElement);
  PRBool found = PR_FALSE;
  nsIDOMNode* currNode = node;
  nsAutoString valueStr;
  PRInt32 offset = 0;
  olState defaultOLState(0, PR_FALSE);
  olState* state = nsnull;
  if (mOLStateStack.Count() > 0) 
    state = (olState*)mOLStateStack.ElementAt(mOLStateStack.Count()-1);
  


  if (!state || mOLStateStack.Count() == 0)
    state = &defaultOLState;
  PRInt32 startVal = state->startVal;
  state->isFirstListItem = PR_FALSE;
  
  
  while (currNode && !found) {
    nsCOMPtr<nsIDOMElement> currElement = do_QueryInterface(currNode);
    
    if (currElement) {
      nsAutoString tagName;
      currElement->GetTagName(tagName);
      if (tagName.LowerCaseEqualsLiteral("li")) {
        currElement->GetAttribute(NS_LITERAL_STRING("value"), valueStr);
        if (valueStr.IsEmpty())
          offset++;
        else {
          found = PR_TRUE;
          PRInt32 rv = 0;
          startVal = valueStr.ToInteger(&rv); 
        }
      }
    }
    currNode->GetPreviousSibling(&currNode);
  }
  
  
  if (offset == 0 && found) {
    
    
    SerializeAttr(EmptyString(), NS_LITERAL_STRING("value"), valueStr, aStr, PR_FALSE);
  }
  else if (offset == 1 && !found) {
    



     
  }
  else if (offset > 0) {
    
    nsAutoString valueStr;

    
    valueStr.AppendInt(startVal + offset);
    SerializeAttr(EmptyString(), NS_LITERAL_STRING("value"), valueStr, aStr, PR_FALSE);
  }
}

PRBool
nsHTMLContentSerializer::IsFirstChildOfOL(nsIDOMElement* aElement){
  nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aElement);
  nsAutoString parentName;
  {
    nsCOMPtr<nsIDOMNode> parentNode;
    node->GetParentNode(getter_AddRefs(parentNode));
    if (parentNode)
      parentNode->GetNodeName(parentName);
    else
      return PR_FALSE;
  }
  
  if (parentName.LowerCaseEqualsLiteral("ol")) {
    olState defaultOLState(0, PR_FALSE);
    olState* state = nsnull;
    if (mOLStateStack.Count() > 0) 
      state = (olState*)mOLStateStack.ElementAt(mOLStateStack.Count()-1);
    


    if (!state)
      state = &defaultOLState;
    
    if (state->isFirstListItem)
      return PR_TRUE;

    return PR_FALSE;
  }
  else
    return PR_FALSE;
}

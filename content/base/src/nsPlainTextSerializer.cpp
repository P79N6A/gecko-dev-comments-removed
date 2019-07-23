












































#include "nsPlainTextSerializer.h"
#include "nsLWBrkCIID.h"
#include "nsIServiceManager.h"
#include "nsGkAtoms.h"
#include "nsIDOMText.h"
#include "nsIDOMCDATASection.h"
#include "nsIDOMElement.h"
#include "nsINameSpaceManager.h"
#include "nsTextFragment.h"
#include "nsContentUtils.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsCRT.h"
#include "nsIParserService.h"

#define PREF_STRUCTS "converter.html2txt.structs"
#define PREF_HEADER_STRATEGY "converter.html2txt.header_strategy"

static const  PRInt32 kTabSize=4;
static const  PRInt32 kOLNumberWidth = 3;
static const  PRInt32 kIndentSizeHeaders = 2;  




static const  PRInt32 kIndentIncrementHeaders = 2;  


static const  PRInt32 kIndentSizeList = (kTabSize > kOLNumberWidth+3) ? kTabSize: kOLNumberWidth+3;
                               
static const  PRInt32 kIndentSizeDD = kTabSize;  
static const  PRUnichar  kNBSP = 160;
static const  PRUnichar kSPACE = ' ';

static PRInt32 HeaderLevel(eHTMLTags aTag);
static PRInt32 GetUnicharWidth(PRUnichar ucs);
static PRInt32 GetUnicharStringWidth(const PRUnichar* pwcs, PRInt32 n);


static const PRUint32 TagStackSize = 500;
static const PRUint32 OLStackSize = 100;

nsresult NS_NewPlainTextSerializer(nsIContentSerializer** aSerializer)
{
  nsPlainTextSerializer* it = new nsPlainTextSerializer();
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return CallQueryInterface(it, aSerializer);
}

nsPlainTextSerializer::nsPlainTextSerializer()
  : kSpace(NS_LITERAL_STRING(" ")) 
{

  mOutputString = nsnull;
  mHeadLevel = 0;
  mAtFirstColumn = PR_TRUE;
  mIndent = 0;
  mCiteQuoteLevel = 0;
  mStructs = PR_TRUE;       
  mHeaderStrategy = 1 ;   
  mQuotesPreformatted = PR_FALSE;                
  mDontWrapAnyQuotes = PR_FALSE;                 
  mHasWrittenCiteBlockquote = PR_FALSE;
  mSpanLevel = 0;
  for (PRInt32 i = 0; i <= 6; i++) {
    mHeaderCounter[i] = 0;
  }

  
  mWrapColumn = 72;     
  mCurrentLineWidth = 0;

  
  mEmptyLines = 1; 
  mInWhitespace = PR_TRUE;
  mPreFormatted = PR_FALSE;
  mStartedOutput = PR_FALSE;

  
  mTagStack = new nsHTMLTag[TagStackSize];
  mTagStackIndex = 0;
  mIgnoreAboveIndex = (PRUint32)kNotFound;

  
  mOLStack = new PRInt32[OLStackSize];
  mOLStackIndex = 0;

  mULCount = 0;
}

nsPlainTextSerializer::~nsPlainTextSerializer()
{
  delete[] mTagStack;
  delete[] mOLStack;
  NS_WARN_IF_FALSE(mHeadLevel == 0, "Wrong head level!");
}

NS_IMPL_ISUPPORTS4(nsPlainTextSerializer, 
                   nsIContentSerializer,
                   nsIContentSink,
                   nsIHTMLContentSink,
                   nsIHTMLToTextSink)


NS_IMETHODIMP 
nsPlainTextSerializer::Init(PRUint32 aFlags, PRUint32 aWrapColumn,
                            const char* aCharSet, PRBool aIsCopying,
                            PRBool aIsWholeDocument)
{
#ifdef DEBUG
  
  if(aFlags & nsIDocumentEncoder::OutputFormatFlowed) {
    NS_ASSERTION(aFlags & nsIDocumentEncoder::OutputFormatted,
                 "If you want format=flowed, you must combine it with "
                 "nsIDocumentEncoder::OutputFormatted");
  }

  if(aFlags & nsIDocumentEncoder::OutputFormatted) {
    NS_ASSERTION(!(aFlags & nsIDocumentEncoder::OutputPreformatted),
                 "Can't do formatted and preformatted output at the same time!");
  }
#endif

  NS_ENSURE_TRUE(nsContentUtils::GetParserService(), NS_ERROR_UNEXPECTED);

  mFlags = aFlags;
  mWrapColumn = aWrapColumn;

  
  if (MayWrap()) {
    mLineBreaker = nsContentUtils::LineBreaker();
  }

  
  if ((mFlags & nsIDocumentEncoder::OutputCRLineBreak)
      && (mFlags & nsIDocumentEncoder::OutputLFLineBreak)) {
    
    mLineBreak.AssignLiteral("\r\n");
  }
  else if (mFlags & nsIDocumentEncoder::OutputCRLineBreak) {
    
    mLineBreak.Assign(PRUnichar('\r'));
  }
  else if (mFlags & nsIDocumentEncoder::OutputLFLineBreak) {
    
    mLineBreak.Assign(PRUnichar('\n'));
  }
  else {
    
    mLineBreak.AssignLiteral(NS_LINEBREAK);
  }

  mLineBreakDue = PR_FALSE;
  mFloatingLines = -1;

  if (mFlags & nsIDocumentEncoder::OutputFormatted) {
    
    mStructs = nsContentUtils::GetBoolPref(PREF_STRUCTS, mStructs);

    mHeaderStrategy =
      nsContentUtils::GetIntPref(PREF_HEADER_STRATEGY, mHeaderStrategy);

    
    mQuotesPreformatted =
      nsContentUtils::GetBoolPref("editor.quotesPreformatted",
                                  mQuotesPreformatted);

    
    
    
    if (mFlags & nsIDocumentEncoder::OutputWrap || mWrapColumn > 0) {
      mDontWrapAnyQuotes =
        nsContentUtils::GetBoolPref("mail.compose.wrap_to_window_width",
                                    mDontWrapAnyQuotes);
    }
  }

  
  if (nsContentUtils::GetBoolPref("browser.frames.enabled")) {
    mFlags &= ~nsIDocumentEncoder::OutputNoFramesContent;
  }
  else {
    mFlags |= nsIDocumentEncoder::OutputNoFramesContent;
  }

  return NS_OK;
}

PRBool
nsPlainTextSerializer::GetLastBool(const nsTArray<PRPackedBool>& aStack)
{
  PRUint32 size = aStack.Length();
  if (size == 0) {
    return PR_FALSE;
  }
  return aStack.ElementAt(size-1);
}

void
nsPlainTextSerializer::SetLastBool(nsTArray<PRPackedBool>& aStack, PRBool aValue)
{
  PRUint32 size = aStack.Length();
  if (size > 0) {
    aStack.ElementAt(size-1) = aValue;
  }
  else {
    NS_ERROR("There is no \"Last\" value");
  }
}

void
nsPlainTextSerializer::PushBool(nsTArray<PRPackedBool>& aStack, PRBool aValue)
{
    aStack.AppendElement(PRPackedBool(aValue));
}

PRBool
nsPlainTextSerializer::PopBool(nsTArray<PRPackedBool>& aStack)
{
  PRBool returnValue = PR_FALSE;
  PRUint32 size = aStack.Length();
  if (size > 0) {
    returnValue = aStack.ElementAt(size-1);
    aStack.RemoveElementAt(size-1);
  }
  return returnValue;
}

NS_IMETHODIMP
nsPlainTextSerializer::Initialize(nsAString* aOutString,
                                  PRUint32 aFlags, PRUint32 aWrapCol)
{
  nsresult rv = Init(aFlags, aWrapCol, nsnull, PR_FALSE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  mOutputString = aOutString;

  return NS_OK;
}

NS_IMETHODIMP 
nsPlainTextSerializer::AppendText(nsIDOMText* aText, 
                                  PRInt32 aStartOffset,
                                  PRInt32 aEndOffset, 
                                  nsAString& aStr)
{
  if (mIgnoreAboveIndex != (PRUint32)kNotFound) {
    return NS_OK;
  }
    
  NS_ASSERTION(aStartOffset >= 0, "Negative start offset for text fragment!");
  if ( aStartOffset < 0 )
    return NS_ERROR_INVALID_ARG;

  NS_ENSURE_ARG(aText);

  nsresult rv = NS_OK;
  PRInt32 length = 0;
  nsAutoString textstr;

  nsCOMPtr<nsIContent> content = do_QueryInterface(aText);
  const nsTextFragment* frag;
  if (!content || !(frag = content->GetText())) {
    return NS_ERROR_FAILURE;
  }
  
  PRInt32 endoffset = (aEndOffset == -1) ? frag->GetLength() : aEndOffset;
  NS_ASSERTION(aStartOffset <= endoffset, "A start offset is beyond the end of the text fragment!");

  length = endoffset - aStartOffset;
  if (length <= 0) {
    return NS_OK;
  }

  if (frag->Is2b()) {
    textstr.Assign(frag->Get2b() + aStartOffset, length);
  }
  else {
    textstr.AssignWithConversion(frag->Get1b()+aStartOffset, length);
  }

  mOutputString = &aStr;

  
  
  PRInt32 start = 0;
  PRInt32 offset = textstr.FindCharInSet("\n\r");
  while (offset != kNotFound) {

    if(offset>start) {
      
      rv = DoAddLeaf(nsnull,
                     eHTMLTag_text,
                     Substring(textstr, start, offset-start));
      if (NS_FAILED(rv)) break;
    }

    
    rv = DoAddLeaf(nsnull, eHTMLTag_newline, mLineBreak);
    if (NS_FAILED(rv)) break;
    
    start = offset+1;
    offset = textstr.FindCharInSet("\n\r", start);
  }

  
  if (NS_SUCCEEDED(rv) && start < length) {
    if (start) {
      rv = DoAddLeaf(nsnull,
                     eHTMLTag_text,
                     Substring(textstr, start, length-start));
    }
    else {
      rv = DoAddLeaf(nsnull, eHTMLTag_text, textstr);
    }
  }
  
  mOutputString = nsnull;

  return rv;
}

NS_IMETHODIMP
nsPlainTextSerializer::AppendCDATASection(nsIDOMCDATASection* aCDATASection,
                                          PRInt32 aStartOffset,
                                          PRInt32 aEndOffset,
                                          nsAString& aStr)
{
  return AppendText(aCDATASection, aStartOffset, aEndOffset, aStr);
}

NS_IMETHODIMP
nsPlainTextSerializer::AppendElementStart(nsIDOMElement *aElement,
                                          nsIDOMElement *aOriginalElement,
                                          nsAString& aStr)
{
  NS_ENSURE_ARG(aElement);

  mContent = do_QueryInterface(aElement);
  if (!mContent) return NS_ERROR_FAILURE;

  nsresult rv;
  PRInt32 id = GetIdForContent(mContent);

  PRBool isContainer = IsContainer(id);

  mOutputString = &aStr;

  if (isContainer) {
    rv = DoOpenContainer(nsnull, id);
  }
  else {
    rv = DoAddLeaf(nsnull, id, EmptyString());
  }

  mContent = 0;
  mOutputString = nsnull;

  if (id == eHTMLTag_head) {
    ++mHeadLevel;
  }

  return rv;
} 
 
NS_IMETHODIMP 
nsPlainTextSerializer::AppendElementEnd(nsIDOMElement *aElement,
                                        nsAString& aStr)
{
  NS_ENSURE_ARG(aElement);

  mContent = do_QueryInterface(aElement);
  if (!mContent) return NS_ERROR_FAILURE;

  nsresult rv;
  PRInt32 id = GetIdForContent(mContent);

  PRBool isContainer = IsContainer(id);

  mOutputString = &aStr;

  rv = NS_OK;
  if (isContainer) {
    rv = DoCloseContainer(id);
  }

  mContent = 0;
  mOutputString = nsnull;

  if (id == eHTMLTag_head) {
    --mHeadLevel;
    NS_ASSERTION(mHeadLevel >= 0, "mHeadLevel < 0");
  }

  return rv;
}

NS_IMETHODIMP 
nsPlainTextSerializer::Flush(nsAString& aStr)
{
  mOutputString = &aStr;
  FlushLine();
  mOutputString = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsPlainTextSerializer::AppendDocumentStart(nsIDOMDocument *aDocument,
                                             nsAString& aStr)
{
  return NS_OK;
}

NS_IMETHODIMP
nsPlainTextSerializer::OpenContainer(const nsIParserNode& aNode)
{
  PRInt32 type = aNode.GetNodeType();

  if (type == eHTMLTag_head) {
    ++mHeadLevel;
    return NS_OK;
  }

  return DoOpenContainer(&aNode, type);
}

NS_IMETHODIMP 
nsPlainTextSerializer::CloseContainer(const nsHTMLTag aTag)
{
  if (aTag == eHTMLTag_head) {
    --mHeadLevel;
    NS_ASSERTION(mHeadLevel >= 0, "mHeadLevel < 0");
    return NS_OK;
  }

  return DoCloseContainer(aTag);
}

NS_IMETHODIMP 
nsPlainTextSerializer::AddLeaf(const nsIParserNode& aNode)
{
  if (mIgnoreAboveIndex != (PRUint32)kNotFound) {
    return NS_OK;
  }

  eHTMLTags type = (eHTMLTags)aNode.GetNodeType();
  const nsAString& text = aNode.GetText();

  if ((type == eHTMLTag_text) ||
      (type == eHTMLTag_whitespace) ||
      (type == eHTMLTag_newline)) {
    
    nsAutoString str;
    PRUint32 length;
    str.SetCapacity(text.Length());
    nsReadingIterator<PRUnichar> srcStart, srcEnd;
    length = nsContentUtils::CopyNewlineNormalizedUnicodeTo(text.BeginReading(srcStart), text.EndReading(srcEnd), str);
    str.SetLength(length);
    return DoAddLeaf(&aNode, type, str);
  }
  else {
    return DoAddLeaf(&aNode, type, text);
  }
}

NS_IMETHODIMP 
nsPlainTextSerializer::OpenHead()
{
  ++mHeadLevel;
  return NS_OK;
}

NS_IMETHODIMP
nsPlainTextSerializer::IsEnabled(PRInt32 aTag, PRBool* aReturn)
{
  nsHTMLTag theHTMLTag = nsHTMLTag(aTag);

  if (theHTMLTag == eHTMLTag_script) {
    *aReturn = !(mFlags & nsIDocumentEncoder::OutputNoScriptContent);
  }
  else if (theHTMLTag == eHTMLTag_frameset) {
    *aReturn = !(mFlags & nsIDocumentEncoder::OutputNoFramesContent);
  }
  else {
    *aReturn = PR_FALSE;
  }

  return NS_OK;
}





nsresult
nsPlainTextSerializer::DoOpenContainer(const nsIParserNode* aNode, PRInt32 aTag)
{
  if (mFlags & nsIDocumentEncoder::OutputRaw) {
    
    
    
    

    return NS_OK;
  }

  eHTMLTags type = (eHTMLTags)aTag;

  if (mTagStackIndex < TagStackSize) {
    mTagStack[mTagStackIndex++] = type;
  }

  if (mIgnoreAboveIndex != (PRUint32)kNotFound) {
    return NS_OK;
  }

  
  
  mHasWrittenCiteBlockquote = mHasWrittenCiteBlockquote && aTag == eHTMLTag_pre;

  PRBool isInCiteBlockquote = PR_FALSE;

  
  
  if (aTag == eHTMLTag_blockquote) {
    nsAutoString value;
    nsresult rv = GetAttributeValue(aNode, nsGkAtoms::type, value);
    isInCiteBlockquote = NS_SUCCEEDED(rv) && value.EqualsIgnoreCase("cite");
  }

  if (mLineBreakDue && !isInCiteBlockquote)
    EnsureVerticalSpace(mFloatingLines);

  
  if ((type == eHTMLTag_noscript &&
       !(mFlags & nsIDocumentEncoder::OutputNoScriptContent)) ||
      ((type == eHTMLTag_iframe || type == eHTMLTag_noframes) &&
       !(mFlags & nsIDocumentEncoder::OutputNoFramesContent))) {
    
    
    mIgnoreAboveIndex = mTagStackIndex - 1;
    return NS_OK;
  }

  if (type == eHTMLTag_body) {
    
    
    
    
    
    
    
    
    nsAutoString style;
    PRInt32 whitespace;
    if(NS_SUCCEEDED(GetAttributeValue(aNode, nsGkAtoms::style, style)) &&
       (kNotFound != (whitespace = style.Find("white-space:")))) {

      if (kNotFound != style.Find("pre-wrap", PR_TRUE, whitespace)) {
#ifdef DEBUG_preformatted
        printf("Set mPreFormatted based on style pre-wrap\n");
#endif
        mPreFormatted = PR_TRUE;
        PRInt32 widthOffset = style.Find("width:");
        if (widthOffset >= 0) {
          
          
          
          
          
          PRInt32 semiOffset = style.Find("ch", PR_FALSE, widthOffset+6);
          PRInt32 length = (semiOffset > 0 ? semiOffset - widthOffset - 6
                            : style.Length() - widthOffset);
          nsAutoString widthstr;
          style.Mid(widthstr, widthOffset+6, length);
          PRInt32 err;
          PRInt32 col = widthstr.ToInteger(&err);

          if (NS_SUCCEEDED(err)) {
            mWrapColumn = (PRUint32)col;
#ifdef DEBUG_preformatted
            printf("Set wrap column to %d based on style\n", mWrapColumn);
#endif
          }
        }
      }
      else if (kNotFound != style.Find("pre", PR_TRUE, whitespace)) {
#ifdef DEBUG_preformatted
        printf("Set mPreFormatted based on style pre\n");
#endif
        mPreFormatted = PR_TRUE;
        mWrapColumn = 0;
      }
    } 
    else {
      mPreFormatted = PR_FALSE;
    }

    return NS_OK;
  }

  
  if (!DoOutput()) {
    return NS_OK;
  }

  if (type == eHTMLTag_p)
    EnsureVerticalSpace(1);
  else if (type == eHTMLTag_pre) {
    if (GetLastBool(mIsInCiteBlockquote))
      EnsureVerticalSpace(0);
    else if (mHasWrittenCiteBlockquote) {
      EnsureVerticalSpace(0);
      mHasWrittenCiteBlockquote = PR_FALSE;
    }
    else
      EnsureVerticalSpace(1);
  }
  else if (type == eHTMLTag_tr) {
    PushBool(mHasWrittenCellsForRow, PR_FALSE);
  }
  else if (type == eHTMLTag_td || type == eHTMLTag_th) {
    
    

    
    
    if (GetLastBool(mHasWrittenCellsForRow)) {
      
      AddToLine(NS_LITERAL_STRING("\t").get(), 1);
      mInWhitespace = PR_TRUE;
    }
    else if (mHasWrittenCellsForRow.IsEmpty()) {
      
      
      PushBool(mHasWrittenCellsForRow, PR_TRUE); 
    }
    else {
      SetLastBool(mHasWrittenCellsForRow, PR_TRUE);
    }
  }
  else if (type == eHTMLTag_ul) {
    
    EnsureVerticalSpace(mULCount + mOLStackIndex == 0 ? 1 : 0);
         
    mIndent += kIndentSizeList;
    mULCount++;
  }
  else if (type == eHTMLTag_ol) {
    EnsureVerticalSpace(mULCount + mOLStackIndex == 0 ? 1 : 0);
    
    if (mOLStackIndex < OLStackSize) {
      nsAutoString startAttr;
      PRInt32 startVal = 1;
      if(NS_SUCCEEDED(GetAttributeValue(aNode, nsGkAtoms::start, startAttr))){
        PRInt32 rv = 0;
        startVal = startAttr.ToInteger(&rv);
        if (NS_FAILED(rv))
          startVal = 1;
      }
      mOLStack[mOLStackIndex++] = startVal;
    }
    mIndent += kIndentSizeList;  
  }
  else if (type == eHTMLTag_li) {
    if (mTagStackIndex > 1 && IsInOL()) {
      if (mOLStackIndex > 0) {
        nsAutoString valueAttr;
        if(NS_SUCCEEDED(GetAttributeValue(aNode, nsGkAtoms::value, valueAttr))){
          PRInt32 rv = 0;
          PRInt32 valueAttrVal = valueAttr.ToInteger(&rv);
          if (NS_SUCCEEDED(rv))
            mOLStack[mOLStackIndex-1] = valueAttrVal;
        }
        
        mInIndentString.AppendInt(mOLStack[mOLStackIndex-1]++, 10);
      }
      else {
        mInIndentString.Append(PRUnichar('#'));
      }

      mInIndentString.Append(PRUnichar('.'));

    }
    else {
      static char bulletCharArray[] = "*o+#";
      PRUint32 index = mULCount > 0 ? (mULCount - 1) : 3;
      char bulletChar = bulletCharArray[index % 4];
      mInIndentString.Append(PRUnichar(bulletChar));
    }
    
    mInIndentString.Append(PRUnichar(' '));
  }
  else if (type == eHTMLTag_dl) {
    EnsureVerticalSpace(1);
  }
  else if (type == eHTMLTag_dt) {
    EnsureVerticalSpace(0);
  }
  else if (type == eHTMLTag_dd) {
    EnsureVerticalSpace(0);
    mIndent += kIndentSizeDD;
  }
  else if (type == eHTMLTag_span) {
    ++mSpanLevel;
  }
  else if (type == eHTMLTag_blockquote) {
    
    PushBool(mIsInCiteBlockquote, isInCiteBlockquote);
    if (isInCiteBlockquote) {
      EnsureVerticalSpace(0);
      mCiteQuoteLevel++;
    }
    else {
      EnsureVerticalSpace(1);
      mIndent += kTabSize; 
    }
  }
  else if (type == eHTMLTag_q) {
    Write(NS_LITERAL_STRING("\""));
  }

  
  
  else if (IsBlockLevel(aTag)) {
    EnsureVerticalSpace(0);
  }

  
  if (!(mFlags & nsIDocumentEncoder::OutputFormatted)) {
    return NS_OK;
  }
  
  
  
  

  
  PRBool currentNodeIsConverted = IsCurrentNodeConverted(aNode);
  PushBool(mCurrentNodeIsConverted, currentNodeIsConverted);

  if (type == eHTMLTag_h1 || type == eHTMLTag_h2 ||
      type == eHTMLTag_h3 || type == eHTMLTag_h4 ||
      type == eHTMLTag_h5 || type == eHTMLTag_h6)
  {
    EnsureVerticalSpace(2);
    if (mHeaderStrategy == 2) {  
      mIndent += kIndentSizeHeaders;
      
      PRInt32 level = HeaderLevel(type);
      
      mHeaderCounter[level]++;
      
      PRInt32 i;

      for (i = level + 1; i <= 6; i++) {
        mHeaderCounter[i] = 0;
      }

      
      nsAutoString leadup;
      for (i = 1; i <= level; i++) {
        leadup.AppendInt(mHeaderCounter[i]);
        leadup.Append(PRUnichar('.'));
      }
      leadup.Append(PRUnichar(' '));
      Write(leadup);
    }
    else if (mHeaderStrategy == 1) { 
      mIndent += kIndentSizeHeaders;
      for (PRInt32 i = HeaderLevel(type); i > 1; i--) {
           
        mIndent += kIndentIncrementHeaders;
      }
    }
  }
  else if (type == eHTMLTag_a && !currentNodeIsConverted) {
    nsAutoString url;
    if (NS_SUCCEEDED(GetAttributeValue(aNode, nsGkAtoms::href, url))
        && !url.IsEmpty()) {
      mURL = url;
    }
  }
  else if (type == eHTMLTag_sup && mStructs && !currentNodeIsConverted) {
    Write(NS_LITERAL_STRING("^"));
  }
  else if (type == eHTMLTag_sub && mStructs && !currentNodeIsConverted) { 
    Write(NS_LITERAL_STRING("_"));
  }
  else if (type == eHTMLTag_code && mStructs && !currentNodeIsConverted) {
    Write(NS_LITERAL_STRING("|"));
  }
  else if ((type == eHTMLTag_strong || type == eHTMLTag_b)
           && mStructs && !currentNodeIsConverted) {
    Write(NS_LITERAL_STRING("*"));
  }
  else if ((type == eHTMLTag_em || type == eHTMLTag_i)
           && mStructs && !currentNodeIsConverted) {
    Write(NS_LITERAL_STRING("/"));
  }
  else if (type == eHTMLTag_u && mStructs && !currentNodeIsConverted) {
    Write(NS_LITERAL_STRING("_"));
  }

  return NS_OK;
}

nsresult
nsPlainTextSerializer::DoCloseContainer(PRInt32 aTag)
{
  if (mFlags & nsIDocumentEncoder::OutputRaw) {
    
    
    
    

    return NS_OK;
  }

  if (mTagStackIndex > 0) {
    --mTagStackIndex;
  }

  if (mTagStackIndex >= mIgnoreAboveIndex) {
    if (mTagStackIndex == mIgnoreAboveIndex) {
      
      
      
      mIgnoreAboveIndex = (PRUint32)kNotFound;
    }
    return NS_OK;
  }

  eHTMLTags type = (eHTMLTags)aTag;
  
  if((type == eHTMLTag_body) || (type == eHTMLTag_html)) {
    
    
    
    
    if (mFlags & nsIDocumentEncoder::OutputFormatted) {
      EnsureVerticalSpace(0);
    }
    else {
      FlushLine();
    }
    
    
    return NS_OK;
  }

  
  if (!DoOutput()) {
    return NS_OK;
  }

  if (type == eHTMLTag_tr) {
    PopBool(mHasWrittenCellsForRow);
    
    if (mFloatingLines < 0)
      mFloatingLines = 0;
    mLineBreakDue = PR_TRUE;
  } 
  else if ((type == eHTMLTag_li) ||
           (type == eHTMLTag_dt)) {
    
    if (mFloatingLines < 0)
      mFloatingLines = 0;
    mLineBreakDue = PR_TRUE;
  } 
  else if (type == eHTMLTag_pre) {
    mFloatingLines = GetLastBool(mIsInCiteBlockquote) ? 0 : 1;
    mLineBreakDue = PR_TRUE;
  }
  else if (type == eHTMLTag_ul) {
    FlushLine();
    mIndent -= kIndentSizeList;
    if (--mULCount + mOLStackIndex == 0) {
      mFloatingLines = 1;
      mLineBreakDue = PR_TRUE;
    }
  }
  else if (type == eHTMLTag_ol) {
    FlushLine(); 
    mIndent -= kIndentSizeList;
    NS_ASSERTION(mOLStackIndex, "Wrong OLStack level!");
    mOLStackIndex--;
    if (mULCount + mOLStackIndex == 0) {
      mFloatingLines = 1;
      mLineBreakDue = PR_TRUE;
    }
  }  
  else if (type == eHTMLTag_dl) {
    mFloatingLines = 1;
    mLineBreakDue = PR_TRUE;
  }
  else if (type == eHTMLTag_dd) {
    FlushLine();
    mIndent -= kIndentSizeDD;
  }
  else if (type == eHTMLTag_span) {
    NS_ASSERTION(mSpanLevel, "Span level will be negative!");
    --mSpanLevel;
  }
  else if (type == eHTMLTag_div) {
    if (mFloatingLines < 0)
      mFloatingLines = 0;
    mLineBreakDue = PR_TRUE;
  }
  else if (type == eHTMLTag_blockquote) {
    FlushLine();    

    
    PRBool isInCiteBlockquote = PopBool(mIsInCiteBlockquote);

    if (isInCiteBlockquote) {
      NS_ASSERTION(mCiteQuoteLevel, "CiteQuote level will be negative!");
      mCiteQuoteLevel--;
      mFloatingLines = 0;
      mHasWrittenCiteBlockquote = PR_TRUE;
    }
    else {
      mIndent -= kTabSize;
      mFloatingLines = 1;
    }
    mLineBreakDue = PR_TRUE;
  }
  else if (type == eHTMLTag_q) {
    Write(NS_LITERAL_STRING("\""));
  }
  else if (IsBlockLevel(aTag)
           && type != eHTMLTag_script
           && type != eHTMLTag_doctypeDecl
           && type != eHTMLTag_markupDecl) {
    
    
    
    
    if (mFlags & nsIDocumentEncoder::OutputFormatted)
      EnsureVerticalSpace(1);
    else {
      if (mFloatingLines < 0)
        mFloatingLines = 0;
      mLineBreakDue = PR_TRUE;
    }
  }

  
  if (!(mFlags & nsIDocumentEncoder::OutputFormatted)) {
    return NS_OK;
  }
  
  
  
  

  
  PRBool currentNodeIsConverted = PopBool(mCurrentNodeIsConverted);
  
  if (type == eHTMLTag_h1 || type == eHTMLTag_h2 ||
      type == eHTMLTag_h3 || type == eHTMLTag_h4 ||
      type == eHTMLTag_h5 || type == eHTMLTag_h6) {
    
    if (mHeaderStrategy) {   
      mIndent -= kIndentSizeHeaders;
    }
    if (mHeaderStrategy == 1  ) {
      for (PRInt32 i = HeaderLevel(type); i > 1; i--) {
           
        mIndent -= kIndentIncrementHeaders;
      }
    }
    EnsureVerticalSpace(1);
  }
  else if (type == eHTMLTag_a && !currentNodeIsConverted && !mURL.IsEmpty()) {
    nsAutoString temp; 
    temp.AssignLiteral(" <");
    temp += mURL;
    temp.Append(PRUnichar('>'));
    Write(temp);
    mURL.Truncate();
  }
  else if ((type == eHTMLTag_sup || type == eHTMLTag_sub) 
           && mStructs && !currentNodeIsConverted) {
    Write(kSpace);
  }
  else if (type == eHTMLTag_code && mStructs && !currentNodeIsConverted) {
    Write(NS_LITERAL_STRING("|"));
  }
  else if ((type == eHTMLTag_strong || type == eHTMLTag_b)
           && mStructs && !currentNodeIsConverted) {
    Write(NS_LITERAL_STRING("*"));
  }
  else if ((type == eHTMLTag_em || type == eHTMLTag_i)
           && mStructs && !currentNodeIsConverted) {
    Write(NS_LITERAL_STRING("/"));
  }
  else if (type == eHTMLTag_u && mStructs && !currentNodeIsConverted) {
    Write(NS_LITERAL_STRING("_"));
  }

  return NS_OK;
}





nsresult
nsPlainTextSerializer::DoAddLeaf(const nsIParserNode *aNode, PRInt32 aTag, 
                                 const nsAString& aText)
{
  
  if (!DoOutput()) {
    return NS_OK;
  }

  if (aTag != eHTMLTag_whitespace && aTag != eHTMLTag_newline) {
    
    mHasWrittenCiteBlockquote = PR_FALSE;
  }
  
  if (mLineBreakDue)
    EnsureVerticalSpace(mFloatingLines);

  eHTMLTags type = (eHTMLTags)aTag;
  
  if ((mTagStackIndex > 1 &&
       mTagStack[mTagStackIndex-2] == eHTMLTag_select) ||
      (mTagStackIndex > 0 &&
        mTagStack[mTagStackIndex-1] == eHTMLTag_select)) {
    
    
    
    return NS_OK;
  }
  else if (mTagStackIndex > 0 &&
           (mTagStack[mTagStackIndex-1] == eHTMLTag_script ||
            mTagStack[mTagStackIndex-1] == eHTMLTag_style)) {
    
    return NS_OK;
  }
  else if (type == eHTMLTag_text) {
    


    if (!mURL.IsEmpty() && mURL.Equals(aText)) {
      mURL.Truncate();
    }
    Write(aText);
  }
  else if (type == eHTMLTag_entity) {
    nsIParserService* parserService = nsContentUtils::GetParserService();
    if (parserService) {
      nsAutoString str(aText);
      PRInt32 entity;
      parserService->HTMLConvertEntityToUnicode(str, &entity);
      if (entity == -1 && 
          !str.IsEmpty() &&
          str.First() == (PRUnichar) '#') {
        PRInt32 err = 0;
        entity = str.ToInteger(&err, kAutoDetect);  
      }
      nsAutoString temp;
      temp.Append(PRUnichar(entity));
      Write(temp);
    }
  }
  else if (type == eHTMLTag_br) {
    
    
    nsAutoString typeAttr;
    if (NS_FAILED(GetAttributeValue(aNode, nsGkAtoms::type, typeAttr))
        || !typeAttr.EqualsLiteral("_moz")) {
      EnsureVerticalSpace(mEmptyLines+1);
    }
  }
  else if (type == eHTMLTag_whitespace) {
    
    
    
    
    
    
    
    
    if (mFlags & nsIDocumentEncoder::OutputPreformatted ||
        (mPreFormatted && !mWrapColumn) ||
        IsInPre()) {
      Write(aText);
    }
    else if(!mInWhitespace ||
            (!mStartedOutput
             && mFlags | nsIDocumentEncoder::OutputSelectionOnly)) {
      mInWhitespace = PR_FALSE;
      Write(kSpace);
      mInWhitespace = PR_TRUE;
    }
  }
  else if (type == eHTMLTag_newline) {
    if (mFlags & nsIDocumentEncoder::OutputPreformatted ||
        (mPreFormatted && !mWrapColumn) ||
        IsInPre()) {
      EnsureVerticalSpace(mEmptyLines+1);
    }
    else {
      Write(kSpace);
    }
  }
  else if (type == eHTMLTag_hr &&
           (mFlags & nsIDocumentEncoder::OutputFormatted)) {
    EnsureVerticalSpace(0);

    
    
    nsAutoString line;
    PRUint32 width = (mWrapColumn > 0 ? mWrapColumn : 25);
    while (line.Length() < width) {
      line.Append(PRUnichar('-'));
    }
    Write(line);

    EnsureVerticalSpace(0);
  }
  else if (type == eHTMLTag_img) {
    

    
    nsAutoString imageDescription;
    if (NS_SUCCEEDED(GetAttributeValue(aNode,
                                       nsGkAtoms::alt,
                                       imageDescription))) {
      
    }
    else if (NS_SUCCEEDED(GetAttributeValue(aNode,
                                            nsGkAtoms::title,
                                            imageDescription))
             && !imageDescription.IsEmpty()) {
      imageDescription = NS_LITERAL_STRING(" [") +
                         imageDescription +
                         NS_LITERAL_STRING("] ");
    }
   
    Write(imageDescription);
  }


  return NS_OK;
}








void
nsPlainTextSerializer::EnsureVerticalSpace(PRInt32 noOfRows)
{
  
  
  
  if(noOfRows >= 0 && !mInIndentString.IsEmpty()) {
    EndLine(PR_FALSE);
  }

  while(mEmptyLines < noOfRows) {
    EndLine(PR_FALSE);
  }
  mLineBreakDue = PR_FALSE;
  mFloatingLines = -1;
}









void
nsPlainTextSerializer::FlushLine()
{
  if(!mCurrentLine.IsEmpty()) {
    if(mAtFirstColumn) {
      OutputQuotesAndIndent(); 
    }

    Output(mCurrentLine);
    mAtFirstColumn = mAtFirstColumn && mCurrentLine.IsEmpty();
    mCurrentLine.Truncate();
    mCurrentLineWidth = 0;
  }
}







void 
nsPlainTextSerializer::Output(nsString& aString)
{
  if (!aString.IsEmpty()) {
    mStartedOutput = PR_TRUE;
  }

  if (!(mFlags & nsIDocumentEncoder::OutputPersistNBSP)) {
    
    
    aString.ReplaceChar(kNBSP, kSPACE);
  }
  mOutputString->Append(aString);
}

static PRBool
IsSpaceStuffable(const PRUnichar *s)
{
  if (s[0] == '>' || s[0] == ' ' || s[0] == kNBSP ||
      nsCRT::strncmp(s, NS_LITERAL_STRING("From ").get(), 5) == 0)
    return PR_TRUE;
  else
    return PR_FALSE;
}







void
nsPlainTextSerializer::AddToLine(const PRUnichar * aLineFragment, 
                                 PRInt32 aLineFragmentLength)
{
  PRUint32 prefixwidth = (mCiteQuoteLevel > 0 ? mCiteQuoteLevel + 1:0)+mIndent;
  
  if (mLineBreakDue)
    EnsureVerticalSpace(mFloatingLines);

  PRInt32 linelength = mCurrentLine.Length();
  if(0 == linelength) {
    if(0 == aLineFragmentLength) {
      
      return;
    }

    if(mFlags & nsIDocumentEncoder::OutputFormatFlowed) {
      if(IsSpaceStuffable(aLineFragment)
         && mCiteQuoteLevel == 0  
         )
        {
          
          mCurrentLine.Append(PRUnichar(' '));
          
          if(MayWrap()) {
            mCurrentLineWidth += GetUnicharWidth(' ');
#ifdef DEBUG_wrapping
            NS_ASSERTION(GetUnicharStringWidth(mCurrentLine.get(),
                                               mCurrentLine.Length()) ==
                         (PRInt32)mCurrentLineWidth,
                         "mCurrentLineWidth and reality out of sync!");
#endif
          }
        }
    }
    mEmptyLines=-1;
  }
    
  mCurrentLine.Append(aLineFragment, aLineFragmentLength);
  if(MayWrap()) {
    mCurrentLineWidth += GetUnicharStringWidth(aLineFragment,
                                               aLineFragmentLength);
#ifdef DEBUG_wrapping
    NS_ASSERTION(GetUnicharstringWidth(mCurrentLine.get(),
                                       mCurrentLine.Length()) ==
                 (PRInt32)mCurrentLineWidth,
                 "mCurrentLineWidth and reality out of sync!");
#endif
  }

  linelength = mCurrentLine.Length();

  
  if(MayWrap())
  {
#ifdef DEBUG_wrapping
    NS_ASSERTION(GetUnicharstringWidth(mCurrentLine.get(),
                                  mCurrentLine.Length()) ==
                 (PRInt32)mCurrentLineWidth,
                 "mCurrentLineWidth and reality out of sync!");
#endif
    
    
    
    
    PRUint32 bonuswidth = (mWrapColumn > 20) ? 4 : 0;

    
    while(mCurrentLineWidth+prefixwidth > mWrapColumn+bonuswidth) {      
      
      
      PRInt32 goodSpace = mCurrentLine.Length();
      PRUint32 width = mCurrentLineWidth;
      while(goodSpace > 0 && (width+prefixwidth > mWrapColumn)) {
        goodSpace--;
        width -= GetUnicharWidth(mCurrentLine[goodSpace]);
      }

      goodSpace++;
      
      if (mLineBreaker) {
        goodSpace = mLineBreaker->Prev(mCurrentLine.get(), 
                                    mCurrentLine.Length(), goodSpace);
        if (goodSpace != NS_LINEBREAKER_NEED_MORE_TEXT &&
            nsCRT::IsAsciiSpace(mCurrentLine.CharAt(goodSpace-1))) {
          --goodSpace;    
        }
      }
      
      if (!mLineBreaker) {
        goodSpace = mWrapColumn-prefixwidth;
        while (goodSpace >= 0 &&
               !nsCRT::IsAsciiSpace(mCurrentLine.CharAt(goodSpace))) {
          goodSpace--;
        }
      }
      
      nsAutoString restOfLine;
      if (goodSpace == NS_LINEBREAKER_NEED_MORE_TEXT) {
        
        
        goodSpace=(prefixwidth>mWrapColumn+1)?1:mWrapColumn-prefixwidth+1;
        if (mLineBreaker) {
          if (goodSpace < mCurrentLine.Length())
            goodSpace = mLineBreaker->Next(mCurrentLine.get(), 
                                           mCurrentLine.Length(), goodSpace);
          if (goodSpace == NS_LINEBREAKER_NEED_MORE_TEXT)
            goodSpace = mCurrentLine.Length();
        }
        
        if (!mLineBreaker) {
          goodSpace=(prefixwidth>mWrapColumn)?1:mWrapColumn-prefixwidth;
          while (goodSpace < linelength &&
                 !nsCRT::IsAsciiSpace(mCurrentLine.CharAt(goodSpace))) {
            goodSpace++;
          }
        }
      }
      
      if((goodSpace < linelength) && (goodSpace > 0)) {
        

        
        
        if (nsCRT::IsAsciiSpace(mCurrentLine.CharAt(goodSpace))) {
          mCurrentLine.Right(restOfLine, linelength-goodSpace-1);
        }
        else {
          mCurrentLine.Right(restOfLine, linelength-goodSpace);
        }
        mCurrentLine.Truncate(goodSpace); 
        EndLine(PR_TRUE);
        mCurrentLine.Truncate();
        
        if(mFlags & nsIDocumentEncoder::OutputFormatFlowed) {
          if(!restOfLine.IsEmpty() && IsSpaceStuffable(restOfLine.get())
              && mCiteQuoteLevel == 0  
            )
          {
            
            mCurrentLine.Append(PRUnichar(' '));
            
          }
        }
        mCurrentLine.Append(restOfLine);
        mCurrentLineWidth = GetUnicharStringWidth(mCurrentLine.get(),
                                                  mCurrentLine.Length());
        linelength = mCurrentLine.Length();
        mEmptyLines = -1;
      } 
      else {
        
        
        break;
      }
    }
  } 
  else {
    
  }
}







void
nsPlainTextSerializer::EndLine(PRBool aSoftlinebreak)
{
  PRUint32 currentlinelength = mCurrentLine.Length();

  if(aSoftlinebreak && 0 == currentlinelength) {
    
    return;
  }

  




  
  if(!(mFlags & nsIDocumentEncoder::OutputPreformatted) &&
     (aSoftlinebreak || 
     !(mCurrentLine.EqualsLiteral("-- ") || mCurrentLine.EqualsLiteral("- -- ")))) {
    
    while(currentlinelength > 0 &&
          mCurrentLine[currentlinelength-1] == ' ') {
      --currentlinelength;
    }
    mCurrentLine.SetLength(currentlinelength);
  }
  
  if(aSoftlinebreak &&
     (mFlags & nsIDocumentEncoder::OutputFormatFlowed) &&
     (mIndent == 0)) {
    
    
    
    mCurrentLine.Append(PRUnichar(' '));
  }

  if(aSoftlinebreak) {
    mEmptyLines=0;
  } 
  else {
    
    if(!mCurrentLine.IsEmpty() || !mInIndentString.IsEmpty()) {
      mEmptyLines=-1;
    }

    mEmptyLines++;
  }

  if(mAtFirstColumn) {
    
    
    
    PRBool stripTrailingSpaces = mCurrentLine.IsEmpty();
    OutputQuotesAndIndent(stripTrailingSpaces);
  }

  mCurrentLine.Append(mLineBreak);
  Output(mCurrentLine);
  mCurrentLine.Truncate();
  mCurrentLineWidth = 0;
  mAtFirstColumn=PR_TRUE;
  mInWhitespace=PR_TRUE;
  mLineBreakDue = PR_FALSE;
  mFloatingLines = -1;
}







void
nsPlainTextSerializer::OutputQuotesAndIndent(PRBool stripTrailingSpaces )
{
  nsAutoString stringToOutput;
  
  
  if (mCiteQuoteLevel > 0) {
    nsAutoString quotes;
    for(int i=0; i < mCiteQuoteLevel; i++) {
      quotes.Append(PRUnichar('>'));
    }
    if (!mCurrentLine.IsEmpty()) {
      




      quotes.Append(PRUnichar(' '));
    }
    stringToOutput = quotes;
    mAtFirstColumn = PR_FALSE;
  }
  
  
  PRInt32 indentwidth = mIndent - mInIndentString.Length();
  if (indentwidth > 0
      && (!mCurrentLine.IsEmpty() || !mInIndentString.IsEmpty())
      
      ) {
    nsAutoString spaces;
    for (int i=0; i < indentwidth; ++i)
      spaces.Append(PRUnichar(' '));
    stringToOutput += spaces;
    mAtFirstColumn = PR_FALSE;
  }
  
  if(!mInIndentString.IsEmpty()) {
    stringToOutput += mInIndentString;
    mAtFirstColumn = PR_FALSE;
    mInIndentString.Truncate();
  }

  if(stripTrailingSpaces) {
    PRInt32 lineLength = stringToOutput.Length();
    while(lineLength > 0 &&
          ' ' == stringToOutput[lineLength-1]) {
      --lineLength;
    }
    stringToOutput.SetLength(lineLength);
  }

  if(!stringToOutput.IsEmpty()) {
    Output(stringToOutput);
  }
    
}






void
nsPlainTextSerializer::Write(const nsAString& aStr)
{
  
  
  nsAutoString str(aStr);

#ifdef DEBUG_wrapping
  printf("Write(%s): wrap col = %d\n",
         NS_ConvertUTF16toUTF8(str).get(), mWrapColumn);
#endif

  PRInt32 bol = 0;
  PRInt32 newline;
  
  PRInt32 totLen = str.Length();

  
  if (totLen <= 0) return;

  
  
  if (mFlags & nsIDocumentEncoder::OutputFormatFlowed) {
    for (PRInt32 i = totLen-1; i >= 0; i--) {
      PRUnichar c = str[i];
      if ('\n' == c || '\r' == c || ' ' == c || '\t' == c)
        continue;
      if (kNBSP == c)
        str.Replace(i, 1, ' ');
      else
        break;
    }
  }

  
  
  
  if ((mPreFormatted && !mWrapColumn) || IsInPre()
      || ((((!mQuotesPreformatted && mSpanLevel > 0) || mDontWrapAnyQuotes))
          && mEmptyLines >= 0 && str.First() == PRUnichar('>'))) {
    

    
    
    NS_ASSERTION(mCurrentLine.IsEmpty(),
                 "Mixed wrapping data and nonwrapping data on the same line");
    if (!mCurrentLine.IsEmpty()) {
      FlushLine();
    }

    
    
    while(bol<totLen) {
      PRBool outputQuotes = mAtFirstColumn;
      PRBool atFirstColumn = mAtFirstColumn;
      PRBool outputLineBreak = PR_FALSE;
      PRBool spacesOnly = PR_TRUE;

      
      
      nsAString::const_iterator iter;           str.BeginReading(iter);
      nsAString::const_iterator done_searching; str.EndReading(done_searching);
      iter.advance(bol); 
      PRInt32 new_newline = bol;
      newline = kNotFound;
      while(iter != done_searching) {
        if('\n' == *iter || '\r' == *iter) {
          newline = new_newline;
          break;
        }
        if(' ' != *iter)
          spacesOnly = PR_FALSE;
        ++new_newline;
        ++iter;
      }

      
      nsAutoString stringpart;
      if(newline == kNotFound) {
        
        stringpart.Assign(Substring(str, bol, totLen - bol));
        if(!stringpart.IsEmpty()) {
          PRUnichar lastchar = stringpart[stringpart.Length()-1];
          if((lastchar == '\t') || (lastchar == ' ') ||
             (lastchar == '\r') ||(lastchar == '\n')) {
            mInWhitespace = PR_TRUE;
          } 
          else {
            mInWhitespace = PR_FALSE;
          }
        }
        mEmptyLines=-1;
        atFirstColumn = mAtFirstColumn && (totLen-bol)==0;
        bol = totLen;
      } 
      else {
        
        stringpart.Assign(Substring(str, bol, newline-bol));
        mInWhitespace = PR_TRUE;
        outputLineBreak = PR_TRUE;
        mEmptyLines=0;
        atFirstColumn = PR_TRUE;
        bol = newline+1;
        if('\r' == *iter && bol < totLen && '\n' == *++iter) {
          
          
          
          bol++;
        }
      }

      mCurrentLine.AssignLiteral("");
      if (mFlags & nsIDocumentEncoder::OutputFormatFlowed) {
        if ((outputLineBreak || !spacesOnly) && 
            !stringpart.EqualsLiteral("-- ") &&
            !stringpart.EqualsLiteral("- -- "))
          stringpart.Trim(" ", PR_FALSE, PR_TRUE, PR_TRUE);
        if (IsSpaceStuffable(stringpart.get()) && stringpart[0] != '>')
          mCurrentLine.Append(PRUnichar(' '));
      }
      mCurrentLine.Append(stringpart);

      if(outputQuotes) {
        
        OutputQuotesAndIndent();
      }

      Output(mCurrentLine);
      if (outputLineBreak) {
        Output(mLineBreak);
      }
      mAtFirstColumn = atFirstColumn;
    }

    
    mCurrentLine.Truncate();

#ifdef DEBUG_wrapping
    printf("No wrapping: newline is %d, totLen is %d\n",
           newline, totLen);
#endif
    return;
  }

  
  
  
  PRInt32 nextpos;
  const PRUnichar * offsetIntoBuffer = nsnull;
  
  while (bol < totLen) {    
    
    nextpos = str.FindCharInSet(" \t\n\r", bol);
#ifdef DEBUG_wrapping
    nsAutoString remaining;
    str.Right(remaining, totLen - bol);
    foo = ToNewCString(remaining);
    
    
    nsMemory::Free(foo);
#endif

    if(nextpos == kNotFound) {
      
      offsetIntoBuffer = str.get() + bol;
      AddToLine(offsetIntoBuffer, totLen-bol);
      bol=totLen;
      mInWhitespace=PR_FALSE;
    } 
    else {
      
      if (nextpos != 0 && (nextpos + 1) < totLen) {
        offsetIntoBuffer = str.get() + nextpos;
        
        if (offsetIntoBuffer[0] == '\n' && IS_CJ_CHAR(offsetIntoBuffer[-1]) && IS_CJ_CHAR(offsetIntoBuffer[1])) {
          offsetIntoBuffer = str.get() + bol;
          AddToLine(offsetIntoBuffer, nextpos-bol);
          bol = nextpos + 1;
          continue;
        }
      }
      
      if (mInWhitespace && (nextpos == bol) && !mPreFormatted &&
          !(mFlags & nsIDocumentEncoder::OutputPreformatted)) {
        
        bol++;
        continue;
      }

      if(nextpos == bol) {
        
        mInWhitespace = PR_TRUE;
        offsetIntoBuffer = str.get() + nextpos;
        AddToLine(offsetIntoBuffer, 1);
        bol++;
        continue;
      }
      
      mInWhitespace = PR_TRUE;
      
      offsetIntoBuffer = str.get() + bol;
      if(mPreFormatted || (mFlags & nsIDocumentEncoder::OutputPreformatted)) {
        
        nextpos++;
        AddToLine(offsetIntoBuffer, nextpos-bol);
        bol = nextpos;
      } 
      else {
        
        AddToLine(offsetIntoBuffer, nextpos-bol);
        AddToLine(kSpace.get(),1);
        bol = nextpos + 1; 
      }
    }
  } 
}






nsresult
nsPlainTextSerializer::GetAttributeValue(const nsIParserNode* aNode,
                                         nsIAtom* aName,
                                         nsString& aValueRet)
{
  if (mContent) {
    if (mContent->GetAttr(kNameSpaceID_None, aName, aValueRet)) {
      return NS_OK;
    }
  }
  else if (aNode) {
    nsAutoString name; 
    aName->ToString(name);

    PRInt32 count = aNode->GetAttributeCount();
    for (PRInt32 i=0;i<count;i++) {
      const nsAString& key = aNode->GetKeyAt(i);
      if (key.Equals(name, nsCaseInsensitiveStringComparator())) {
        aValueRet = aNode->GetValueAt(i);
        return NS_OK;
      }
    }
  }

  return NS_ERROR_NOT_AVAILABLE;
}





PRBool 
nsPlainTextSerializer::IsCurrentNodeConverted(const nsIParserNode* aNode)
{
  nsAutoString value;
  nsresult rv = GetAttributeValue(aNode, nsGkAtoms::_class, value);
  return (NS_SUCCEEDED(rv) &&
          (value.EqualsIgnoreCase("moz-txt", 7) ||
           value.EqualsIgnoreCase("\"moz-txt", 8)));
}



PRInt32
nsPlainTextSerializer::GetIdForContent(nsIContent* aContent)
{
  if (!aContent->IsNodeOfType(nsINode::eHTML)) {
    return eHTMLTag_unknown;
  }

  nsIParserService* parserService = nsContentUtils::GetParserService();

  return parserService ? parserService->HTMLAtomTagToId(aContent->Tag()) :
                         eHTMLTag_unknown;
}





PRBool 
nsPlainTextSerializer::IsBlockLevel(PRInt32 aId)
{
  PRBool isBlock = PR_FALSE;

  nsIParserService* parserService = nsContentUtils::GetParserService();
  if (parserService) {
    parserService->IsBlock(aId, isBlock);
  }

  return isBlock;
}




PRBool 
nsPlainTextSerializer::IsContainer(PRInt32 aId)
{
  PRBool isContainer = PR_FALSE;

  nsIParserService* parserService = nsContentUtils::GetParserService();
  if (parserService) {
    parserService->IsContainer(aId, isContainer);
  }

  return isContainer;
}







  
PRBool
nsPlainTextSerializer::IsInPre()
{
  PRInt32 i = mTagStackIndex;
  while(i > 0) {
    if(mTagStack[i-1] == eHTMLTag_pre)
      return PR_TRUE;
    if(IsBlockLevel(mTagStack[i-1])) {
      
      return PR_FALSE;
    }
    --i;
  }

  
  return PR_FALSE;
}





PRBool
nsPlainTextSerializer::IsInOL()
{
  PRInt32 i = mTagStackIndex;
  while(--i >= 0) {
    if(mTagStack[i] == eHTMLTag_ol)
      return PR_TRUE;
    if (mTagStack[i] == eHTMLTag_ul) {
      
      return PR_FALSE;
    }
  }
  
  return PR_FALSE;
}




PRInt32 HeaderLevel(eHTMLTags aTag)
{
  PRInt32 result;
  switch (aTag)
  {
    case eHTMLTag_h1:
      result = 1; break;
    case eHTMLTag_h2:
      result = 2; break;
    case eHTMLTag_h3:
      result = 3; break;
    case eHTMLTag_h4:
      result = 4; break;
    case eHTMLTag_h5:
      result = 5; break;
    case eHTMLTag_h6:
      result = 6; break;
    default:
      result = 0; break;
  }
  return result;
}





































PRInt32 GetUnicharWidth(PRUnichar ucs)
{
  
  static const struct interval {
    PRUint16 first;
    PRUint16 last;
  } combining[] = {
    { 0x0300, 0x034E }, { 0x0360, 0x0362 }, { 0x0483, 0x0486 },
    { 0x0488, 0x0489 }, { 0x0591, 0x05A1 }, { 0x05A3, 0x05B9 },
    { 0x05BB, 0x05BD }, { 0x05BF, 0x05BF }, { 0x05C1, 0x05C2 },
    { 0x05C4, 0x05C4 }, { 0x064B, 0x0655 }, { 0x0670, 0x0670 },
    { 0x06D6, 0x06E4 }, { 0x06E7, 0x06E8 }, { 0x06EA, 0x06ED },
    { 0x0711, 0x0711 }, { 0x0730, 0x074A }, { 0x07A6, 0x07B0 },
    { 0x0901, 0x0902 }, { 0x093C, 0x093C }, { 0x0941, 0x0948 },
    { 0x094D, 0x094D }, { 0x0951, 0x0954 }, { 0x0962, 0x0963 },
    { 0x0981, 0x0981 }, { 0x09BC, 0x09BC }, { 0x09C1, 0x09C4 },
    { 0x09CD, 0x09CD }, { 0x09E2, 0x09E3 }, { 0x0A02, 0x0A02 },
    { 0x0A3C, 0x0A3C }, { 0x0A41, 0x0A42 }, { 0x0A47, 0x0A48 },
    { 0x0A4B, 0x0A4D }, { 0x0A70, 0x0A71 }, { 0x0A81, 0x0A82 },
    { 0x0ABC, 0x0ABC }, { 0x0AC1, 0x0AC5 }, { 0x0AC7, 0x0AC8 },
    { 0x0ACD, 0x0ACD }, { 0x0B01, 0x0B01 }, { 0x0B3C, 0x0B3C },
    { 0x0B3F, 0x0B3F }, { 0x0B41, 0x0B43 }, { 0x0B4D, 0x0B4D },
    { 0x0B56, 0x0B56 }, { 0x0B82, 0x0B82 }, { 0x0BC0, 0x0BC0 },
    { 0x0BCD, 0x0BCD }, { 0x0C3E, 0x0C40 }, { 0x0C46, 0x0C48 },
    { 0x0C4A, 0x0C4D }, { 0x0C55, 0x0C56 }, { 0x0CBF, 0x0CBF },
    { 0x0CC6, 0x0CC6 }, { 0x0CCC, 0x0CCD }, { 0x0D41, 0x0D43 },
    { 0x0D4D, 0x0D4D }, { 0x0DCA, 0x0DCA }, { 0x0DD2, 0x0DD4 },
    { 0x0DD6, 0x0DD6 }, { 0x0E31, 0x0E31 }, { 0x0E34, 0x0E3A },
    { 0x0E47, 0x0E4E }, { 0x0EB1, 0x0EB1 }, { 0x0EB4, 0x0EB9 },
    { 0x0EBB, 0x0EBC }, { 0x0EC8, 0x0ECD }, { 0x0F18, 0x0F19 },
    { 0x0F35, 0x0F35 }, { 0x0F37, 0x0F37 }, { 0x0F39, 0x0F39 },
    { 0x0F71, 0x0F7E }, { 0x0F80, 0x0F84 }, { 0x0F86, 0x0F87 },
    { 0x0F90, 0x0F97 }, { 0x0F99, 0x0FBC }, { 0x0FC6, 0x0FC6 },
    { 0x102D, 0x1030 }, { 0x1032, 0x1032 }, { 0x1036, 0x1037 },
    { 0x1039, 0x1039 }, { 0x1058, 0x1059 }, { 0x17B7, 0x17BD },
    { 0x17C6, 0x17C6 }, { 0x17C9, 0x17D3 }, { 0x18A9, 0x18A9 },
    { 0x20D0, 0x20E3 }, { 0x302A, 0x302F }, { 0x3099, 0x309A },
    { 0xFB1E, 0xFB1E }, { 0xFE20, 0xFE23 }
  };
  PRInt32 min = 0;
  PRInt32 max = sizeof(combining) / sizeof(struct interval) - 1;
  PRInt32 mid;

  
  if (ucs == 0)
    return 0;
  if (ucs < 32 || (ucs >= 0x7f && ucs < 0xa0))
    return -1;

  
  if (ucs < combining[0].first)
    return 1;

  
  while (max >= min) {
    mid = (min + max) / 2;
    if (combining[mid].last < ucs)
      min = mid + 1;
    else if (combining[mid].first > ucs)
      max = mid - 1;
    else if (combining[mid].first <= ucs && combining[mid].last >= ucs)
      return 0;
  }

  

  
  if (ucs < 0x1100)
    return 1;

  return 1 +
    ((ucs >= 0x1100 && ucs <= 0x115f) || 
     (ucs >= 0x2e80 && ucs <= 0xa4cf && (ucs & ~0x0011) != 0x300a &&
      ucs != 0x303f) ||                  
     (ucs >= 0xac00 && ucs <= 0xd7a3) || 
     (ucs >= 0xf900 && ucs <= 0xfaff) || 
     (ucs >= 0xfe30 && ucs <= 0xfe6f) || 
     (ucs >= 0xff00 && ucs <= 0xff5f) || 
     (ucs >= 0xffe0 && ucs <= 0xffe6));
}


PRInt32 GetUnicharStringWidth(const PRUnichar* pwcs, PRInt32 n)
{
  PRInt32 w, width = 0;

  for (;*pwcs && n-- > 0; pwcs++)
    if ((w = GetUnicharWidth(*pwcs)) < 0)
      ++width; 
    else
      width += w;

  return width;
}


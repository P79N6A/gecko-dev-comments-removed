










#include "nsPlainTextSerializer.h"
#include "nsLWBrkCIID.h"
#include "nsIServiceManager.h"
#include "nsGkAtoms.h"
#include "nsNameSpaceManager.h"
#include "nsTextFragment.h"
#include "nsContentUtils.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsCRT.h"
#include "mozilla/dom/Element.h"
#include "mozilla/Preferences.h"
#include "mozilla/BinarySearch.h"
#include "nsComputedDOMStyle.h"

using namespace mozilla;
using namespace mozilla::dom;

#define PREF_STRUCTS "converter.html2txt.structs"
#define PREF_HEADER_STRATEGY "converter.html2txt.header_strategy"
#define PREF_ALWAYS_INCLUDE_RUBY "converter.html2txt.always_include_ruby"

static const  int32_t kTabSize=4;
static const  int32_t kIndentSizeHeaders = 2;  




static const  int32_t kIndentIncrementHeaders = 2;  


static const  int32_t kIndentSizeList = kTabSize;
                               
static const  int32_t kIndentSizeDD = kTabSize;  
static const  char16_t  kNBSP = 160;
static const  char16_t kSPACE = ' ';

static int32_t HeaderLevel(nsIAtom* aTag);
static int32_t GetUnicharWidth(char16_t ucs);
static int32_t GetUnicharStringWidth(const char16_t* pwcs, int32_t n);


static const uint32_t TagStackSize = 500;
static const uint32_t OLStackSize = 100;

nsresult
NS_NewPlainTextSerializer(nsIContentSerializer** aSerializer)
{
  nsRefPtr<nsPlainTextSerializer> it = new nsPlainTextSerializer();
  it.forget(aSerializer);
  return NS_OK;
}

nsPlainTextSerializer::nsPlainTextSerializer()
  : kSpace(NS_LITERAL_STRING(" ")) 
{

  mOutputString = nullptr;
  mHeadLevel = 0;
  mAtFirstColumn = true;
  mIndent = 0;
  mCiteQuoteLevel = 0;
  mStructs = true;       
  mHeaderStrategy = 1 ;   
  mDontWrapAnyQuotes = false;                 
  mHasWrittenCiteBlockquote = false;
  mSpanLevel = 0;
  for (int32_t i = 0; i <= 6; i++) {
    mHeaderCounter[i] = 0;
  }

  
  mWrapColumn = 72;     
  mCurrentLineWidth = 0;

  
  mEmptyLines = 1; 
  mInWhitespace = false;
  mPreFormattedMail = false;
  mStartedOutput = false;

  mPreformattedBlockBoundary = false;
  mWithRubyAnnotation = false;  

  
  
  
  mTagStack = new nsIAtom*[TagStackSize];
  mTagStackIndex = 0;
  mIgnoreAboveIndex = (uint32_t)kNotFound;

  
  mOLStack = new int32_t[OLStackSize];
  mOLStackIndex = 0;

  mULCount = 0;

  mIgnoredChildNodeLevel = 0;
}

nsPlainTextSerializer::~nsPlainTextSerializer()
{
  delete[] mTagStack;
  delete[] mOLStack;
  NS_WARN_IF_FALSE(mHeadLevel == 0, "Wrong head level!");
}

NS_IMPL_ISUPPORTS(nsPlainTextSerializer,
                  nsIContentSerializer)


NS_IMETHODIMP 
nsPlainTextSerializer::Init(uint32_t aFlags, uint32_t aWrapColumn,
                            const char* aCharSet, bool aIsCopying,
                            bool aIsWholeDocument)
{
#ifdef DEBUG
  
  if (aFlags & nsIDocumentEncoder::OutputFormatFlowed) {
    NS_ASSERTION(aFlags & nsIDocumentEncoder::OutputFormatted,
                 "If you want format=flowed, you must combine it with "
                 "nsIDocumentEncoder::OutputFormatted");
  }

  if (aFlags & nsIDocumentEncoder::OutputFormatted) {
    NS_ASSERTION(!(aFlags & nsIDocumentEncoder::OutputPreformatted),
                 "Can't do formatted and preformatted output at the same time!");
  }
#endif

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
    
    mLineBreak.Assign(char16_t('\r'));
  }
  else if (mFlags & nsIDocumentEncoder::OutputLFLineBreak) {
    
    mLineBreak.Assign(char16_t('\n'));
  }
  else {
    
    mLineBreak.AssignLiteral(NS_LINEBREAK);
  }

  mLineBreakDue = false;
  mFloatingLines = -1;

  mPreformattedBlockBoundary = false;

  if (mFlags & nsIDocumentEncoder::OutputFormatted) {
    
    mStructs = Preferences::GetBool(PREF_STRUCTS, mStructs);

    mHeaderStrategy =
      Preferences::GetInt(PREF_HEADER_STRATEGY, mHeaderStrategy);

    
    
    
    if (mFlags & nsIDocumentEncoder::OutputWrap || mWrapColumn > 0) {
      mDontWrapAnyQuotes =
        Preferences::GetBool("mail.compose.wrap_to_window_width",
                             mDontWrapAnyQuotes);
    }
  }

  
  
  
  mWithRubyAnnotation =
    Preferences::GetBool(PREF_ALWAYS_INCLUDE_RUBY, true) ||
    (mFlags & nsIDocumentEncoder::OutputRubyAnnotation);

  
  mFlags &= ~nsIDocumentEncoder::OutputNoFramesContent;

  return NS_OK;
}

bool
nsPlainTextSerializer::GetLastBool(const nsTArray<bool>& aStack)
{
  uint32_t size = aStack.Length();
  if (size == 0) {
    return false;
  }
  return aStack.ElementAt(size-1);
}

void
nsPlainTextSerializer::SetLastBool(nsTArray<bool>& aStack, bool aValue)
{
  uint32_t size = aStack.Length();
  if (size > 0) {
    aStack.ElementAt(size-1) = aValue;
  }
  else {
    NS_ERROR("There is no \"Last\" value");
  }
}

void
nsPlainTextSerializer::PushBool(nsTArray<bool>& aStack, bool aValue)
{
    aStack.AppendElement(bool(aValue));
}

bool
nsPlainTextSerializer::PopBool(nsTArray<bool>& aStack)
{
  bool returnValue = false;
  uint32_t size = aStack.Length();
  if (size > 0) {
    returnValue = aStack.ElementAt(size-1);
    aStack.RemoveElementAt(size-1);
  }
  return returnValue;
}

bool
nsPlainTextSerializer::ShouldReplaceContainerWithPlaceholder(nsIAtom* aTag)
{
  
  
  
  if (!(mFlags & nsIDocumentEncoder::OutputNonTextContentAsPlaceholder)) {
    return false;
  }

  return
    (aTag == nsGkAtoms::audio) ||
    (aTag == nsGkAtoms::canvas) ||
    (aTag == nsGkAtoms::iframe) ||
    (aTag == nsGkAtoms::meter) ||
    (aTag == nsGkAtoms::progress) ||
    (aTag == nsGkAtoms::object) ||
    (aTag == nsGkAtoms::svg) ||
    (aTag == nsGkAtoms::video);
}

bool
nsPlainTextSerializer::IsIgnorableRubyAnnotation(nsIAtom* aTag)
{
  if (mWithRubyAnnotation) {
    return false;
  }

  return
    aTag == nsGkAtoms::rp ||
    aTag == nsGkAtoms::rt ||
    aTag == nsGkAtoms::rtc;
}

NS_IMETHODIMP 
nsPlainTextSerializer::AppendText(nsIContent* aText,
                                  int32_t aStartOffset,
                                  int32_t aEndOffset, 
                                  nsAString& aStr)
{
  if (mIgnoreAboveIndex != (uint32_t)kNotFound) {
    return NS_OK;
  }
    
  NS_ASSERTION(aStartOffset >= 0, "Negative start offset for text fragment!");
  if ( aStartOffset < 0 )
    return NS_ERROR_INVALID_ARG;

  NS_ENSURE_ARG(aText);

  nsresult rv = NS_OK;

  nsIContent* content = aText;
  const nsTextFragment* frag;
  if (!content || !(frag = content->GetText())) {
    return NS_ERROR_FAILURE;
  }
  
  int32_t fragLength = frag->GetLength();
  int32_t endoffset = (aEndOffset == -1) ? fragLength : std::min(aEndOffset, fragLength);
  NS_ASSERTION(aStartOffset <= endoffset, "A start offset is beyond the end of the text fragment!");

  int32_t length = endoffset - aStartOffset;
  if (length <= 0) {
    return NS_OK;
  }

  nsAutoString textstr;
  if (frag->Is2b()) {
    textstr.Assign(frag->Get2b() + aStartOffset, length);
  }
  else {
    
    const char *data = frag->Get1b();
    CopyASCIItoUTF16(Substring(data + aStartOffset, data + endoffset), textstr);
  }

  mOutputString = &aStr;

  
  
  int32_t start = 0;
  int32_t offset = textstr.FindCharInSet("\n\r");
  while (offset != kNotFound) {

    if (offset>start) {
      
      DoAddText(false,
                Substring(textstr, start, offset-start));
    }

    
    DoAddText(true, mLineBreak);
    
    start = offset+1;
    offset = textstr.FindCharInSet("\n\r", start);
  }

  
  if (start < length) {
    if (start) {
      DoAddText(false, Substring(textstr, start, length - start));
    }
    else {
      DoAddText(false, textstr);
    }
  }
  
  mOutputString = nullptr;

  return rv;
}

NS_IMETHODIMP
nsPlainTextSerializer::AppendCDATASection(nsIContent* aCDATASection,
                                          int32_t aStartOffset,
                                          int32_t aEndOffset,
                                          nsAString& aStr)
{
  return AppendText(aCDATASection, aStartOffset, aEndOffset, aStr);
}

NS_IMETHODIMP
nsPlainTextSerializer::AppendElementStart(Element* aElement,
                                          Element* aOriginalElement,
                                          nsAString& aStr)
{
  NS_ENSURE_ARG(aElement);

  mElement = aElement;

  nsresult rv;
  nsIAtom* id = GetIdForContent(mElement);

  bool isContainer = !FragmentOrElement::IsHTMLVoid(id);

  mOutputString = &aStr;

  if (isContainer) {
    rv = DoOpenContainer(id);
    mPreformatStack.push(IsElementPreformatted(mElement));
  }
  else {
    rv = DoAddLeaf(id);
  }

  mElement = nullptr;
  mOutputString = nullptr;

  if (id == nsGkAtoms::head) {
    ++mHeadLevel;
  }

  return rv;
} 
 
NS_IMETHODIMP 
nsPlainTextSerializer::AppendElementEnd(Element* aElement,
                                        nsAString& aStr)
{
  NS_ENSURE_ARG(aElement);

  mElement = aElement;

  nsresult rv;
  nsIAtom* id = GetIdForContent(mElement);

  bool isContainer = !FragmentOrElement::IsHTMLVoid(id);

  mOutputString = &aStr;

  rv = NS_OK;
  if (isContainer) {
    rv = DoCloseContainer(id);
    mPreformatStack.pop();
  }

  mElement = nullptr;
  mOutputString = nullptr;

  if (id == nsGkAtoms::head) {
    NS_ASSERTION(mHeadLevel != 0,
                 "mHeadLevel being decremented below 0");
    --mHeadLevel;
  }

  return rv;
}

NS_IMETHODIMP 
nsPlainTextSerializer::Flush(nsAString& aStr)
{
  mOutputString = &aStr;
  FlushLine();
  mOutputString = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
nsPlainTextSerializer::AppendDocumentStart(nsIDocument *aDocument,
                                           nsAString& aStr)
{
  return NS_OK;
}

nsresult
nsPlainTextSerializer::DoOpenContainer(nsIAtom* aTag)
{
  
  
  if (ShouldReplaceContainerWithPlaceholder(mElement->NodeInfo()->NameAtom())) {
    if (mIgnoredChildNodeLevel == 0) {
      
      Write(NS_LITERAL_STRING("\xFFFC"));
    }
    
    mIgnoredChildNodeLevel++;
    return NS_OK;
  }
  if (IsIgnorableRubyAnnotation(aTag)) {
    
    
    mIgnoredChildNodeLevel++;
    return NS_OK;
  }

  if (mFlags & nsIDocumentEncoder::OutputForPlainTextClipboardCopy) {
    if (mPreformattedBlockBoundary && DoOutput()) {
      
      if (mFloatingLines < 0)
        mFloatingLines = 0;
      mLineBreakDue = true;
    }
    mPreformattedBlockBoundary = false;
  }

  if (mFlags & nsIDocumentEncoder::OutputRaw) {
    
    
    
    

    return NS_OK;
  }

  if (mTagStackIndex < TagStackSize) {
    mTagStack[mTagStackIndex++] = aTag;
  }

  if (mIgnoreAboveIndex != (uint32_t)kNotFound) {
    return NS_OK;
  }

  
  
  mHasWrittenCiteBlockquote = mHasWrittenCiteBlockquote &&
                              aTag == nsGkAtoms::pre;

  bool isInCiteBlockquote = false;

  
  
  if (aTag == nsGkAtoms::blockquote) {
    nsAutoString value;
    nsresult rv = GetAttributeValue(nsGkAtoms::type, value);
    isInCiteBlockquote = NS_SUCCEEDED(rv) && value.EqualsIgnoreCase("cite");
  }

  if (mLineBreakDue && !isInCiteBlockquote)
    EnsureVerticalSpace(mFloatingLines);

  
  if ((aTag == nsGkAtoms::noscript &&
       !(mFlags & nsIDocumentEncoder::OutputNoScriptContent)) ||
      ((aTag == nsGkAtoms::iframe || aTag == nsGkAtoms::noframes) &&
       !(mFlags & nsIDocumentEncoder::OutputNoFramesContent))) {
    
    
    mIgnoreAboveIndex = mTagStackIndex - 1;
    return NS_OK;
  }

  if (aTag == nsGkAtoms::body) {
    
    
    
    
    
    
    
    
    nsAutoString style;
    int32_t whitespace;
    if (NS_SUCCEEDED(GetAttributeValue(nsGkAtoms::style, style)) &&
       (kNotFound != (whitespace = style.Find("white-space:")))) {

      if (kNotFound != style.Find("pre-wrap", true, whitespace)) {
#ifdef DEBUG_preformatted
        printf("Set mPreFormattedMail based on style pre-wrap\n");
#endif
        mPreFormattedMail = true;
        int32_t widthOffset = style.Find("width:");
        if (widthOffset >= 0) {
          
          
          
          
          
          int32_t semiOffset = style.Find("ch", false, widthOffset+6);
          int32_t length = (semiOffset > 0 ? semiOffset - widthOffset - 6
                            : style.Length() - widthOffset);
          nsAutoString widthstr;
          style.Mid(widthstr, widthOffset+6, length);
          nsresult err;
          int32_t col = widthstr.ToInteger(&err);

          if (NS_SUCCEEDED(err)) {
            mWrapColumn = (uint32_t)col;
#ifdef DEBUG_preformatted
            printf("Set wrap column to %d based on style\n", mWrapColumn);
#endif
          }
        }
      }
      else if (kNotFound != style.Find("pre", true, whitespace)) {
#ifdef DEBUG_preformatted
        printf("Set mPreFormattedMail based on style pre\n");
#endif
        mPreFormattedMail = true;
        mWrapColumn = 0;
      }
    } 
    else {
      
      mInWhitespace = true;
      mPreFormattedMail = false;
    }

    return NS_OK;
  }

  
  if (!DoOutput()) {
    return NS_OK;
  }

  if (aTag == nsGkAtoms::p)
    EnsureVerticalSpace(1);
  else if (aTag == nsGkAtoms::pre) {
    if (GetLastBool(mIsInCiteBlockquote))
      EnsureVerticalSpace(0);
    else if (mHasWrittenCiteBlockquote) {
      EnsureVerticalSpace(0);
      mHasWrittenCiteBlockquote = false;
    }
    else
      EnsureVerticalSpace(1);
  }
  else if (aTag == nsGkAtoms::tr) {
    PushBool(mHasWrittenCellsForRow, false);
  }
  else if (aTag == nsGkAtoms::td || aTag == nsGkAtoms::th) {
    
    

    
    
    if (GetLastBool(mHasWrittenCellsForRow)) {
      
      AddToLine(MOZ_UTF16("\t"), 1);
      mInWhitespace = true;
    }
    else if (mHasWrittenCellsForRow.IsEmpty()) {
      
      
      PushBool(mHasWrittenCellsForRow, true); 
    }
    else {
      SetLastBool(mHasWrittenCellsForRow, true);
    }
  }
  else if (aTag == nsGkAtoms::ul) {
    
    EnsureVerticalSpace(mULCount + mOLStackIndex == 0 ? 1 : 0);
         
    mIndent += kIndentSizeList;
    mULCount++;
  }
  else if (aTag == nsGkAtoms::ol) {
    EnsureVerticalSpace(mULCount + mOLStackIndex == 0 ? 1 : 0);
    if (mFlags & nsIDocumentEncoder::OutputFormatted) {
      
      if (mOLStackIndex < OLStackSize) {
        nsAutoString startAttr;
        int32_t startVal = 1;
        if (NS_SUCCEEDED(GetAttributeValue(nsGkAtoms::start, startAttr))) {
          nsresult rv = NS_OK;
          startVal = startAttr.ToInteger(&rv);
          if (NS_FAILED(rv))
            startVal = 1;
        }
        mOLStack[mOLStackIndex++] = startVal;
      }
    } else {
      mOLStackIndex++;
    }
    mIndent += kIndentSizeList;  
  }
  else if (aTag == nsGkAtoms::li &&
           (mFlags & nsIDocumentEncoder::OutputFormatted)) {
    if (mTagStackIndex > 1 && IsInOL()) {
      if (mOLStackIndex > 0) {
        nsAutoString valueAttr;
        if (NS_SUCCEEDED(GetAttributeValue(nsGkAtoms::value, valueAttr))) {
          nsresult rv = NS_OK;
          int32_t valueAttrVal = valueAttr.ToInteger(&rv);
          if (NS_SUCCEEDED(rv))
            mOLStack[mOLStackIndex-1] = valueAttrVal;
        }
        
        mInIndentString.AppendInt(mOLStack[mOLStackIndex-1]++, 10);
      }
      else {
        mInIndentString.Append(char16_t('#'));
      }

      mInIndentString.Append(char16_t('.'));

    }
    else {
      static char bulletCharArray[] = "*o+#";
      uint32_t index = mULCount > 0 ? (mULCount - 1) : 3;
      char bulletChar = bulletCharArray[index % 4];
      mInIndentString.Append(char16_t(bulletChar));
    }

    mInIndentString.Append(char16_t(' '));
  }
  else if (aTag == nsGkAtoms::dl) {
    EnsureVerticalSpace(1);
  }
  else if (aTag == nsGkAtoms::dt) {
    EnsureVerticalSpace(0);
  }
  else if (aTag == nsGkAtoms::dd) {
    EnsureVerticalSpace(0);
    mIndent += kIndentSizeDD;
  }
  else if (aTag == nsGkAtoms::span) {
    ++mSpanLevel;
  }
  else if (aTag == nsGkAtoms::blockquote) {
    
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
  else if (aTag == nsGkAtoms::q) {
    Write(NS_LITERAL_STRING("\""));
  }

  
  
  else if (IsElementBlock(mElement)) {
    EnsureVerticalSpace(0);
  }

  
  if (!(mFlags & nsIDocumentEncoder::OutputFormatted)) {
    return NS_OK;
  }
  
  
  
  

  
  bool currentNodeIsConverted = IsCurrentNodeConverted();

  if (aTag == nsGkAtoms::h1 || aTag == nsGkAtoms::h2 ||
      aTag == nsGkAtoms::h3 || aTag == nsGkAtoms::h4 ||
      aTag == nsGkAtoms::h5 || aTag == nsGkAtoms::h6)
  {
    EnsureVerticalSpace(2);
    if (mHeaderStrategy == 2) {  
      mIndent += kIndentSizeHeaders;
      
      int32_t level = HeaderLevel(aTag);
      
      mHeaderCounter[level]++;
      
      int32_t i;

      for (i = level + 1; i <= 6; i++) {
        mHeaderCounter[i] = 0;
      }

      
      nsAutoString leadup;
      for (i = 1; i <= level; i++) {
        leadup.AppendInt(mHeaderCounter[i]);
        leadup.Append(char16_t('.'));
      }
      leadup.Append(char16_t(' '));
      Write(leadup);
    }
    else if (mHeaderStrategy == 1) { 
      mIndent += kIndentSizeHeaders;
      for (int32_t i = HeaderLevel(aTag); i > 1; i--) {
           
        mIndent += kIndentIncrementHeaders;
      }
    }
  }
  else if (aTag == nsGkAtoms::a && !currentNodeIsConverted) {
    nsAutoString url;
    if (NS_SUCCEEDED(GetAttributeValue(nsGkAtoms::href, url))
        && !url.IsEmpty()) {
      mURL = url;
    }
  }
  else if (aTag == nsGkAtoms::sup && mStructs && !currentNodeIsConverted) {
    Write(NS_LITERAL_STRING("^"));
  }
  else if (aTag == nsGkAtoms::sub && mStructs && !currentNodeIsConverted) {
    Write(NS_LITERAL_STRING("_"));
  }
  else if (aTag == nsGkAtoms::code && mStructs && !currentNodeIsConverted) {
    Write(NS_LITERAL_STRING("|"));
  }
  else if ((aTag == nsGkAtoms::strong || aTag == nsGkAtoms::b)
           && mStructs && !currentNodeIsConverted) {
    Write(NS_LITERAL_STRING("*"));
  }
  else if ((aTag == nsGkAtoms::em || aTag == nsGkAtoms::i)
           && mStructs && !currentNodeIsConverted) {
    Write(NS_LITERAL_STRING("/"));
  }
  else if (aTag == nsGkAtoms::u && mStructs && !currentNodeIsConverted) {
    Write(NS_LITERAL_STRING("_"));
  }

  




  mInWhitespace = true;

  return NS_OK;
}

nsresult
nsPlainTextSerializer::DoCloseContainer(nsIAtom* aTag)
{
  if (ShouldReplaceContainerWithPlaceholder(mElement->NodeInfo()->NameAtom())) {
    mIgnoredChildNodeLevel--;
    return NS_OK;
  }
  if (IsIgnorableRubyAnnotation(aTag)) {
    mIgnoredChildNodeLevel--;
    return NS_OK;
  }

  if (mFlags & nsIDocumentEncoder::OutputForPlainTextClipboardCopy) {
    if (DoOutput() && IsInPre() && IsElementBlock(mElement)) {
      
      
      mPreformattedBlockBoundary = true;
    }
  }

  if (mFlags & nsIDocumentEncoder::OutputRaw) {
    
    
    
    

    return NS_OK;
  }

  if (mTagStackIndex > 0) {
    --mTagStackIndex;
  }

  if (mTagStackIndex >= mIgnoreAboveIndex) {
    if (mTagStackIndex == mIgnoreAboveIndex) {
      
      
      
      mIgnoreAboveIndex = (uint32_t)kNotFound;
    }
    return NS_OK;
  }

  
  if ((aTag == nsGkAtoms::body) || (aTag == nsGkAtoms::html)) {
    
    
    
    
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

  if (aTag == nsGkAtoms::tr) {
    PopBool(mHasWrittenCellsForRow);
    
    if (mFloatingLines < 0)
      mFloatingLines = 0;
    mLineBreakDue = true;
  }
  else if (((aTag == nsGkAtoms::li) ||
            (aTag == nsGkAtoms::dt)) &&
           (mFlags & nsIDocumentEncoder::OutputFormatted)) {
    
    if (mFloatingLines < 0)
      mFloatingLines = 0;
    mLineBreakDue = true;
  }
  else if (aTag == nsGkAtoms::pre) {
    mFloatingLines = GetLastBool(mIsInCiteBlockquote) ? 0 : 1;
    mLineBreakDue = true;
  }
  else if (aTag == nsGkAtoms::ul) {
    FlushLine();
    mIndent -= kIndentSizeList;
    if (--mULCount + mOLStackIndex == 0) {
      mFloatingLines = 1;
      mLineBreakDue = true;
    }
  }
  else if (aTag == nsGkAtoms::ol) {
    FlushLine(); 
    mIndent -= kIndentSizeList;
    NS_ASSERTION(mOLStackIndex, "Wrong OLStack level!");
    mOLStackIndex--;
    if (mULCount + mOLStackIndex == 0) {
      mFloatingLines = 1;
      mLineBreakDue = true;
    }
  }  
  else if (aTag == nsGkAtoms::dl) {
    mFloatingLines = 1;
    mLineBreakDue = true;
  }
  else if (aTag == nsGkAtoms::dd) {
    FlushLine();
    mIndent -= kIndentSizeDD;
  }
  else if (aTag == nsGkAtoms::span) {
    NS_ASSERTION(mSpanLevel, "Span level will be negative!");
    --mSpanLevel;
  }
  else if (aTag == nsGkAtoms::div) {
    if (mFloatingLines < 0)
      mFloatingLines = 0;
    mLineBreakDue = true;
  }
  else if (aTag == nsGkAtoms::blockquote) {
    FlushLine();    

    
    bool isInCiteBlockquote = PopBool(mIsInCiteBlockquote);

    if (isInCiteBlockquote) {
      NS_ASSERTION(mCiteQuoteLevel, "CiteQuote level will be negative!");
      mCiteQuoteLevel--;
      mFloatingLines = 0;
      mHasWrittenCiteBlockquote = true;
    }
    else {
      mIndent -= kTabSize;
      mFloatingLines = 1;
    }
    mLineBreakDue = true;
  }
  else if (aTag == nsGkAtoms::q) {
    Write(NS_LITERAL_STRING("\""));
  }
  else if (IsElementBlock(mElement) && aTag != nsGkAtoms::script) {
    
    
    
    
    if (mFlags & nsIDocumentEncoder::OutputFormatted)
      EnsureVerticalSpace(1);
    else {
      if (mFloatingLines < 0)
        mFloatingLines = 0;
      mLineBreakDue = true;
    }
  }

  
  if (!(mFlags & nsIDocumentEncoder::OutputFormatted)) {
    return NS_OK;
  }
  
  
  
  

  
  bool currentNodeIsConverted = IsCurrentNodeConverted();
  
  if (aTag == nsGkAtoms::h1 || aTag == nsGkAtoms::h2 ||
      aTag == nsGkAtoms::h3 || aTag == nsGkAtoms::h4 ||
      aTag == nsGkAtoms::h5 || aTag == nsGkAtoms::h6) {
    
    if (mHeaderStrategy) {   
      mIndent -= kIndentSizeHeaders;
    }
    if (mHeaderStrategy == 1  ) {
      for (int32_t i = HeaderLevel(aTag); i > 1; i--) {
           
        mIndent -= kIndentIncrementHeaders;
      }
    }
    EnsureVerticalSpace(1);
  }
  else if (aTag == nsGkAtoms::a && !currentNodeIsConverted && !mURL.IsEmpty()) {
    nsAutoString temp; 
    temp.AssignLiteral(" <");
    temp += mURL;
    temp.Append(char16_t('>'));
    Write(temp);
    mURL.Truncate();
  }
  else if ((aTag == nsGkAtoms::sup || aTag == nsGkAtoms::sub)
           && mStructs && !currentNodeIsConverted) {
    Write(kSpace);
  }
  else if (aTag == nsGkAtoms::code && mStructs && !currentNodeIsConverted) {
    Write(NS_LITERAL_STRING("|"));
  }
  else if ((aTag == nsGkAtoms::strong || aTag == nsGkAtoms::b)
           && mStructs && !currentNodeIsConverted) {
    Write(NS_LITERAL_STRING("*"));
  }
  else if ((aTag == nsGkAtoms::em || aTag == nsGkAtoms::i)
           && mStructs && !currentNodeIsConverted) {
    Write(NS_LITERAL_STRING("/"));
  }
  else if (aTag == nsGkAtoms::u && mStructs && !currentNodeIsConverted) {
    Write(NS_LITERAL_STRING("_"));
  }

  return NS_OK;
}

bool
nsPlainTextSerializer::MustSuppressLeaf()
{
  if (mIgnoredChildNodeLevel > 0) {
    return true;
  }

  if ((mTagStackIndex > 1 &&
       mTagStack[mTagStackIndex-2] == nsGkAtoms::select) ||
      (mTagStackIndex > 0 &&
        mTagStack[mTagStackIndex-1] == nsGkAtoms::select)) {
    
    
    
    return true;
  }

  if (mTagStackIndex > 0 &&
      (mTagStack[mTagStackIndex-1] == nsGkAtoms::script ||
       mTagStack[mTagStackIndex-1] == nsGkAtoms::style)) {
    
    return true;
  }

  return false;
}

void
nsPlainTextSerializer::DoAddText(bool aIsLineBreak, const nsAString& aText)
{
  
  if (!DoOutput()) {
    return;
  }

  if (!aIsLineBreak) {
    
    mHasWrittenCiteBlockquote = false;
  }

  if (mLineBreakDue)
    EnsureVerticalSpace(mFloatingLines);

  if (MustSuppressLeaf()) {
    return;
  }

  if (aIsLineBreak) {
    
    
    
    
    
    
    if ((mFlags & nsIDocumentEncoder::OutputPreformatted) ||
        (mPreFormattedMail && !mWrapColumn) ||
        IsInPre()) {
      EnsureVerticalSpace(mEmptyLines+1);
    }
    else if (!mInWhitespace) {
      Write(kSpace);
      mInWhitespace = true;
    }
    return;
  }

  


  if (!mURL.IsEmpty() && mURL.Equals(aText)) {
    mURL.Truncate();
  }
  Write(aText);
}

nsresult
nsPlainTextSerializer::DoAddLeaf(nsIAtom* aTag)
{
  mPreformattedBlockBoundary = false;

  
  if (!DoOutput()) {
    return NS_OK;
  }

  if (mLineBreakDue)
    EnsureVerticalSpace(mFloatingLines);

  if (MustSuppressLeaf()) {
    return NS_OK;
  }

  if (aTag == nsGkAtoms::br) {
    
    
    nsAutoString tagAttr;
    if (NS_FAILED(GetAttributeValue(nsGkAtoms::type, tagAttr))
        || !tagAttr.EqualsLiteral("_moz")) {
      EnsureVerticalSpace(mEmptyLines+1);
    }
  }
  else if (aTag == nsGkAtoms::hr &&
           (mFlags & nsIDocumentEncoder::OutputFormatted)) {
    EnsureVerticalSpace(0);

    
    
    nsAutoString line;
    uint32_t width = (mWrapColumn > 0 ? mWrapColumn : 25);
    while (line.Length() < width) {
      line.Append(char16_t('-'));
    }
    Write(line);

    EnsureVerticalSpace(0);
  }
  else if (mFlags & nsIDocumentEncoder::OutputNonTextContentAsPlaceholder) {
    Write(NS_LITERAL_STRING("\xFFFC"));
  }
  else if (aTag == nsGkAtoms::img) {
    

    
    nsAutoString imageDescription;
    if (NS_SUCCEEDED(GetAttributeValue(nsGkAtoms::alt,
                                       imageDescription))) {
      
    }
    else if (NS_SUCCEEDED(GetAttributeValue(nsGkAtoms::title,
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
nsPlainTextSerializer::EnsureVerticalSpace(int32_t noOfRows)
{
  
  
  
  if (noOfRows >= 0 && !mInIndentString.IsEmpty()) {
    EndLine(false);
    mInWhitespace = true;
  }

  while(mEmptyLines < noOfRows) {
    EndLine(false);
    mInWhitespace = true;
  }
  mLineBreakDue = false;
  mFloatingLines = -1;
}









void
nsPlainTextSerializer::FlushLine()
{
  if (!mCurrentLine.IsEmpty()) {
    if (mAtFirstColumn) {
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
    mStartedOutput = true;
  }

  if (!(mFlags & nsIDocumentEncoder::OutputPersistNBSP)) {
    
    
    aString.ReplaceChar(kNBSP, kSPACE);
  }
  mOutputString->Append(aString);
}

static bool
IsSpaceStuffable(const char16_t *s)
{
  if (s[0] == '>' || s[0] == ' ' || s[0] == kNBSP ||
      nsCRT::strncmp(s, MOZ_UTF16("From "), 5) == 0)
    return true;
  else
    return false;
}







void
nsPlainTextSerializer::AddToLine(const char16_t * aLineFragment, 
                                 int32_t aLineFragmentLength)
{
  uint32_t prefixwidth = (mCiteQuoteLevel > 0 ? mCiteQuoteLevel + 1:0)+mIndent;
  
  if (mLineBreakDue)
    EnsureVerticalSpace(mFloatingLines);

  int32_t linelength = mCurrentLine.Length();
  if (0 == linelength) {
    if (0 == aLineFragmentLength) {
      
      return;
    }

    if (mFlags & nsIDocumentEncoder::OutputFormatFlowed) {
      if (IsSpaceStuffable(aLineFragment)
         && mCiteQuoteLevel == 0  
         )
        {
          
          mCurrentLine.Append(char16_t(' '));
          
          if (MayWrap()) {
            mCurrentLineWidth += GetUnicharWidth(' ');
#ifdef DEBUG_wrapping
            NS_ASSERTION(GetUnicharStringWidth(mCurrentLine.get(),
                                               mCurrentLine.Length()) ==
                         (int32_t)mCurrentLineWidth,
                         "mCurrentLineWidth and reality out of sync!");
#endif
          }
        }
    }
    mEmptyLines=-1;
  }
    
  mCurrentLine.Append(aLineFragment, aLineFragmentLength);
  if (MayWrap()) {
    mCurrentLineWidth += GetUnicharStringWidth(aLineFragment,
                                               aLineFragmentLength);
#ifdef DEBUG_wrapping
    NS_ASSERTION(GetUnicharstringWidth(mCurrentLine.get(),
                                       mCurrentLine.Length()) ==
                 (int32_t)mCurrentLineWidth,
                 "mCurrentLineWidth and reality out of sync!");
#endif
  }

  linelength = mCurrentLine.Length();

  
  if (MayWrap())
  {
#ifdef DEBUG_wrapping
    NS_ASSERTION(GetUnicharstringWidth(mCurrentLine.get(),
                                  mCurrentLine.Length()) ==
                 (int32_t)mCurrentLineWidth,
                 "mCurrentLineWidth and reality out of sync!");
#endif
    
    
    
    
    uint32_t bonuswidth = (mWrapColumn > 20) ? 4 : 0;

    
    while(mCurrentLineWidth+prefixwidth > mWrapColumn+bonuswidth) {      
      
      
      int32_t goodSpace = mCurrentLine.Length();
      uint32_t width = mCurrentLineWidth;
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
          if ((uint32_t)goodSpace < mCurrentLine.Length())
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
      
      if ((goodSpace < linelength) && (goodSpace > 0)) {
        

        
        
        if (nsCRT::IsAsciiSpace(mCurrentLine.CharAt(goodSpace))) {
          mCurrentLine.Right(restOfLine, linelength-goodSpace-1);
        }
        else {
          mCurrentLine.Right(restOfLine, linelength-goodSpace);
        }
        
        bool breakBySpace = mCurrentLine.CharAt(goodSpace) == ' ';
        mCurrentLine.Truncate(goodSpace); 
        EndLine(true, breakBySpace);
        mCurrentLine.Truncate();
        
        if (mFlags & nsIDocumentEncoder::OutputFormatFlowed) {
          if (!restOfLine.IsEmpty() && IsSpaceStuffable(restOfLine.get())
              && mCiteQuoteLevel == 0  
            )
          {
            
            mCurrentLine.Append(char16_t(' '));
            
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
nsPlainTextSerializer::EndLine(bool aSoftlinebreak, bool aBreakBySpace)
{
  uint32_t currentlinelength = mCurrentLine.Length();

  if (aSoftlinebreak && 0 == currentlinelength) {
    
    return;
  }

  




  
  if (!(mFlags & nsIDocumentEncoder::OutputPreformatted) &&
      !(mFlags & nsIDocumentEncoder::OutputDontRemoveLineEndingSpaces) &&
     (aSoftlinebreak || 
     !(mCurrentLine.EqualsLiteral("-- ") || mCurrentLine.EqualsLiteral("- -- ")))) {
    
    while(currentlinelength > 0 &&
          mCurrentLine[currentlinelength-1] == ' ') {
      --currentlinelength;
    }
    mCurrentLine.SetLength(currentlinelength);
  }
  
  if (aSoftlinebreak &&
     (mFlags & nsIDocumentEncoder::OutputFormatFlowed) &&
     (mIndent == 0)) {
    
    
    

    
    
    if ((mFlags & nsIDocumentEncoder::OutputFormatDelSp) && aBreakBySpace)
      mCurrentLine.AppendLiteral("  ");
    else
      mCurrentLine.Append(char16_t(' '));
  }

  if (aSoftlinebreak) {
    mEmptyLines=0;
  } 
  else {
    
    if (!mCurrentLine.IsEmpty() || !mInIndentString.IsEmpty()) {
      mEmptyLines=-1;
    }

    mEmptyLines++;
  }

  if (mAtFirstColumn) {
    
    
    
    bool stripTrailingSpaces = mCurrentLine.IsEmpty();
    OutputQuotesAndIndent(stripTrailingSpaces);
  }

  mCurrentLine.Append(mLineBreak);
  Output(mCurrentLine);
  mCurrentLine.Truncate();
  mCurrentLineWidth = 0;
  mAtFirstColumn=true;
  mInWhitespace=true;
  mLineBreakDue = false;
  mFloatingLines = -1;
}







void
nsPlainTextSerializer::OutputQuotesAndIndent(bool stripTrailingSpaces )
{
  nsAutoString stringToOutput;
  
  
  if (mCiteQuoteLevel > 0) {
    nsAutoString quotes;
    for(int i=0; i < mCiteQuoteLevel; i++) {
      quotes.Append(char16_t('>'));
    }
    if (!mCurrentLine.IsEmpty()) {
      




      quotes.Append(char16_t(' '));
    }
    stringToOutput = quotes;
    mAtFirstColumn = false;
  }
  
  
  int32_t indentwidth = mIndent - mInIndentString.Length();
  if (indentwidth > 0
      && (!mCurrentLine.IsEmpty() || !mInIndentString.IsEmpty())
      
      ) {
    nsAutoString spaces;
    for (int i=0; i < indentwidth; ++i)
      spaces.Append(char16_t(' '));
    stringToOutput += spaces;
    mAtFirstColumn = false;
  }
  
  if (!mInIndentString.IsEmpty()) {
    stringToOutput += mInIndentString;
    mAtFirstColumn = false;
    mInIndentString.Truncate();
  }

  if (stripTrailingSpaces) {
    int32_t lineLength = stringToOutput.Length();
    while(lineLength > 0 &&
          ' ' == stringToOutput[lineLength-1]) {
      --lineLength;
    }
    stringToOutput.SetLength(lineLength);
  }

  if (!stringToOutput.IsEmpty()) {
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

  int32_t bol = 0;
  int32_t newline;
  
  int32_t totLen = str.Length();

  
  if (totLen <= 0) return;

  
  
  if (mFlags & nsIDocumentEncoder::OutputFormatFlowed) {
    for (int32_t i = totLen-1; i >= 0; i--) {
      char16_t c = str[i];
      if ('\n' == c || '\r' == c || ' ' == c || '\t' == c)
        continue;
      if (kNBSP == c)
        str.Replace(i, 1, ' ');
      else
        break;
    }
  }

  
  
  
  if ((mPreFormattedMail && !mWrapColumn) || (IsInPre() && !mPreFormattedMail)
      || ((mSpanLevel > 0 || mDontWrapAnyQuotes)
          && mEmptyLines >= 0 && str.First() == char16_t('>'))) {
    

    
    
    NS_ASSERTION(mCurrentLine.IsEmpty() || (IsInPre() && !mPreFormattedMail),
                 "Mixed wrapping data and nonwrapping data on the same line");
    if (!mCurrentLine.IsEmpty()) {
      FlushLine();
    }

    
    
    while(bol<totLen) {
      bool outputQuotes = mAtFirstColumn;
      bool atFirstColumn = mAtFirstColumn;
      bool outputLineBreak = false;
      bool spacesOnly = true;

      
      
      nsAString::const_iterator iter;           str.BeginReading(iter);
      nsAString::const_iterator done_searching; str.EndReading(done_searching);
      iter.advance(bol); 
      int32_t new_newline = bol;
      newline = kNotFound;
      while(iter != done_searching) {
        if ('\n' == *iter || '\r' == *iter) {
          newline = new_newline;
          break;
        }
        if (' ' != *iter)
          spacesOnly = false;
        ++new_newline;
        ++iter;
      }

      
      nsAutoString stringpart;
      if (newline == kNotFound) {
        
        stringpart.Assign(Substring(str, bol, totLen - bol));
        if (!stringpart.IsEmpty()) {
          char16_t lastchar = stringpart[stringpart.Length()-1];
          if ((lastchar == '\t') || (lastchar == ' ') ||
             (lastchar == '\r') ||(lastchar == '\n')) {
            mInWhitespace = true;
          } 
          else {
            mInWhitespace = false;
          }
        }
        mEmptyLines=-1;
        atFirstColumn = mAtFirstColumn && (totLen-bol)==0;
        bol = totLen;
      } 
      else {
        
        stringpart.Assign(Substring(str, bol, newline-bol));
        mInWhitespace = true;
        outputLineBreak = true;
        mEmptyLines=0;
        atFirstColumn = true;
        bol = newline+1;
        if ('\r' == *iter && bol < totLen && '\n' == *++iter) {
          
          
          
          bol++;
        }
      }

      mCurrentLine.Truncate();
      if (mFlags & nsIDocumentEncoder::OutputFormatFlowed) {
        if ((outputLineBreak || !spacesOnly) && 
            !stringpart.EqualsLiteral("-- ") &&
            !stringpart.EqualsLiteral("- -- "))
          stringpart.Trim(" ", false, true, true);
        if (IsSpaceStuffable(stringpart.get()) && stringpart[0] != '>')
          mCurrentLine.Append(char16_t(' '));
      }
      mCurrentLine.Append(stringpart);

      if (outputQuotes) {
        
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

  
  
  
  int32_t nextpos;
  const char16_t * offsetIntoBuffer = nullptr;
  
  while (bol < totLen) {    
    
    nextpos = str.FindCharInSet(" \t\n\r", bol);
#ifdef DEBUG_wrapping
    nsAutoString remaining;
    str.Right(remaining, totLen - bol);
    foo = ToNewCString(remaining);
    
    
    free(foo);
#endif

    if (nextpos == kNotFound) {
      
      offsetIntoBuffer = str.get() + bol;
      AddToLine(offsetIntoBuffer, totLen-bol);
      bol=totLen;
      mInWhitespace=false;
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
      
      if (mInWhitespace && (nextpos == bol) && !mPreFormattedMail &&
          !(mFlags & nsIDocumentEncoder::OutputPreformatted)) {
        
        bol++;
        continue;
      }

      if (nextpos == bol) {
        
        mInWhitespace = true;
        offsetIntoBuffer = str.get() + nextpos;
        AddToLine(offsetIntoBuffer, 1);
        bol++;
        continue;
      }
      
      mInWhitespace = true;
      
      offsetIntoBuffer = str.get() + bol;
      if (mPreFormattedMail || (mFlags & nsIDocumentEncoder::OutputPreformatted)) {
        
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
nsPlainTextSerializer::GetAttributeValue(nsIAtom* aName,
                                         nsString& aValueRet)
{
  if (mElement) {
    if (mElement->GetAttr(kNameSpaceID_None, aName, aValueRet)) {
      return NS_OK;
    }
  }

  return NS_ERROR_NOT_AVAILABLE;
}





bool 
nsPlainTextSerializer::IsCurrentNodeConverted()
{
  nsAutoString value;
  nsresult rv = GetAttributeValue(nsGkAtoms::_class, value);
  return (NS_SUCCEEDED(rv) &&
          (value.EqualsIgnoreCase("moz-txt", 7) ||
           value.EqualsIgnoreCase("\"moz-txt", 8)));
}



nsIAtom*
nsPlainTextSerializer::GetIdForContent(nsIContent* aContent)
{
  if (!aContent->IsHTMLElement()) {
    return nullptr;
  }

  nsIAtom* localName = aContent->NodeInfo()->NameAtom();
  return localName->IsStaticAtom() ? localName : nullptr;
}

bool
nsPlainTextSerializer::IsInPre()
{
  return !mPreformatStack.empty() && mPreformatStack.top();
}

bool
nsPlainTextSerializer::IsElementPreformatted(Element* aElement)
{
  nsRefPtr<nsStyleContext> styleContext =
    nsComputedDOMStyle::GetStyleContextForElementNoFlush(aElement, nullptr,
                                                         nullptr);
  if (styleContext) {
    const nsStyleText* textStyle = styleContext->StyleText();
    return textStyle->WhiteSpaceOrNewlineIsSignificant();
  }
  
  return GetIdForContent(aElement) == nsGkAtoms::pre;
}

bool
nsPlainTextSerializer::IsElementBlock(Element* aElement)
{
  nsRefPtr<nsStyleContext> styleContext =
    nsComputedDOMStyle::GetStyleContextForElementNoFlush(aElement, nullptr,
                                                         nullptr);
  if (styleContext) {
    const nsStyleDisplay* displayStyle = styleContext->StyleDisplay();
    return displayStyle->IsBlockOutsideStyle();
  }
  
  return nsContentUtils::IsHTMLBlock(aElement);
}





bool
nsPlainTextSerializer::IsInOL()
{
  int32_t i = mTagStackIndex;
  while(--i >= 0) {
    if (mTagStack[i] == nsGkAtoms::ol)
      return true;
    if (mTagStack[i] == nsGkAtoms::ul) {
      
      return false;
    }
  }
  
  return false;
}




int32_t HeaderLevel(nsIAtom* aTag)
{
  if (aTag == nsGkAtoms::h1) {
    return 1;
  }
  if (aTag == nsGkAtoms::h2) {
    return 2;
  }
  if (aTag == nsGkAtoms::h3) {
    return 3;
  }
  if (aTag == nsGkAtoms::h4) {
    return 4;
  }
  if (aTag == nsGkAtoms::h5) {
    return 5;
  }
  if (aTag == nsGkAtoms::h6) {
    return 6;
  }
  return 0;
}





































namespace {

struct interval
{
  uint16_t first;
  uint16_t last;
};

struct CombiningComparator
{
  const char16_t mUcs;
  explicit CombiningComparator(char16_t aUcs) : mUcs(aUcs) {}
  int operator()(const interval& combining) const {
    if (mUcs > combining.last)
      return 1;
    if (mUcs < combining.first)
      return -1;

    MOZ_ASSERT(combining.first <= mUcs);
    MOZ_ASSERT(mUcs <= combining.last);
    return 0;
  }
};

} 

int32_t GetUnicharWidth(char16_t ucs)
{
  
  static const interval combining[] = {
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

  
  if (ucs == 0)
    return 0;
  if (ucs < 32 || (ucs >= 0x7f && ucs < 0xa0))
    return -1;

  
  if (ucs < combining[0].first)
    return 1;

  
  size_t idx;
  if (BinarySearchIf(combining, 0, ArrayLength(combining),
                     CombiningComparator(ucs), &idx)) {
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


int32_t GetUnicharStringWidth(const char16_t* pwcs, int32_t n)
{
  int32_t w, width = 0;

  for (;*pwcs && n-- > 0; pwcs++)
    if ((w = GetUnicharWidth(*pwcs)) < 0)
      ++width; 
    else
      width += w;

  return width;
}


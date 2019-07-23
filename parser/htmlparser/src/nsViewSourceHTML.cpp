














































#define NS_VIEWSOURCE_TOKENS_PER_BLOCK 16

#include "nsIAtom.h"
#include "nsViewSourceHTML.h"
#include "nsCRT.h"
#include "nsParser.h"
#include "nsScanner.h"
#include "nsDTDUtils.h"
#include "nsIContentSink.h"
#include "nsIHTMLContentSink.h"
#include "nsHTMLTokenizer.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsUnicharUtils.h"
#include "nsPrintfCString.h"
#include "nsNetUtil.h"
#include "nsHTMLEntities.h"

#include "nsIServiceManager.h"

#include "nsElementTable.h"

#include "prenv.h"  
#include "prtypes.h"  
#include "prio.h"
#include "plstr.h"
#include "prmem.h"

#ifdef RAPTOR_PERF_METRICS
#include "stopwatch.h"
Stopwatch vsTimer;
#endif





#ifdef DUMP_TO_FILE
#include <stdio.h>
  FILE* gDumpFile=0;
  static const char* gDumpFileName = "/tmp/viewsource.html";

#endif 




static const char kBodyId[] = "viewsource";
static const char kBodyClassWrap[] = "wrap";

NS_IMPL_ISUPPORTS1(CViewSourceHTML, nsIDTD)




enum {
  kStartTag = 0,
  kEndTag,
  kComment,
  kCData,
  kDoctype,
  kPI,
  kEntity,
  kText,
  kAttributeName,
  kAttributeValue,
  kMarkupDecl
};

static const char* const kElementClasses[] = {
  "start-tag",
  "end-tag",
  "comment",
  "cdata",
  "doctype",
  "pi",
  "entity",
  "text",
  "attribute-name",
  "attribute-value",
  "markupdeclaration"
};

static const char* const kBeforeText[] = {
  "<",
  "</",
  "",
  "",
  "",
  "",
  "&",
  "",
  "",
  "=",
  ""
};

static const char* const kAfterText[] = {
  ">",
  ">",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  ""
};

#ifdef DUMP_TO_FILE
static const char* const kDumpFileBeforeText[] = {
  "&lt;",
  "&lt;/",
  "",
  "",
  "",
  "",
  "&amp;",
  "",
  "",
  "=",
  ""
};

static const char* const kDumpFileAfterText[] = {
  "&gt;",
  "&gt;",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  ""
};
#endif 








CViewSourceHTML::CViewSourceHTML()
{
  mSyntaxHighlight = PR_FALSE;
  mWrapLongLines = PR_FALSE;
  nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (prefBranch) {
    PRBool temp;
    nsresult rv;
    rv = prefBranch->GetBoolPref("view_source.syntax_highlight", &temp);
    mSyntaxHighlight = NS_SUCCEEDED(rv) ? temp : PR_TRUE;

    rv = prefBranch->GetBoolPref("view_source.wrap_long_lines", &temp);
    mWrapLongLines = NS_SUCCEEDED(rv) ? temp : PR_FALSE;
  }

  mSink = 0;
  mLineNumber = 1;
  mTokenizer = 0;
  mDocType=eHTML_Quirks;
  mHasOpenRoot=PR_FALSE;
  mHasOpenBody=PR_FALSE;

  mTokenCount=0;

#ifdef DUMP_TO_FILE
  gDumpFile = fopen(gDumpFileName,"w");
#endif 

}










CViewSourceHTML::~CViewSourceHTML(){
  mSink=0; 
}










NS_IMETHODIMP
CViewSourceHTML::WillBuildModel(const CParserContext& aParserContext,
                                nsITokenizer* aTokenizer,
                                nsIContentSink* aSink)
{
  nsresult result=NS_OK;

#ifdef RAPTOR_PERF_METRICS
  vsTimer.Reset();
  NS_START_STOPWATCH(vsTimer);
#endif

  mSink=(nsIHTMLContentSink*)aSink;

  if((!aParserContext.mPrevContext) && (mSink)) {

    nsAString & contextFilename = aParserContext.mScanner->GetFilename();
    mFilename = Substring(contextFilename,
                          12, 
                          contextFilename.Length() - 12);

    mDocType=aParserContext.mDocType;
    mMimeType=aParserContext.mMimeType;
    mDTDMode=aParserContext.mDTDMode;
    mParserCommand=aParserContext.mParserCommand;
    mTokenizer = aTokenizer;

#ifdef DUMP_TO_FILE
    if (gDumpFile) {

      fprintf(gDumpFile, "<html>\n");
      fprintf(gDumpFile, "<head>\n");
      fprintf(gDumpFile, "<title>");
      fprintf(gDumpFile, "Source of: ");
      fputs(NS_ConvertUTF16toUTF8(mFilename).get(), gDumpFile);
      fprintf(gDumpFile, "</title>\n");
      fprintf(gDumpFile, "<link rel=\"stylesheet\" type=\"text/css\" href=\"resource://gre/res/viewsource.css\">\n");
      fprintf(gDumpFile, "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">\n");
      fprintf(gDumpFile, "</head>\n");
      fprintf(gDumpFile, "<body id=\"viewsource\">\n");
      fprintf(gDumpFile, "<pre id=\"line1\">\n");
    }
#endif 
  }


  if(eViewSource!=aParserContext.mParserCommand)
    mDocType=ePlainText;
  else mDocType=aParserContext.mDocType;

  mLineNumber = 1;

  return result;
}









NS_IMETHODIMP CViewSourceHTML::BuildModel(nsITokenizer* aTokenizer,
                                          PRBool aCanInterrupt,
                                          PRBool aCountLines,
                                          const nsCString* aCharsetPtr)
{
  nsresult result=NS_OK;

  if(aTokenizer) {

    nsITokenizer*  oldTokenizer=mTokenizer;
    mTokenizer=aTokenizer;
    nsTokenAllocator* theAllocator=mTokenizer->GetTokenAllocator();

    if(!mHasOpenRoot) {
      
      
      CStartToken htmlToken(NS_LITERAL_STRING("HTML"), eHTMLTag_html);
      nsCParserNode htmlNode(&htmlToken, 0);
      mSink->OpenContainer(htmlNode);

      CStartToken headToken(NS_LITERAL_STRING("HEAD"), eHTMLTag_head);
      nsCParserNode headNode(&headToken, 0);
      mSink->OpenContainer(headNode);

      CStartToken titleToken(NS_LITERAL_STRING("TITLE"), eHTMLTag_title);
      nsCParserNode titleNode(&titleToken, 0);
      mSink->OpenContainer(titleNode);

      
      if (StringBeginsWith(mFilename, NS_LITERAL_STRING("data:")) &&
          mFilename.Length() > 50) {
        nsAutoString dataFilename(Substring(mFilename, 0, 50));
        dataFilename.AppendLiteral("...");
        CTextToken titleText(dataFilename);
        nsCParserNode titleTextNode(&titleText, 0);
        mSink->AddLeaf(titleTextNode);
      } else {
        CTextToken titleText(mFilename);
        nsCParserNode titleTextNode(&titleText, 0);
        mSink->AddLeaf(titleTextNode);
      }

      mSink->CloseContainer(eHTMLTag_title);

      if (theAllocator) {
        CStartToken* theToken=
          static_cast<CStartToken*>
                     (theAllocator->CreateTokenOfType(eToken_start,
                                                         eHTMLTag_link,
                                                         NS_LITERAL_STRING("LINK")));
        if (theToken) {
          nsCParserStartNode theNode(theToken, theAllocator);

          AddAttrToNode(theNode, theAllocator,
                        NS_LITERAL_STRING("rel"),
                        NS_LITERAL_STRING("stylesheet"));

          AddAttrToNode(theNode, theAllocator,
                        NS_LITERAL_STRING("type"),
                        NS_LITERAL_STRING("text/css"));

          AddAttrToNode(theNode, theAllocator,
                        NS_LITERAL_STRING("href"),
                        NS_LITERAL_STRING("resource://gre/res/viewsource.css"));

          mSink->AddLeaf(theNode);
        }
        IF_FREE(theToken, theAllocator);
      }

      result = mSink->CloseContainer(eHTMLTag_head);
      if(NS_SUCCEEDED(result)) {
        mHasOpenRoot = PR_TRUE;
      }
    }
    if (NS_SUCCEEDED(result) && !mHasOpenBody) {
      if (theAllocator) {
        CStartToken* bodyToken=
          static_cast<CStartToken*>
                     (theAllocator->CreateTokenOfType(eToken_start,
                                                         eHTMLTag_body,
                                                         NS_LITERAL_STRING("BODY")));
        if (bodyToken) {
          nsCParserStartNode bodyNode(bodyToken, theAllocator);

          AddAttrToNode(bodyNode, theAllocator,
                        NS_LITERAL_STRING("id"),
                        NS_ConvertASCIItoUTF16(kBodyId));

          if (mWrapLongLines) {
            AddAttrToNode(bodyNode, theAllocator,
                          NS_LITERAL_STRING("class"),
                          NS_ConvertASCIItoUTF16(kBodyClassWrap));
          }
          result = mSink->OpenContainer(bodyNode);
          if(NS_SUCCEEDED(result)) mHasOpenBody=PR_TRUE;
        }
        IF_FREE(bodyToken, theAllocator);

        if (NS_SUCCEEDED(result)) {
          CStartToken* preToken =
            static_cast<CStartToken*>
                       (theAllocator->CreateTokenOfType(eToken_start,
                                                           eHTMLTag_pre,
                                                           NS_LITERAL_STRING("PRE")));
          if (preToken) {
            nsCParserStartNode preNode(preToken, theAllocator);
            AddAttrToNode(preNode, theAllocator,
                          NS_LITERAL_STRING("id"),
                          NS_LITERAL_STRING("line1"));
            result = mSink->OpenContainer(preNode);
          } else {
            result = NS_ERROR_OUT_OF_MEMORY;
          }
          IF_FREE(preToken, theAllocator);
        }
      }
    }

    NS_ASSERTION(aCharsetPtr, "CViewSourceHTML::BuildModel expects a charset!");
    mCharset = *aCharsetPtr;

    NS_ASSERTION(aCanInterrupt, "CViewSourceHTML can't run scripts, so "
                 "document.write should not forbid interruptions. Why is "
                 "the parser telling us not to interrupt?");

    while(NS_SUCCEEDED(result)){
      CToken* theToken=mTokenizer->PopToken();
      if(theToken) {
        result=HandleToken(theToken);
        if(NS_SUCCEEDED(result)) {
          IF_FREE(theToken, mTokenizer->GetTokenAllocator());
          if (mSink->DidProcessAToken() == NS_ERROR_HTMLPARSER_INTERRUPTED) {
            result = NS_ERROR_HTMLPARSER_INTERRUPTED;
            break;
          }
        } else {
          mTokenizer->PushTokenFront(theToken);
        }
      }
      else break;
    }

    mTokenizer=oldTokenizer;
  }
  else result=NS_ERROR_HTMLPARSER_BADTOKENIZER;
  return result;
}





void CViewSourceHTML::StartNewPreBlock(void){
  CEndToken endToken(eHTMLTag_pre);
  nsCParserNode endNode(&endToken, 0);
  mSink->CloseContainer(eHTMLTag_pre);

  nsTokenAllocator* theAllocator = mTokenizer->GetTokenAllocator();
  if (!theAllocator) {
    return;
  }

  CStartToken* theToken =
    static_cast<CStartToken*>
               (theAllocator->CreateTokenOfType(eToken_start,
                                                   eHTMLTag_pre,
                                                   NS_LITERAL_STRING("PRE")));
  if (!theToken) {
    return;
  }

  nsCParserStartNode startNode(theToken, theAllocator);
  AddAttrToNode(startNode, theAllocator,
                NS_LITERAL_STRING("id"),
                NS_ConvertASCIItoUTF16(nsPrintfCString("line%d", mLineNumber)));
  mSink->OpenContainer(startNode);
  IF_FREE(theToken, theAllocator);

#ifdef DUMP_TO_FILE
  if (gDumpFile) {
    fprintf(gDumpFile, "</pre>\n");
    fprintf(gDumpFile, "<pre id=\"line%d\">\n", mLineNumber);
  }
#endif 

  mTokenCount = 0;
}

void CViewSourceHTML::AddAttrToNode(nsCParserStartNode& aNode,
                                    nsTokenAllocator* aAllocator,
                                    const nsAString& aAttrName,
                                    const nsAString& aAttrValue)
{
  NS_PRECONDITION(aAllocator, "Must have a token allocator!");

  CAttributeToken* theAttr =
    (CAttributeToken*) aAllocator->CreateTokenOfType(eToken_attribute,
                                                     eHTMLTag_unknown,
                                                     aAttrValue);
  if (!theAttr) {
    NS_ERROR("Failed to allocate attribute token");
    return;
  }

  theAttr->SetKey(aAttrName);
  aNode.AddAttribute(theAttr);

  
  
  
}







NS_IMETHODIMP CViewSourceHTML::DidBuildModel(nsresult anErrorCode)
{
  nsresult result= NS_OK;

  

  if (mSink) {
      

#ifdef DUMP_TO_FILE
    if(gDumpFile) {
      fprintf(gDumpFile, "</pre>\n");
      fprintf(gDumpFile, "</body>\n");
      fprintf(gDumpFile, "</html>\n");
      fclose(gDumpFile);
    }
#endif 

    if(ePlainText!=mDocType) {
      mSink->CloseContainer(eHTMLTag_pre);
      mSink->CloseContainer(eHTMLTag_body);
      mSink->CloseContainer(eHTMLTag_html);
    }
  }

#ifdef RAPTOR_PERF_METRICS
  NS_STOP_STOPWATCH(vsTimer);
  printf("viewsource timer: ");
  vsTimer.Print();
  printf("\n");
#endif

  return result;
}











NS_IMETHODIMP_(void)
CViewSourceHTML::Terminate() {
}

NS_IMETHODIMP_(PRInt32)
CViewSourceHTML::GetType() {
  return NS_IPARSER_FLAG_HTML;
}

NS_IMETHODIMP_(nsDTDMode)
CViewSourceHTML::GetMode() const
{
  
  
  return eDTDMode_full_standards;
}








void CViewSourceHTML::SetVerification(PRBool aEnabled)
{
}










NS_IMETHODIMP_(PRBool)
CViewSourceHTML::CanContain(PRInt32 aParent, PRInt32 aChild) const
{
  PRBool result=PR_TRUE;
  return result;
}









NS_IMETHODIMP_(PRBool)
CViewSourceHTML::IsContainer(PRInt32 aTag) const
{
  PRBool result=PR_TRUE;
  return result;
}








nsresult CViewSourceHTML::WriteAttributes(const nsAString& tagName, 
                                          nsTokenAllocator* allocator, 
                                          PRInt32 attrCount, PRBool aOwnerInError) {
  nsresult result=NS_OK;

  if(attrCount){ 
    int attr = 0;
    for(attr = 0; attr < attrCount; ++attr){
      CToken* theToken = mTokenizer->PeekToken();
      if(theToken)  {
        eHTMLTokenTypes theType = eHTMLTokenTypes(theToken->GetTokenType());
        if(eToken_attribute == theType){
          mTokenizer->PopToken(); 
          mTokenNode.AddAttribute(theToken);  

          CAttributeToken* theAttrToken = (CAttributeToken*)theToken;
          const nsSubstring& theKey = theAttrToken->GetKey();

          
          const PRBool attributeInError =
            !aOwnerInError && theAttrToken->IsInError();

          result = WriteTag(kAttributeName,theKey,0,attributeInError);
          const nsSubstring& theValue = theAttrToken->GetValue();

          if(!theValue.IsEmpty() || theAttrToken->mHasEqualWithoutValue){
            if (IsUrlAttribute(tagName, theKey, theValue)) {
              WriteHrefAttribute(allocator, theValue);
            } else {
              WriteTag(kAttributeValue,theValue,0,attributeInError);
            }
          }
        }
      }
      else return kEOF;
    }
  }

  return result;
}








nsresult CViewSourceHTML::WriteTag(PRInt32 aTagType,const nsSubstring & aText,PRInt32 attrCount,PRBool aTagInError) {
  nsresult result=NS_OK;

  
  
  
  
  
  
  
  
  mLineNumber += aText.CountChar(PRUnichar('\n'));

  nsTokenAllocator* theAllocator=mTokenizer->GetTokenAllocator();
  NS_ASSERTION(0!=theAllocator,"Error: no allocator");
  if(0==theAllocator)
    return NS_ERROR_FAILURE;

  
  if (mSyntaxHighlight && aTagInError) {
    CStartToken* theTagToken=
      static_cast<CStartToken*>
                 (theAllocator->CreateTokenOfType(eToken_start,
                                                     eHTMLTag_span,
                                                     NS_LITERAL_STRING("SPAN")));
    NS_ENSURE_TRUE(theTagToken, NS_ERROR_OUT_OF_MEMORY);
    mErrorNode.Init(theTagToken, theAllocator);
    AddAttrToNode(mErrorNode, theAllocator,
                  NS_LITERAL_STRING("class"),
                  NS_LITERAL_STRING("error"));
    mSink->OpenContainer(mErrorNode);
    IF_FREE(theTagToken, theAllocator);
#ifdef DUMP_TO_FILE
    if (gDumpFile) {
      fprintf(gDumpFile, "<span class=\"error\">");
    }
#endif
  }

  if (kBeforeText[aTagType][0] != 0) {
    NS_ConvertASCIItoUTF16 beforeText(kBeforeText[aTagType]);
    mITextToken.SetIndirectString(beforeText);
    nsCParserNode theNode(&mITextToken, 0);
    mSink->AddLeaf(theNode);
  }
#ifdef DUMP_TO_FILE
  if (gDumpFile && kDumpFileBeforeText[aTagType][0])
    fprintf(gDumpFile, kDumpFileBeforeText[aTagType]);
#endif 

  if (mSyntaxHighlight && aTagType != kText) {
    CStartToken* theTagToken=
      static_cast<CStartToken*>
                 (theAllocator->CreateTokenOfType(eToken_start,
                                                     eHTMLTag_span,
                                                     NS_LITERAL_STRING("SPAN")));
    NS_ENSURE_TRUE(theTagToken, NS_ERROR_OUT_OF_MEMORY);
    mStartNode.Init(theTagToken, theAllocator);
    AddAttrToNode(mStartNode, theAllocator,
                  NS_LITERAL_STRING("class"),
                  NS_ConvertASCIItoUTF16(kElementClasses[aTagType]));
    mSink->OpenContainer(mStartNode);  
    IF_FREE(theTagToken, theAllocator);
#ifdef DUMP_TO_FILE
    if (gDumpFile) {
      fprintf(gDumpFile, "<span class=\"");
      fprintf(gDumpFile, kElementClasses[aTagType]);
      fprintf(gDumpFile, "\">");
    }
#endif 
  }

  mITextToken.SetIndirectString(aText);  

  nsCParserNode theNode(&mITextToken, 0);
  mSink->AddLeaf(theNode);
#ifdef DUMP_TO_FILE
  if (gDumpFile) {
    fputs(NS_ConvertUTF16toUTF8(aText).get(), gDumpFile);
  }
#endif 

  if (mSyntaxHighlight && aTagType != kText) {
    mStartNode.ReleaseAll();
    mSink->CloseContainer(eHTMLTag_span);  
#ifdef DUMP_TO_FILE
    if (gDumpFile)
      fprintf(gDumpFile, "</span>");
#endif 
  }

  if(attrCount){
    result=WriteAttributes(aText, theAllocator, attrCount, aTagInError);
  }

  
  
  if (!aTagInError && kAfterText[aTagType][0] != 0) {
    NS_ConvertASCIItoUTF16 afterText(kAfterText[aTagType]);
    mITextToken.SetIndirectString(afterText);
    nsCParserNode theNode(&mITextToken, 0);
    mSink->AddLeaf(theNode);
  }
#ifdef DUMP_TO_FILE
  if (!aTagInError && gDumpFile && kDumpFileAfterText[aTagType][0])
    fprintf(gDumpFile, kDumpFileAfterText[aTagType]);
#endif 

  if (mSyntaxHighlight && aTagInError) {
    mErrorNode.ReleaseAll();
    mSink->CloseContainer(eHTMLTag_span);  
#ifdef DUMP_TO_FILE
    if (gDumpFile)
      fprintf(gDumpFile, "</span>");
#endif 
  }

  return result;
}







nsresult
CViewSourceHTML::HandleToken(CToken* aToken)
{
  nsresult        result=NS_OK;
  CHTMLToken*     theToken= (CHTMLToken*)(aToken);
  eHTMLTokenTypes theType= (eHTMLTokenTypes)theToken->GetTokenType();

  NS_ASSERTION(mSink, "No sink in CViewSourceHTML::HandleToken? Was WillBuildModel called?");

  mTokenNode.Init(theToken, mTokenizer->GetTokenAllocator());

  switch(theType) {

    case eToken_start:
      {
        const nsSubstring& startValue = aToken->GetStringValue();
        result = WriteTag(kStartTag,startValue,aToken->GetAttributeCount(),aToken->IsInError());

        if((ePlainText!=mDocType) && (NS_OK==result)) {
          result = mSink->NotifyTagObservers(&mTokenNode);
        }
      }
      break;

    case eToken_end:
      {
        const nsSubstring& endValue = aToken->GetStringValue();
        result = WriteTag(kEndTag,endValue,aToken->GetAttributeCount(),aToken->IsInError());
      }
      break;

    case eToken_cdatasection:
      {
        nsAutoString theStr;
        theStr.AssignLiteral("<!");
        theStr.Append(aToken->GetStringValue());
        if (!aToken->IsInError()) {
          theStr.AppendLiteral(">");
        }
        result=WriteTag(kCData,theStr,0,aToken->IsInError());
      }
      break;

    case eToken_markupDecl:
      {
        nsAutoString theStr;
        theStr.AssignLiteral("<!");
        theStr.Append(aToken->GetStringValue());
        if (!aToken->IsInError()) {
          theStr.AppendLiteral(">");
        }
        result=WriteTag(kMarkupDecl,theStr,0,aToken->IsInError());
      }
      break;

    case eToken_comment:
      {
        nsAutoString theStr;
        aToken->AppendSourceTo(theStr);
        result=WriteTag(kComment,theStr,0,aToken->IsInError());
      }
      break;

    case eToken_doctypeDecl:
      {
        const nsSubstring& doctypeValue = aToken->GetStringValue();
        result=WriteTag(kDoctype,doctypeValue,0,aToken->IsInError());
      }
      break;

    case eToken_newline:
      {
        const nsSubstring& newlineValue = aToken->GetStringValue();
        result=WriteTag(kText,newlineValue,0,PR_FALSE);
        ++mTokenCount;
        if (NS_VIEWSOURCE_TOKENS_PER_BLOCK > 0 &&
            mTokenCount > NS_VIEWSOURCE_TOKENS_PER_BLOCK) {
          StartNewPreBlock();
        }
      }
      break;

    case eToken_whitespace:
      {
        const nsSubstring& wsValue = aToken->GetStringValue();
        result=WriteTag(kText,wsValue,0,PR_FALSE);
        ++mTokenCount;
        if (NS_VIEWSOURCE_TOKENS_PER_BLOCK > 0 &&
            mTokenCount > NS_VIEWSOURCE_TOKENS_PER_BLOCK &&
            !wsValue.IsEmpty()) {
          PRUnichar ch = wsValue.Last();
          if (ch == kLF || ch == kCR)
            StartNewPreBlock();
        }
      }
      break;

    case eToken_text:
      {
        const nsSubstring& str = aToken->GetStringValue();
        result=WriteTag(kText,str,aToken->GetAttributeCount(),aToken->IsInError());
        ++mTokenCount;
        if (NS_VIEWSOURCE_TOKENS_PER_BLOCK > 0 &&
            mTokenCount > NS_VIEWSOURCE_TOKENS_PER_BLOCK && !str.IsEmpty()) {
          PRUnichar ch = str.Last();
          if (ch == kLF || ch == kCR)
            StartNewPreBlock();
        }
      }

      break;

    case eToken_entity:
      result=WriteTag(kEntity,aToken->GetStringValue(),0,aToken->IsInError());
      break;

    case eToken_instruction:
      result=WriteTag(kPI,aToken->GetStringValue(),0,aToken->IsInError());
      break;

    default:
      result=NS_OK;
  }

  mTokenNode.ReleaseAll();

  return result;
}

PRBool CViewSourceHTML::IsUrlAttribute(const nsAString& tagName,
                                       const nsAString& attrName, 
                                       const nsAString& attrValue) {
  const nsSubstring &trimmedAttrName = TrimTokenValue(attrName);

  PRBool isHref = trimmedAttrName.LowerCaseEqualsLiteral("href");
  PRBool isSrc = !isHref && trimmedAttrName.LowerCaseEqualsLiteral("src");

  
  
  
  if (isHref && tagName.LowerCaseEqualsLiteral("base")) {
    const nsAString& baseSpec = TrimTokenValue(attrValue);
    nsAutoString expandedBaseSpec;
    ExpandEntities(baseSpec, expandedBaseSpec);
    SetBaseURI(expandedBaseSpec);
  }

  return isHref || isSrc;
}

void CViewSourceHTML::WriteHrefAttribute(nsTokenAllocator* allocator,
                                         const nsAString& href) {
  
  
  
  nsAString::const_iterator startProper, endProper;
  href.BeginReading(startProper);
  href.EndReading(endProper);
  TrimTokenValue(startProper, endProper);

  
  
  nsAString::const_iterator start, end;
  href.BeginReading(start);
  href.EndReading(end);  
  const nsAString &precedingText = Substring(start, startProper);
  const nsAString &hrefProper = Substring(startProper, endProper);
  const nsAString &succeedingText = Substring(endProper, end);

  nsAutoString fullPrecedingText;
  fullPrecedingText.Assign(kEqual);
  fullPrecedingText.Append(precedingText);

  
  
  
  
  
  
  
  
  
  

  
  nsAutoString viewSourceUrl;
  CreateViewSourceURL(hrefProper, viewSourceUrl);

  
  if (viewSourceUrl.IsEmpty()) {
    nsAutoString equalsHref(kEqual);
    equalsHref.Append(href);
    WriteTextInSpan(equalsHref, allocator, EmptyString(), EmptyString());
  } else {
    NS_NAMED_LITERAL_STRING(HREF, "href");
    if (fullPrecedingText.Length() > 0) {
      WriteTextInSpan(fullPrecedingText, allocator, EmptyString(), EmptyString());
    }
    WriteTextInAnchor(hrefProper, allocator, HREF, viewSourceUrl);
    if (succeedingText.Length() > 0) {
      WriteTextInSpan(succeedingText, allocator, EmptyString(), EmptyString());
    }
  }
}

nsresult CViewSourceHTML::CreateViewSourceURL(const nsAString& linkUrl,
                                              nsString& viewSourceUrl) {
  nsCOMPtr<nsIURI> baseURI;
  nsCOMPtr<nsIURI> hrefURI;
  nsresult rv;

  
  viewSourceUrl.Truncate();

  
  rv = GetBaseURI(getter_AddRefs(baseURI));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsAutoString expandedLinkUrl;
  ExpandEntities(linkUrl, expandedLinkUrl);
  rv = NS_NewURI(getter_AddRefs(hrefURI), expandedLinkUrl, mCharset.get(), baseURI);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCString absoluteLinkUrl;
  hrefURI->GetSpec(absoluteLinkUrl);

  
  
  
  
  PRBool openingExecutesScript = PR_FALSE;
  rv = NS_URIChainHasFlags(hrefURI, nsIProtocolHandler::URI_OPENING_EXECUTES_SCRIPT,
                           &openingExecutesScript);
  NS_ENSURE_SUCCESS(rv, NS_OK); 
  if (openingExecutesScript) {
    return NS_OK;
  }

  
  
  
  PRBool doesNotReturnData = PR_FALSE;
  rv = NS_URIChainHasFlags(hrefURI, nsIProtocolHandler::URI_DOES_NOT_RETURN_DATA,
                           &doesNotReturnData);
  NS_ENSURE_SUCCESS(rv, NS_OK);  
  if (!doesNotReturnData) {
    viewSourceUrl.AssignLiteral("view-source:");    
  }

  viewSourceUrl.AppendWithConversion(absoluteLinkUrl);

  return NS_OK;
}

void CViewSourceHTML::WriteTextInSpan(const nsAString& text, 
                                      nsTokenAllocator* allocator, 
                                      const nsAString& attrName, 
                                      const nsAString& attrValue) {
  NS_NAMED_LITERAL_STRING(SPAN, "SPAN");
  WriteTextInElement(SPAN, eHTMLTag_span, text, allocator, attrName, attrValue);
}

void CViewSourceHTML::WriteTextInAnchor(const nsAString& text, 
                                        nsTokenAllocator* allocator, 
                                        const nsAString& attrName, 
                                        const nsAString& attrValue) {
  NS_NAMED_LITERAL_STRING(ANCHOR, "A");
  WriteTextInElement(ANCHOR, eHTMLTag_a, text, allocator, attrName, attrValue);
}

void CViewSourceHTML::WriteTextInElement(const nsAString& tagName, 
                                         eHTMLTags tagType, const nsAString& text,
                                         nsTokenAllocator* allocator,
                                         const nsAString& attrName, 
                                         const nsAString& attrValue) {
  
  nsTokenAllocator* theAllocator = mTokenizer->GetTokenAllocator();
  if (!theAllocator) {
    return;
  }

  CStartToken* startToken =
    static_cast<CStartToken*>
      (theAllocator->CreateTokenOfType(eToken_start, tagType, tagName));
  if (!startToken) {
    return;
  }

  nsCParserStartNode startNode(startToken, theAllocator);
  if (!attrName.IsEmpty()) {
    AddAttrToNode(startNode, allocator, attrName, attrValue);
  }
  mSink->OpenContainer(startNode);
  IF_FREE(startToken, theAllocator);

  
  CTextToken textToken(text);
  nsCParserNode textNode(&textToken, 0);
  mSink->AddLeaf(textNode);

  
  mSink->CloseContainer(tagType);
}

const nsDependentSubstring CViewSourceHTML::TrimTokenValue(const nsAString& tokenValue) {
  nsAString::const_iterator start, end;
  tokenValue.BeginReading(start);
  tokenValue.EndReading(end);
  TrimTokenValue(start, end);
  return Substring(start, end);
}

void CViewSourceHTML::TrimTokenValue(nsAString::const_iterator& start,
                                     nsAString::const_iterator& end) {
  
  
  
  
        
  
  while (start != end) {
    if (!IsTokenValueTrimmableCharacter(*start)) break;
    ++start;
  }

  
  
  
  while (end != start) {      
    --end;
    if (!IsTokenValueTrimmableCharacter(*end)) {
      ++end; 
      break;
    }
  }
}

PRBool CViewSourceHTML::IsTokenValueTrimmableCharacter(PRUnichar ch) {
  if (ch == ' ') return PR_TRUE;
  if (ch == '\t') return PR_TRUE;
  if (ch == '\r') return PR_TRUE;
  if (ch == '\n') return PR_TRUE;
  if (ch == '\'') return PR_TRUE;
  if (ch == '"') return PR_TRUE;
  return PR_FALSE;
}

nsresult CViewSourceHTML::GetBaseURI(nsIURI **result) {
  nsresult rv = NS_OK;
  if (!mBaseURI) {
    rv = SetBaseURI(mFilename);
  }
  NS_IF_ADDREF(*result = mBaseURI);
  return rv;
}

nsresult CViewSourceHTML::SetBaseURI(const nsAString& baseSpec) {
  
  nsCOMPtr<nsIURI> baseURI;
  nsresult rv = NS_NewURI(getter_AddRefs(baseURI), baseSpec, mCharset.get());
  NS_ENSURE_SUCCESS(rv, rv);
  mBaseURI = baseURI;
  return NS_OK;
}

void CViewSourceHTML::ExpandEntities(const nsAString& textIn, nsString& textOut)
{  
  nsAString::const_iterator iter, end;
  textIn.BeginReading(iter);
  textIn.EndReading(end);

  
  
  
  
  
  
  
  
  while (iter != end) {
    
    for (; iter != end; ++iter) {
      PRUnichar ch = *iter;
      if (ch == kAmpersand) {
        break;
      }
      textOut.Append(ch);
    }

    
    
    
    
    CopyPossibleEntity(iter, end, textOut);
  }
}

static PRBool InRange(PRUnichar ch, unsigned char chLow, unsigned char chHigh)
{
  return (chLow <= ch) && (ch <= chHigh);
}

static PRBool IsDigit(PRUnichar ch)
{ 
  return InRange(ch, '0', '9');
}

static PRBool IsHexDigit(PRUnichar ch)
{
  return IsDigit(ch) || InRange(ch, 'A', 'F') || InRange(ch, 'a', 'f');
}

static PRBool IsAlphaNum(PRUnichar ch)
{
  return InRange(ch, 'A', 'Z') || InRange(ch, 'a', 'z') || IsDigit(ch);
}

static PRBool IsAmpersand(PRUnichar ch)
{
  return ch == kAmpersand;
}

static PRBool IsHashsign(PRUnichar ch)
{
  return ch == kHashsign;
}

static PRBool IsXx(PRUnichar ch)
{
  return (ch == 'X') || (ch == 'x');
}

static PRBool IsSemicolon(PRUnichar ch)
{
  return ch == kSemicolon;
}

static PRBool ConsumeChar(nsAString::const_iterator& start,
                          const nsAString::const_iterator &end,
                          PRBool (*testFun)(PRUnichar ch))
{
  if (start == end) {
    return PR_FALSE;
  }
  if (!testFun(*start)) {
    return PR_FALSE;
  }
  ++start;
  return PR_TRUE;
}

void CViewSourceHTML::CopyPossibleEntity(nsAString::const_iterator& iter,
                                         const nsAString::const_iterator& end,
                                         nsAString& textBuffer)
{
  
  
  

  
  const nsAString::const_iterator start(iter);
  
  
  if (!ConsumeChar(iter, end, IsAmpersand)) {
    return;
  }

  
  nsAString::const_iterator startBody, endBody;
  enum {TYPE_ID, TYPE_DECIMAL, TYPE_HEXADECIMAL} entityType;
  if (ConsumeChar(iter, end, IsHashsign)) {
    if (ConsumeChar(iter, end, IsXx)) {
      startBody = iter;
      entityType = TYPE_HEXADECIMAL;
      while (ConsumeChar(iter, end, IsHexDigit)) {
        
      }
    } else {
      startBody = iter;
      entityType = TYPE_DECIMAL;
      while (ConsumeChar(iter, end, IsDigit)) {
        
      }
    }
  } else {
    startBody = iter;
    entityType = TYPE_ID;
    
    
    
    while (ConsumeChar(iter, end, IsAlphaNum)) {
      
    }
  }

  
  endBody = iter;
  
  
  PRBool properlyTerminated = ConsumeChar(iter, end, IsSemicolon);

  
  
  if (startBody == endBody) {
    textBuffer.Append(Substring(start, iter));
    return;
  }

  
  
  nsAutoString entityBody(Substring(startBody, endBody));

  
  PRInt32 entityCode = -1;
  switch (entityType) {
  case TYPE_ID:
    entityCode = nsHTMLEntities::EntityToUnicode(entityBody);
    break;
  case TYPE_DECIMAL:
    entityCode = ToUnicode(entityBody, 10, -1);
    break;
  case TYPE_HEXADECIMAL:
    entityCode = ToUnicode(entityBody, 16, -1);
    break;
  default:
    NS_NOTREACHED("Unknown entity type!");
    break;
  }

  
  
  if (properlyTerminated || ((0 <= entityCode) && (entityCode < 256))) {
    textBuffer.Append((PRUnichar) entityCode);
  } else {
    
    textBuffer.Append(Substring(start, iter));
  }
}

PRInt32 CViewSourceHTML::ToUnicode(const nsString &strNum, PRInt32 radix, PRInt32 fallback)
{
  PRInt32 result;
  PRInt32 code = strNum.ToInteger(&result, radix);
  if (result == NS_OK) {
    return code;
  }
  return fallback;
}


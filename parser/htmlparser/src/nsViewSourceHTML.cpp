














































#define NS_VIEWSOURCE_TOKENS_PER_BLOCK 16

#ifdef RAPTOR_PERF_METRICS
#  define START_TIMER()                    \
    if(mParser) mParser->mParseTime.Start(PR_FALSE); \
    if(mParser) mParser->mDTDTime.Start(PR_FALSE); 

#  define STOP_TIMER()                     \
    if(mParser) mParser->mParseTime.Stop(); \
    if(mParser) mParser->mDTDTime.Stop(); 

#else
#  define STOP_TIMER() 
#  define START_TIMER()
#endif

#include "nsIAtom.h"
#include "nsViewSourceHTML.h"
#include "nsCRT.h"
#include "nsParser.h"
#include "nsScanner.h"
#include "nsIParser.h"
#include "nsDTDUtils.h"
#include "nsIContentSink.h"
#include "nsIHTMLContentSink.h"
#include "nsHTMLTokenizer.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsUnicharUtils.h"
#include "nsPrintfCString.h"

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




class CIndirectTextToken : public CTextToken {
public:
  CIndirectTextToken() : CTextToken() {
    mIndirectString=0;
  }
  
  void SetIndirectString(const nsSubstring& aString) {
    mIndirectString=&aString;
  }

  virtual const nsSubstring& GetStringValue(void){
    return (const nsSubstring&)*mIndirectString;
  }

  const nsSubstring* mIndirectString;
};






class CSharedVSContext {
public:

  CSharedVSContext() {
  }
  
  ~CSharedVSContext() {
  }

  static CSharedVSContext& GetSharedContext() {
    static CSharedVSContext gSharedVSContext;
    return gSharedVSContext;
  }

  nsCParserNode       mEndNode;
  nsCParserStartNode  mStartNode;
  nsCParserStartNode  mTokenNode;
  CIndirectTextToken  mITextToken;
  nsCParserStartNode  mErrorNode;
  nsCParserNode       mEndErrorNode;
};

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

  mParser = 0;
  mSink = 0;
  mLineNumber = 1;
  mTokenizer = 0;
  mDocType=eHTML3_Quirks; 
  mHasOpenRoot=PR_FALSE;
  mHasOpenBody=PR_FALSE;

  mTokenCount=0;

#ifdef DUMP_TO_FILE
  gDumpFile = fopen(gDumpFileName,"w");
#endif 

}










CViewSourceHTML::~CViewSourceHTML(){
  mParser=0; 
}










nsresult CViewSourceHTML::WillBuildModel(const CParserContext& aParserContext,
                                         nsITokenizer* aTokenizer,
                                         nsIContentSink* aSink){

  nsresult result=NS_OK;

#ifdef RAPTOR_PERF_METRICS
  vsTimer.Reset();
  NS_START_STOPWATCH(vsTimer);
#endif 

  STOP_TIMER();
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
  
  
  
  
  
  
  
  
  
  CParserContext& parserContext = const_cast<CParserContext&>(aParserContext);
  parserContext.mDTDMode = eDTDMode_full_standards;
  result = mSink->WillBuildModel();
  
  parserContext.mDTDMode = mDTDMode;
  START_TIMER();
  return result;
}









NS_IMETHODIMP CViewSourceHTML::BuildModel(nsIParser* aParser,nsITokenizer* aTokenizer,nsITokenObserver* anObserver,nsIContentSink* aSink) {
  nsresult result=NS_OK;

  if(aTokenizer && aParser) {

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
        }
      }
    }

    mSink->WillProcessTokens();

    while(NS_SUCCEEDED(result)){
      CToken* theToken=mTokenizer->PopToken();
      if(theToken) {
        result=HandleToken(theToken,aParser);
        if(NS_SUCCEEDED(result)) {
          IF_FREE(theToken, mTokenizer->GetTokenAllocator());
          if (mParser->CanInterrupt() &&
              mSink->DidProcessAToken() == NS_ERROR_HTMLPARSER_INTERRUPTED) {
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







NS_IMETHODIMP CViewSourceHTML::DidBuildModel(nsresult anErrorCode,PRBool aNotifySink,nsIParser* aParser,nsIContentSink* aSink){
  nsresult result= NS_OK;

  

  if(aParser){

    mParser=(nsParser*)aParser;  
    STOP_TIMER();

    mSink=(nsIHTMLContentSink*)aParser->GetContentSink();
    if((aNotifySink) && (mSink)) {
        

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
      result = mSink->DidBuildModel();
    }

    START_TIMER();

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







NS_IMETHODIMP CViewSourceHTML::WillResumeParse(nsIContentSink* aSink){
  nsresult result = NS_OK;
  if(mSink) {
    result = mSink->WillResume();
  }
  return result;
}







NS_IMETHODIMP CViewSourceHTML::WillInterruptParse(nsIContentSink* aSink){
  nsresult result = NS_OK;
  if(mSink) {
    result = mSink->WillInterrupt();
  }
  return result;
}








void CViewSourceHTML::SetVerification(PRBool aEnabled)
{
}










PRBool CViewSourceHTML::CanContain(PRInt32 aParent,PRInt32 aChild) const{
  PRBool result=PR_TRUE;
  return result;
}









PRBool CViewSourceHTML::IsContainer(PRInt32 aTag) const{
  PRBool result=PR_TRUE;
  return result;
}








nsresult CViewSourceHTML::WriteAttributes(PRInt32 attrCount, PRBool aOwnerInError) {
  nsresult result=NS_OK;
  
  if(attrCount){ 

    CSharedVSContext& theContext=CSharedVSContext::GetSharedContext();

    int attr = 0;
    for(attr = 0; attr < attrCount; ++attr){
      CToken* theToken = mTokenizer->PeekToken();
      if(theToken)  {
        eHTMLTokenTypes theType = eHTMLTokenTypes(theToken->GetTokenType());
        if(eToken_attribute == theType){
          mTokenizer->PopToken(); 
          theContext.mTokenNode.AddAttribute(theToken);  

          CAttributeToken* theAttrToken = (CAttributeToken*)theToken;
          const nsSubstring& theKey = theAttrToken->GetKey();
          
          
          const PRBool attributeInError =
            !aOwnerInError && theAttrToken->IsInError();

          result = WriteTag(kAttributeName,theKey,0,attributeInError);
          const nsSubstring& theValue = theAttrToken->GetValue();

          if(!theValue.IsEmpty() || theAttrToken->mHasEqualWithoutValue){
            result = WriteTag(kAttributeValue,theValue,0,attributeInError);
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
  
  CSharedVSContext& theContext=CSharedVSContext::GetSharedContext();

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
    theContext.mErrorNode.Init(theTagToken, theAllocator);
    AddAttrToNode(theContext.mErrorNode, theAllocator,
                  NS_LITERAL_STRING("class"),
                  NS_LITERAL_STRING("error"));
    mSink->OpenContainer(theContext.mErrorNode);
#ifdef DUMP_TO_FILE
    if (gDumpFile) {
      fprintf(gDumpFile, "<span class=\"error\">");
    }
#endif
  }

  if (kBeforeText[aTagType][0] != 0) {
    NS_ConvertASCIItoUTF16 beforeText(kBeforeText[aTagType]);
    theContext.mITextToken.SetIndirectString(beforeText);
    nsCParserNode theNode(&theContext.mITextToken, 0);
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
    theContext.mStartNode.Init(theTagToken, theAllocator);
    AddAttrToNode(theContext.mStartNode, theAllocator,
                  NS_LITERAL_STRING("class"),
                  NS_ConvertASCIItoUTF16(kElementClasses[aTagType]));
    mSink->OpenContainer(theContext.mStartNode);  
#ifdef DUMP_TO_FILE
    if (gDumpFile) {
      fprintf(gDumpFile, "<span class=\"");
      fprintf(gDumpFile, kElementClasses[aTagType]);
      fprintf(gDumpFile, "\">");
    }
#endif 
  }

  STOP_TIMER();

  theContext.mITextToken.SetIndirectString(aText);  

  nsCParserNode theNode(&theContext.mITextToken, 0);
  mSink->AddLeaf(theNode);
#ifdef DUMP_TO_FILE
  if (gDumpFile) {
    fputs(NS_ConvertUTF16toUTF8(aText).get(), gDumpFile);
  }
#endif 

  if (mSyntaxHighlight && aTagType != kText) {
    theContext.mStartNode.ReleaseAll(); 
    CEndToken theEndToken(eHTMLTag_span);
    theContext.mEndNode.Init(&theEndToken, 0);
    mSink->CloseContainer(eHTMLTag_span);  
#ifdef DUMP_TO_FILE
    if (gDumpFile)
      fprintf(gDumpFile, "</span>");
#endif 
  }

  if(attrCount){
    result=WriteAttributes(attrCount, aTagInError);
  }

  
  
  if (!aTagInError && kAfterText[aTagType][0] != 0) {
    NS_ConvertASCIItoUTF16 afterText(kAfterText[aTagType]);
    theContext.mITextToken.SetIndirectString(afterText);
    nsCParserNode theNode(&theContext.mITextToken, 0);
    mSink->AddLeaf(theNode);
  }
#ifdef DUMP_TO_FILE
  if (!aTagInError && gDumpFile && kDumpFileAfterText[aTagType][0])
    fprintf(gDumpFile, kDumpFileAfterText[aTagType]);
#endif 

  if (mSyntaxHighlight && aTagInError) {
    theContext.mErrorNode.ReleaseAll(); 
    CEndToken theEndToken(eHTMLTag_span);
    theContext.mEndErrorNode.Init(&theEndToken, 0);
    mSink->CloseContainer(eHTMLTag_span);  
#ifdef DUMP_TO_FILE
    if (gDumpFile)
      fprintf(gDumpFile, "</span>");
#endif 
  }

  START_TIMER();

  return result;
}







NS_IMETHODIMP CViewSourceHTML::HandleToken(CToken* aToken,nsIParser* aParser)
{
  nsresult        result=NS_OK;
  CHTMLToken*     theToken= (CHTMLToken*)(aToken);
  eHTMLTokenTypes theType= (eHTMLTokenTypes)theToken->GetTokenType();
 
  mParser=(nsParser*)aParser;
  mSink=(nsIHTMLContentSink*)aParser->GetContentSink();
 
  CSharedVSContext& theContext=CSharedVSContext::GetSharedContext();
  theContext.mTokenNode.Init(theToken, mTokenizer->GetTokenAllocator());

  switch(theType) {
    
    case eToken_start:
      {
        const nsSubstring& startValue = aToken->GetStringValue();
        result = WriteTag(kStartTag,startValue,aToken->GetAttributeCount(),aToken->IsInError());

        if((ePlainText!=mDocType) && mParser && (NS_OK==result)) {
          result = mSink->NotifyTagObservers(&theContext.mTokenNode);
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

  theContext.mTokenNode.ReleaseAll(); 

  return result;
}


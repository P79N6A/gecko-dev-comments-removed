




































#ifndef NS_IPARSER___
#define NS_IPARSER___










#include "nsISupports.h"
#include "nsIStreamListener.h"
#include "nsIDTD.h"
#include "nsStringGlue.h"
#include "nsTArray.h"
#include "nsIAtom.h"

#define NS_IPARSER_IID \
{ 0xcbc0cbd8, 0xbbb7, 0x46d6, \
  { 0xa5, 0x51, 0x37, 0x8a, 0x69, 0x53, 0xa7, 0x14 } }


#define NS_IDEBUG_DUMP_CONTENT_IID \
{ 0x41421c60, 0x310a, 0x11d4, \
{ 0x81, 0x6f, 0x0, 0x0, 0x64, 0x65, 0x73, 0x74 } }

class nsIContentSink;
class nsIRequestObserver;
class nsIParserFilter;
class nsString;
class nsIURI;
class nsIChannel;
class nsIContent;

enum eParserCommands {
  eViewNormal,
  eViewSource,
  eViewFragment,
  eViewErrors
};

enum eParserDocType {
  ePlainText = 0,
  eXML,
  eHTML_Quirks,
  eHTML_Strict
};




#define kCharsetUninitialized           0
#define kCharsetFromWeakDocTypeDefault  1
#define kCharsetFromUserDefault         2
#define kCharsetFromDocTypeDefault      3
#define kCharsetFromCache               4
#define kCharsetFromParentFrame         5
#define kCharsetFromAutoDetection       6
#define kCharsetFromHintPrevDoc         7
#define kCharsetFromMetaPrescan         8 // this one and smaller: HTML5 Tentative
#define kCharsetFromMetaTag             9 // this one and greater: HTML5 Confident
#define kCharsetFromByteOrderMark      10
#define kCharsetFromChannel            11
#define kCharsetFromOtherComponent     12

#define kCharsetFromParentForced       13
#define kCharsetFromUserForced         14
#define kCharsetFromPreviousLoading    15

enum eStreamState {eNone,eOnStart,eOnDataAvail,eOnStop};










class nsIDebugDumpContent : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDEBUG_DUMP_CONTENT_IID)
  NS_IMETHOD DumpContentModel()=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDebugDumpContent, NS_IDEBUG_DUMP_CONTENT_IID)





class nsIParser : public nsISupports {
  public:

    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPARSER_IID)

    





    NS_IMETHOD_(void) SetContentSink(nsIContentSink* aSink)=0;


    




    NS_IMETHOD_(nsIContentSink*) GetContentSink(void)=0;

    








    NS_IMETHOD_(void) GetCommand(nsCString& aCommand)=0;
    NS_IMETHOD_(void) SetCommand(const char* aCommand)=0;
    NS_IMETHOD_(void) SetCommand(eParserCommands aParserCommand)=0;

    








    NS_IMETHOD_(void) SetDocumentCharset(const nsACString& aCharset, PRInt32 aSource)=0;
    NS_IMETHOD_(void) GetDocumentCharset(nsACString& oCharset, PRInt32& oSource)=0;

    NS_IMETHOD_(void) SetParserFilter(nsIParserFilter* aFilter) = 0;

    





    NS_IMETHOD GetChannel(nsIChannel** aChannel) = 0;

    





    NS_IMETHOD GetDTD(nsIDTD** aDTD) = 0;
    
    




    NS_IMETHOD GetStreamListener(nsIStreamListener** aListener) = 0;

    




    
    
    
    
    
    
    
    NS_IMETHOD ContinueInterruptedParsing() = 0;
    
    
    NS_IMETHOD_(void) BlockParser() = 0;
    
    
    
    
    
    NS_IMETHOD_(void) UnblockParser() = 0;

    NS_IMETHOD_(PRBool) IsParserEnabled() = 0;
    NS_IMETHOD_(PRBool) IsComplete() = 0;
    
    NS_IMETHOD Parse(nsIURI* aURL,
                     nsIRequestObserver* aListener = nsnull,
                     void* aKey = 0,
                     nsDTDMode aMode = eDTDMode_autodetect) = 0;
    NS_IMETHOD Parse(const nsAString& aSourceBuffer,
                     void* aKey,
                     const nsACString& aMimeType,
                     PRBool aLastCall,
                     nsDTDMode aMode = eDTDMode_autodetect) = 0;

    
    
    NS_IMETHOD_(void *) GetRootContextKey() = 0;
    
    NS_IMETHOD Terminate(void) = 0;

    












    NS_IMETHOD ParseFragment(const nsAString& aSourceBuffer,
                             void* aKey,
                             nsTArray<nsString>& aTagStack,
                             PRBool aXMLMode,
                             const nsACString& aContentType,
                             nsDTDMode aMode = eDTDMode_autodetect) = 0;

    NS_IMETHOD ParseFragment(const nsAString& aSourceBuffer,
                             nsIContent* aTargetNode,
                             nsIAtom* aContextLocalName,
                             PRInt32 aContextNamespace,
                             PRBool aQuirks) = 0;

    





    NS_IMETHOD BuildModel(void) = 0;

    









    NS_IMETHOD CancelParsingEvents() = 0;

    virtual void Reset() = 0;

    



    virtual PRBool CanInterrupt() = 0;

    


    virtual PRBool IsInsertionPointDefined() = 0;

    


    virtual void BeginEvaluatingParserInsertedScript() = 0;

    


    virtual void EndEvaluatingParserInsertedScript() = 0;

    


    virtual void MarkAsNotScriptCreated() = 0;

    


    virtual PRBool IsScriptCreated() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIParser, NS_IPARSER_IID)





#include "prtypes.h"
#include "nsError.h"

#define NS_ERROR_HTMLPARSER_EOF                            NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_HTMLPARSER,1000)
#define NS_ERROR_HTMLPARSER_UNKNOWN                        NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_HTMLPARSER,1001)
#define NS_ERROR_HTMLPARSER_CANTPROPAGATE                  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_HTMLPARSER,1002)
#define NS_ERROR_HTMLPARSER_CONTEXTMISMATCH                NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_HTMLPARSER,1003)
#define NS_ERROR_HTMLPARSER_BADFILENAME                    NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_HTMLPARSER,1004)
#define NS_ERROR_HTMLPARSER_BADURL                         NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_HTMLPARSER,1005)
#define NS_ERROR_HTMLPARSER_INVALIDPARSERCONTEXT           NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_HTMLPARSER,1006)
#define NS_ERROR_HTMLPARSER_INTERRUPTED                    NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_HTMLPARSER,1007)
#define NS_ERROR_HTMLPARSER_BLOCK                          NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_HTMLPARSER,1008)
#define NS_ERROR_HTMLPARSER_BADTOKENIZER                   NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_HTMLPARSER,1009)
#define NS_ERROR_HTMLPARSER_BADATTRIBUTE                   NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_HTMLPARSER,1010)
#define NS_ERROR_HTMLPARSER_UNRESOLVEDDTD                  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_HTMLPARSER,1011)
#define NS_ERROR_HTMLPARSER_MISPLACEDTABLECONTENT          NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_HTMLPARSER,1012)
#define NS_ERROR_HTMLPARSER_BADDTD                         NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_HTMLPARSER,1013)
#define NS_ERROR_HTMLPARSER_BADCONTEXT                     NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_HTMLPARSER,1014)
#define NS_ERROR_HTMLPARSER_STOPPARSING                    NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_HTMLPARSER,1015)
#define NS_ERROR_HTMLPARSER_UNTERMINATEDSTRINGLITERAL      NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_HTMLPARSER,1016)
#define NS_ERROR_HTMLPARSER_HIERARCHYTOODEEP               NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_HTMLPARSER,1017)
#define NS_ERROR_HTMLPARSER_FAKE_ENDTAG                    NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_HTMLPARSER,1018)
#define NS_ERROR_HTMLPARSER_INVALID_COMMENT                NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_HTMLPARSER,1019)

#define NS_ERROR_HTMLPARSER_CONTINUE              NS_OK


const PRUint32  kEOF              = NS_ERROR_HTMLPARSER_EOF;
const PRUint32  kUnknownError     = NS_ERROR_HTMLPARSER_UNKNOWN;
const PRUint32  kCantPropagate    = NS_ERROR_HTMLPARSER_CANTPROPAGATE;
const PRUint32  kContextMismatch  = NS_ERROR_HTMLPARSER_CONTEXTMISMATCH;
const PRUint32  kBadFilename      = NS_ERROR_HTMLPARSER_BADFILENAME;
const PRUint32  kBadURL           = NS_ERROR_HTMLPARSER_BADURL;
const PRUint32  kInvalidParserContext = NS_ERROR_HTMLPARSER_INVALIDPARSERCONTEXT;
const PRUint32  kBlocked          = NS_ERROR_HTMLPARSER_BLOCK;
const PRUint32  kBadStringLiteral = NS_ERROR_HTMLPARSER_UNTERMINATEDSTRINGLITERAL;
const PRUint32  kHierarchyTooDeep = NS_ERROR_HTMLPARSER_HIERARCHYTOODEEP;
const PRUint32  kFakeEndTag       = NS_ERROR_HTMLPARSER_FAKE_ENDTAG;
const PRUint32  kNotAComment      = NS_ERROR_HTMLPARSER_INVALID_COMMENT;

const PRUnichar  kNewLine          = '\n';
const PRUnichar  kCR               = '\r';
const PRUnichar  kLF               = '\n';
const PRUnichar  kTab              = '\t';
const PRUnichar  kSpace            = ' ';
const PRUnichar  kQuote            = '"';
const PRUnichar  kApostrophe       = '\'';
const PRUnichar  kLessThan         = '<';
const PRUnichar  kGreaterThan      = '>';
const PRUnichar  kAmpersand        = '&';
const PRUnichar  kForwardSlash     = '/';
const PRUnichar  kBackSlash        = '\\';
const PRUnichar  kEqual            = '=';
const PRUnichar  kMinus            = '-';
const PRUnichar  kPlus             = '+';
const PRUnichar  kExclamation      = '!';
const PRUnichar  kSemicolon        = ';';
const PRUnichar  kHashsign         = '#';
const PRUnichar  kAsterisk         = '*';
const PRUnichar  kUnderbar         = '_';
const PRUnichar  kComma            = ',';
const PRUnichar  kLeftParen        = '(';
const PRUnichar  kRightParen       = ')';
const PRUnichar  kLeftBrace        = '{';
const PRUnichar  kRightBrace       = '}';
const PRUnichar  kQuestionMark     = '?';
const PRUnichar  kLeftSquareBracket  = '[';
const PRUnichar  kRightSquareBracket = ']';
const PRUnichar kNullCh           = '\0';

#define NS_IPARSER_FLAG_UNKNOWN_MODE         0x00000000
#define NS_IPARSER_FLAG_QUIRKS_MODE          0x00000002
#define NS_IPARSER_FLAG_STRICT_MODE          0x00000004
#define NS_IPARSER_FLAG_AUTO_DETECT_MODE     0x00000010
#define NS_IPARSER_FLAG_VIEW_NORMAL          0x00000020
#define NS_IPARSER_FLAG_VIEW_SOURCE          0x00000040
#define NS_IPARSER_FLAG_VIEW_ERRORS          0x00000080
#define NS_IPARSER_FLAG_PLAIN_TEXT           0x00000100
#define NS_IPARSER_FLAG_XML                  0x00000200
#define NS_IPARSER_FLAG_HTML                 0x00000400
#define NS_IPARSER_FLAG_SCRIPT_ENABLED       0x00000800
#define NS_IPARSER_FLAG_FRAMES_ENABLED       0x00001000

#endif 

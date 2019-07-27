




#ifndef NS_IPARSER___
#define NS_IPARSER___













#include "nsISupports.h"
#include "nsIStreamListener.h"
#include "nsIDTD.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsIAtom.h"
#include "nsParserBase.h"

#define NS_IPARSER_IID \
{ 0x2c4ad90a, 0x740e, 0x4212, \
  { 0xba, 0x3f, 0xfe, 0xac, 0xda, 0x4b, 0x92, 0x9e } }


#define NS_IDEBUG_DUMP_CONTENT_IID \
{ 0x41421c60, 0x310a, 0x11d4, \
{ 0x81, 0x6f, 0x0, 0x0, 0x64, 0x65, 0x73, 0x74 } }

class nsIContentSink;
class nsIRequestObserver;
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

enum eStreamState {eNone,eOnStart,eOnDataAvail,eOnStop};











class nsIParser : public nsParserBase {
  public:

    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPARSER_IID)

    





    NS_IMETHOD_(void) SetContentSink(nsIContentSink* aSink)=0;


    




    NS_IMETHOD_(nsIContentSink*) GetContentSink(void)=0;

    








    NS_IMETHOD_(void) GetCommand(nsCString& aCommand)=0;
    NS_IMETHOD_(void) SetCommand(const char* aCommand)=0;
    NS_IMETHOD_(void) SetCommand(eParserCommands aParserCommand)=0;

    








    NS_IMETHOD_(void) SetDocumentCharset(const nsACString& aCharset, int32_t aSource)=0;
    NS_IMETHOD_(void) GetDocumentCharset(nsACString& oCharset, int32_t& oSource)=0;

    





    NS_IMETHOD GetChannel(nsIChannel** aChannel) = 0;

    





    NS_IMETHOD GetDTD(nsIDTD** aDTD) = 0;
    
    


    virtual nsIStreamListener* GetStreamListener() = 0;

    




    
    
    
    
    
    
    
    NS_IMETHOD ContinueInterruptedParsing() = 0;
    
    
    NS_IMETHOD_(void) BlockParser() = 0;
    
    
    
    
    
    NS_IMETHOD_(void) UnblockParser() = 0;

    


    NS_IMETHOD_(void) ContinueInterruptedParsingAsync() = 0;

    NS_IMETHOD_(bool) IsParserEnabled() = 0;
    NS_IMETHOD_(bool) IsComplete() = 0;
    
    NS_IMETHOD Parse(nsIURI* aURL,
                     nsIRequestObserver* aListener = nullptr,
                     void* aKey = 0,
                     nsDTDMode aMode = eDTDMode_autodetect) = 0;

    NS_IMETHOD Terminate(void) = 0;

    








    NS_IMETHOD ParseFragment(const nsAString& aSourceBuffer,
                             nsTArray<nsString>& aTagStack) = 0;

    





    NS_IMETHOD BuildModel(void) = 0;

    









    NS_IMETHOD CancelParsingEvents() = 0;

    virtual void Reset() = 0;

    


    virtual bool IsInsertionPointDefined() = 0;

    


    virtual void BeginEvaluatingParserInsertedScript() = 0;

    


    virtual void EndEvaluatingParserInsertedScript() = 0;

    


    virtual void MarkAsNotScriptCreated(const char* aCommand) = 0;

    


    virtual bool IsScriptCreated() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIParser, NS_IPARSER_IID)





#include "nsError.h"

const nsresult  kEOF              = NS_ERROR_HTMLPARSER_EOF;
const nsresult  kUnknownError     = NS_ERROR_HTMLPARSER_UNKNOWN;
const nsresult  kCantPropagate    = NS_ERROR_HTMLPARSER_CANTPROPAGATE;
const nsresult  kContextMismatch  = NS_ERROR_HTMLPARSER_CONTEXTMISMATCH;
const nsresult  kBadFilename      = NS_ERROR_HTMLPARSER_BADFILENAME;
const nsresult  kBadURL           = NS_ERROR_HTMLPARSER_BADURL;
const nsresult  kInvalidParserContext = NS_ERROR_HTMLPARSER_INVALIDPARSERCONTEXT;
const nsresult  kBlocked          = NS_ERROR_HTMLPARSER_BLOCK;
const nsresult  kBadStringLiteral = NS_ERROR_HTMLPARSER_UNTERMINATEDSTRINGLITERAL;
const nsresult  kHierarchyTooDeep = NS_ERROR_HTMLPARSER_HIERARCHYTOODEEP;
const nsresult  kFakeEndTag       = NS_ERROR_HTMLPARSER_FAKE_ENDTAG;
const nsresult  kNotAComment      = NS_ERROR_HTMLPARSER_INVALID_COMMENT;

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

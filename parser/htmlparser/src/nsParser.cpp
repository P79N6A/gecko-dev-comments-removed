






































#include "nsIAtom.h"
#include "nsParser.h"
#include "nsString.h"
#include "nsCRT.h"
#include "nsScanner.h"
#include "plstr.h"
#include "nsIStringStream.h"
#include "nsIChannel.h"
#include "nsICachingChannel.h"
#include "nsICacheEntryDescriptor.h"
#include "nsICharsetAlias.h"
#include "nsICharsetConverterManager.h"
#include "nsIInputStream.h"
#include "CNavDTD.h"
#include "prenv.h"
#include "prlock.h"
#include "prcvar.h"
#include "nsParserCIID.h"
#include "nsReadableUtils.h"
#include "nsCOMPtr.h"
#include "nsExpatDriver.h"
#include "nsIServiceManager.h"
#include "nsICategoryManager.h"
#include "nsISupportsPrimitives.h"
#include "nsIFragmentContentSink.h"
#include "nsStreamUtils.h"
#include "nsHTMLTokenizer.h"
#include "nsIDocument.h"
#include "nsNetUtil.h"
#include "nsScriptLoader.h"
#include "nsDataHashtable.h"
#include "nsIThreadPool.h"
#include "nsXPCOMCIDInternal.h"
#include "nsMimeTypes.h"
#include "mozilla/CondVar.h"
#include "mozilla/Mutex.h"
#include "nsParserConstants.h"

using namespace mozilla;

#define NS_PARSER_FLAG_PARSER_ENABLED         0x00000002
#define NS_PARSER_FLAG_OBSERVERS_ENABLED      0x00000004
#define NS_PARSER_FLAG_PENDING_CONTINUE_EVENT 0x00000008
#define NS_PARSER_FLAG_CAN_INTERRUPT          0x00000010
#define NS_PARSER_FLAG_FLUSH_TOKENS           0x00000020
#define NS_PARSER_FLAG_CAN_TOKENIZE           0x00000040

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_CID(kCParserCID, NS_PARSER_CID);
static NS_DEFINE_IID(kIParserIID, NS_IPARSER_IID);

























































class nsParserContinueEvent : public nsRunnable
{
public:
  nsRefPtr<nsParser> mParser;

  nsParserContinueEvent(nsParser* aParser)
    : mParser(aParser)
  {}

  NS_IMETHOD Run()
  {
    mParser->HandleParserContinueEvent(this);
    return NS_OK;
  }
};



nsICharsetAlias* nsParser::sCharsetAliasService = nsnull;
nsICharsetConverterManager* nsParser::sCharsetConverterManager = nsnull;





nsresult
nsParser::Init()
{
  nsresult rv;

  nsCOMPtr<nsICharsetAlias> charsetAlias =
    do_GetService(NS_CHARSETALIAS_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsICharsetConverterManager> charsetConverter =
    do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  charsetAlias.swap(sCharsetAliasService);
  charsetConverter.swap(sCharsetConverterManager);

  return NS_OK;
}






void nsParser::Shutdown()
{
  NS_IF_RELEASE(sCharsetAliasService);
  NS_IF_RELEASE(sCharsetConverterManager);
}

#ifdef DEBUG
static bool gDumpContent=false;
#endif




nsParser::nsParser()
{
  Initialize(true);
}

nsParser::~nsParser()
{
  Cleanup();
}

void
nsParser::Initialize(bool aConstructor)
{
#ifdef NS_DEBUG
  if (!gDumpContent) {
    gDumpContent = PR_GetEnv("PARSER_DUMP_CONTENT") != nsnull;
  }
#endif

  if (aConstructor) {
    
    mParserContext = 0;
  }
  else {
    
    mObserver = nsnull;
    mUnusedInput.Truncate();
  }

  mContinueEvent = nsnull;
  mCharsetSource = kCharsetUninitialized;
  mCharset.AssignLiteral("ISO-8859-1");
  mInternalState = NS_OK;
  mStreamStatus = 0;
  mCommand = eViewNormal;
  mFlags = NS_PARSER_FLAG_OBSERVERS_ENABLED |
           NS_PARSER_FLAG_PARSER_ENABLED |
           NS_PARSER_FLAG_CAN_TOKENIZE;

  mProcessingNetworkData = false;
}

void
nsParser::Cleanup()
{
#ifdef NS_DEBUG
  if (gDumpContent) {
    if (mSink) {
      
      
      nsresult result = NS_OK;
      nsCOMPtr<nsIDebugDumpContent> trigger = do_QueryInterface(mSink, &result);
      if (NS_SUCCEEDED(result)) {
        trigger->DumpContentModel();
      }
    }
  }
#endif

#ifdef DEBUG
  if (mParserContext && mParserContext->mPrevContext) {
    NS_WARNING("Extra parser contexts still on the parser stack");
  }
#endif

  while (mParserContext) {
    CParserContext *pc = mParserContext->mPrevContext;
    delete mParserContext;
    mParserContext = pc;
  }

  
  
  
  NS_ASSERTION(!(mFlags & NS_PARSER_FLAG_PENDING_CONTINUE_EVENT), "bad");
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsParser)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsParser)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mDTD)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mSink)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mObserver)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsParser)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDTD)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mSink)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mObserver)
  CParserContext *pc = tmp->mParserContext;
  while (pc) {
    cb.NoteXPCOMChild(pc->mTokenizer);
    pc = pc->mPrevContext;
  }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsParser)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsParser)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsParser)
  NS_INTERFACE_MAP_ENTRY(nsIStreamListener)
  NS_INTERFACE_MAP_ENTRY(nsIParser)
  NS_INTERFACE_MAP_ENTRY(nsIRequestObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIParser)
NS_INTERFACE_MAP_END






nsresult
nsParser::PostContinueEvent()
{
  if (!(mFlags & NS_PARSER_FLAG_PENDING_CONTINUE_EVENT)) {
    
    NS_ASSERTION(!mContinueEvent, "bad");

    
    
    nsCOMPtr<nsIRunnable> event = new nsParserContinueEvent(this);
    if (NS_FAILED(NS_DispatchToCurrentThread(event))) {
        NS_WARNING("failed to dispatch parser continuation event");
    } else {
        mFlags |= NS_PARSER_FLAG_PENDING_CONTINUE_EVENT;
        mContinueEvent = event;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP_(void)
nsParser::GetCommand(nsCString& aCommand)
{
  aCommand = mCommandStr;
}








NS_IMETHODIMP_(void)
nsParser::SetCommand(const char* aCommand)
{
  mCommandStr.Assign(aCommand);
  if (mCommandStr.Equals("view-source")) {
    mCommand = eViewSource;
  } else if (mCommandStr.Equals("view-fragment")) {
    mCommand = eViewFragment;
  } else {
    mCommand = eViewNormal;
  }
}








NS_IMETHODIMP_(void)
nsParser::SetCommand(eParserCommands aParserCommand)
{
  mCommand = aParserCommand;
}








NS_IMETHODIMP_(void)
nsParser::SetDocumentCharset(const nsACString& aCharset, PRInt32 aCharsetSource)
{
  mCharset = aCharset;
  mCharsetSource = aCharsetSource;
  if (mParserContext && mParserContext->mScanner) {
     mParserContext->mScanner->SetDocumentCharset(aCharset, aCharsetSource);
  }
}

void
nsParser::SetSinkCharset(nsACString& aCharset)
{
  if (mSink) {
    mSink->SetDocumentCharset(aCharset);
  }
}







NS_IMETHODIMP_(void)
nsParser::SetContentSink(nsIContentSink* aSink)
{
  NS_PRECONDITION(aSink, "sink cannot be null!");
  mSink = aSink;

  if (mSink) {
    mSink->SetParser(this);
  }
}





NS_IMETHODIMP_(nsIContentSink*)
nsParser::GetContentSink()
{
  return mSink;
}














static PRInt32
ParsePS(const nsString& aBuffer, PRInt32 aIndex)
{
  for (;;) {
    PRUnichar ch = aBuffer.CharAt(aIndex);
    if ((ch == PRUnichar(' ')) || (ch == PRUnichar('\t')) ||
        (ch == PRUnichar('\n')) || (ch == PRUnichar('\r'))) {
      ++aIndex;
    } else if (ch == PRUnichar('-')) {
      PRInt32 tmpIndex;
      if (aBuffer.CharAt(aIndex+1) == PRUnichar('-') &&
          kNotFound != (tmpIndex=aBuffer.Find("--",false,aIndex+2,-1))) {
        aIndex = tmpIndex + 2;
      } else {
        return aIndex;
      }
    } else {
      return aIndex;
    }
  }
}

#define PARSE_DTD_HAVE_DOCTYPE          (1<<0)
#define PARSE_DTD_HAVE_PUBLIC_ID        (1<<1)
#define PARSE_DTD_HAVE_SYSTEM_ID        (1<<2)
#define PARSE_DTD_HAVE_INTERNAL_SUBSET  (1<<3)


static bool
ParseDocTypeDecl(const nsString &aBuffer,
                 PRInt32 *aResultFlags,
                 nsString &aPublicID,
                 nsString &aSystemID)
{
  bool haveDoctype = false;
  *aResultFlags = 0;

  
  
  PRInt32 theIndex = 0;
  do {
    theIndex = aBuffer.FindChar('<', theIndex);
    if (theIndex == kNotFound) break;
    PRUnichar nextChar = aBuffer.CharAt(theIndex+1);
    if (nextChar == PRUnichar('!')) {
      PRInt32 tmpIndex = theIndex + 2;
      if (kNotFound !=
          (theIndex=aBuffer.Find("DOCTYPE", true, tmpIndex, 0))) {
        haveDoctype = true;
        theIndex += 7; 
        break;
      }
      theIndex = ParsePS(aBuffer, tmpIndex);
      theIndex = aBuffer.FindChar('>', theIndex);
    } else if (nextChar == PRUnichar('?')) {
      theIndex = aBuffer.FindChar('>', theIndex);
    } else {
      break;
    }
  } while (theIndex != kNotFound);

  if (!haveDoctype)
    return true;
  *aResultFlags |= PARSE_DTD_HAVE_DOCTYPE;

  theIndex = ParsePS(aBuffer, theIndex);
  theIndex = aBuffer.Find("HTML", true, theIndex, 0);
  if (kNotFound == theIndex)
    return false;
  theIndex = ParsePS(aBuffer, theIndex+4);
  PRInt32 tmpIndex = aBuffer.Find("PUBLIC", true, theIndex, 0);

  if (kNotFound != tmpIndex) {
    theIndex = ParsePS(aBuffer, tmpIndex+6);

    
    

    
    

    PRUnichar lit = aBuffer.CharAt(theIndex);
    if ((lit != PRUnichar('\"')) && (lit != PRUnichar('\'')))
      return false;

    
    

    PRInt32 PublicIDStart = theIndex + 1;
    PRInt32 PublicIDEnd = aBuffer.FindChar(lit, PublicIDStart);
    if (kNotFound == PublicIDEnd)
      return false;
    theIndex = ParsePS(aBuffer, PublicIDEnd + 1);
    PRUnichar next = aBuffer.CharAt(theIndex);
    if (next == PRUnichar('>')) {
      
      
      
      
      
    } else if ((next == PRUnichar('\"')) ||
               (next == PRUnichar('\''))) {
      
      *aResultFlags |= PARSE_DTD_HAVE_SYSTEM_ID;
      PRInt32 SystemIDStart = theIndex + 1;
      PRInt32 SystemIDEnd = aBuffer.FindChar(next, SystemIDStart);
      if (kNotFound == SystemIDEnd)
        return false;
      aSystemID =
        Substring(aBuffer, SystemIDStart, SystemIDEnd - SystemIDStart);
    } else if (next == PRUnichar('[')) {
      
      *aResultFlags |= PARSE_DTD_HAVE_INTERNAL_SUBSET;
    } else {
      
      return false;
    }

    
    
    aPublicID = Substring(aBuffer, PublicIDStart, PublicIDEnd - PublicIDStart);
    aPublicID.CompressWhitespace(true, true);
    *aResultFlags |= PARSE_DTD_HAVE_PUBLIC_ID;
  } else {
    tmpIndex=aBuffer.Find("SYSTEM", true, theIndex, 0);
    if (kNotFound != tmpIndex) {
      
      *aResultFlags |= PARSE_DTD_HAVE_SYSTEM_ID;

      theIndex = ParsePS(aBuffer, tmpIndex+6);
      PRUnichar next = aBuffer.CharAt(theIndex);
      if (next != PRUnichar('\"') && next != PRUnichar('\''))
        return false;

      PRInt32 SystemIDStart = theIndex + 1;
      PRInt32 SystemIDEnd = aBuffer.FindChar(next, SystemIDStart);

      if (kNotFound == SystemIDEnd)
        return false;
      aSystemID =
        Substring(aBuffer, SystemIDStart, SystemIDEnd - SystemIDStart);
      theIndex = ParsePS(aBuffer, SystemIDEnd + 1);
    }

    PRUnichar nextChar = aBuffer.CharAt(theIndex);
    if (nextChar == PRUnichar('['))
      *aResultFlags |= PARSE_DTD_HAVE_INTERNAL_SUBSET;
    else if (nextChar != PRUnichar('>'))
      return false;
  }
  return true;
}

struct PubIDInfo
{
  enum eMode {
    eQuirks,         
    eAlmostStandards,
    eFullStandards   
      




  };

  const char* name;
  eMode mode_if_no_sysid;
  eMode mode_if_sysid;
};

#define ELEMENTS_OF(array_) (sizeof(array_)/sizeof(array_[0]))










static const PubIDInfo kPublicIDs[] = {
  {"+//silmaril//dtd html pro v0r11 19970101//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//advasoft ltd//dtd html 3.0 aswedit + extensions//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//as//dtd html 3.0 aswedit + extensions//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html 2.0 level 1//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html 2.0 level 2//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html 2.0 strict level 1//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html 2.0 strict level 2//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html 2.0 strict//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html 2.0//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html 2.1e//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html 3.0//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html 3.0//en//" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html 3.2 final//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html 3.2//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html 3//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html level 0//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html level 0//en//2.0" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html level 1//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html level 1//en//2.0" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html level 2//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html level 2//en//2.0" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html level 3//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html level 3//en//3.0" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html strict level 0//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html strict level 0//en//2.0" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html strict level 1//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html strict level 1//en//2.0" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html strict level 2//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html strict level 2//en//2.0" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html strict level 3//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html strict level 3//en//3.0" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html strict//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html strict//en//2.0" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html strict//en//3.0" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html//en//2.0" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html//en//3.0" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//metrius//dtd metrius presentational//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//microsoft//dtd internet explorer 2.0 html strict//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//microsoft//dtd internet explorer 2.0 html//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//microsoft//dtd internet explorer 2.0 tables//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//microsoft//dtd internet explorer 3.0 html strict//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//microsoft//dtd internet explorer 3.0 html//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//microsoft//dtd internet explorer 3.0 tables//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//netscape comm. corp.//dtd html//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//netscape comm. corp.//dtd strict html//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//o'reilly and associates//dtd html 2.0//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//o'reilly and associates//dtd html extended 1.0//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//o'reilly and associates//dtd html extended relaxed 1.0//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//softquad software//dtd hotmetal pro 6.0::19990601::extensions to html 4.0//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//softquad//dtd hotmetal pro 4.0::19971010::extensions to html 4.0//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//spyglass//dtd html 2.0 extended//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//sq//dtd html 2.0 hotmetal + extensions//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//sun microsystems corp.//dtd hotjava html//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//sun microsystems corp.//dtd hotjava strict html//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//w3c//dtd html 3 1995-03-24//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//w3c//dtd html 3.2 draft//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//w3c//dtd html 3.2 final//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//w3c//dtd html 3.2//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//w3c//dtd html 3.2s draft//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//w3c//dtd html 4.0 frameset//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//w3c//dtd html 4.0 transitional//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//w3c//dtd html 4.01 frameset//en" , PubIDInfo::eQuirks, PubIDInfo::eAlmostStandards},
  {"-//w3c//dtd html 4.01 transitional//en" , PubIDInfo::eQuirks, PubIDInfo::eAlmostStandards},
  {"-//w3c//dtd html experimental 19960712//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//w3c//dtd html experimental 970421//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//w3c//dtd w3 html//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//w3c//dtd xhtml 1.0 frameset//en" , PubIDInfo::eAlmostStandards, PubIDInfo::eAlmostStandards},
  {"-//w3c//dtd xhtml 1.0 transitional//en" , PubIDInfo::eAlmostStandards, PubIDInfo::eAlmostStandards},
  {"-//w3o//dtd w3 html 3.0//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//w3o//dtd w3 html 3.0//en//" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//w3o//dtd w3 html strict 3.0//en//" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//webtechs//dtd mozilla html 2.0//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//webtechs//dtd mozilla html//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-/w3c/dtd html 4.0 transitional/en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"html" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
};

#ifdef DEBUG
static void
VerifyPublicIDs()
{
  static bool gVerified = false;
  if (!gVerified) {
    gVerified = true;
    PRUint32 i;
    for (i = 0; i < ELEMENTS_OF(kPublicIDs) - 1; ++i) {
      if (nsCRT::strcmp(kPublicIDs[i].name, kPublicIDs[i+1].name) >= 0) {
        NS_NOTREACHED("doctypes out of order");
        printf("Doctypes %s and %s out of order.\n",
               kPublicIDs[i].name, kPublicIDs[i+1].name);
      }
    }
    for (i = 0; i < ELEMENTS_OF(kPublicIDs); ++i) {
      nsCAutoString lcPubID(kPublicIDs[i].name);
      ToLowerCase(lcPubID);
      if (nsCRT::strcmp(kPublicIDs[i].name, lcPubID.get()) != 0) {
        NS_NOTREACHED("doctype not lower case");
        printf("Doctype %s not lower case.\n", kPublicIDs[i].name);
      }
    }
  }
}
#endif

static void
DetermineHTMLParseMode(const nsString& aBuffer,
                       nsDTDMode& aParseMode,
                       eParserDocType& aDocType)
{
#ifdef DEBUG
  VerifyPublicIDs();
#endif
  PRInt32 resultFlags;
  nsAutoString publicIDUCS2, sysIDUCS2;
  if (ParseDocTypeDecl(aBuffer, &resultFlags, publicIDUCS2, sysIDUCS2)) {
    if (!(resultFlags & PARSE_DTD_HAVE_DOCTYPE)) {
      
      aParseMode = eDTDMode_quirks;
      aDocType = eHTML_Quirks;
    } else if ((resultFlags & PARSE_DTD_HAVE_INTERNAL_SUBSET) ||
               !(resultFlags & PARSE_DTD_HAVE_PUBLIC_ID)) {
      
      
      aDocType = eHTML_Strict;
      aParseMode = eDTDMode_full_standards;

      
      if (!(resultFlags & PARSE_DTD_HAVE_INTERNAL_SUBSET) &&
          sysIDUCS2 == NS_LITERAL_STRING(
               "http://www.ibm.com/data/dtd/v11/ibmxhtml1-transitional.dtd")) {
        aParseMode = eDTDMode_quirks;
        aDocType = eHTML_Quirks;
      }

    } else {
      
      
      nsCAutoString publicID;
      publicID.AssignWithConversion(publicIDUCS2);

      
      
      ToLowerCase(publicID);

      
      
      
      PRInt32 minimum = 0;
      PRInt32 maximum = ELEMENTS_OF(kPublicIDs) - 1;
      PRInt32 index;
      for (;;) {
        index = (minimum + maximum) / 2;
        PRInt32 comparison =
            nsCRT::strcmp(publicID.get(), kPublicIDs[index].name);
        if (comparison == 0)
          break;
        if (comparison < 0)
          maximum = index - 1;
        else
          minimum = index + 1;

        if (maximum < minimum) {
          
          aParseMode = eDTDMode_full_standards;
          aDocType = eHTML_Strict;
          return;
        }
      }

      switch ((resultFlags & PARSE_DTD_HAVE_SYSTEM_ID)
                ? kPublicIDs[index].mode_if_sysid
                : kPublicIDs[index].mode_if_no_sysid)
      {
        case PubIDInfo::eQuirks:
          aParseMode = eDTDMode_quirks;
          aDocType = eHTML_Quirks;
          break;
        case PubIDInfo::eAlmostStandards:
          aParseMode = eDTDMode_almost_standards;
          aDocType = eHTML_Strict;
          break;
        case PubIDInfo::eFullStandards:
          aParseMode = eDTDMode_full_standards;
          aDocType = eHTML_Strict;
          break;
        default:
          NS_NOTREACHED("no other cases!");
      }
    }
  } else {
    
    aParseMode = eDTDMode_quirks;
    aDocType = eHTML_Quirks;
  }
}

static void
DetermineParseMode(const nsString& aBuffer, nsDTDMode& aParseMode,
                   eParserDocType& aDocType, const nsACString& aMimeType)
{
  if (aMimeType.EqualsLiteral(TEXT_HTML)) {
    DetermineHTMLParseMode(aBuffer, aParseMode, aDocType);
  } else if (aMimeType.EqualsLiteral(TEXT_PLAIN) ||
             aMimeType.EqualsLiteral(TEXT_CSS) ||
             aMimeType.EqualsLiteral(APPLICATION_JAVASCRIPT) ||
             aMimeType.EqualsLiteral(APPLICATION_XJAVASCRIPT) ||
             aMimeType.EqualsLiteral(APPLICATION_JSON) ||
             aMimeType.EqualsLiteral(TEXT_ECMASCRIPT) ||
             aMimeType.EqualsLiteral(APPLICATION_ECMASCRIPT) ||
             aMimeType.EqualsLiteral(TEXT_JAVASCRIPT)) {
    aDocType = ePlainText;
    aParseMode = eDTDMode_quirks;
  } else { 
    aDocType = eXML;
    aParseMode = eDTDMode_full_standards;
  }
}

static nsIDTD*
FindSuitableDTD(CParserContext& aParserContext)
{
  
  aParserContext.mAutoDetectStatus = ePrimaryDetect;

  
  NS_ABORT_IF_FALSE(aParserContext.mParserCommand != eViewSource,
    "The old parser is not supposed to be used for View Source anymore.");

  
  
  if (aParserContext.mDocType != eXML) {
    return new CNavDTD();
  }

  
  NS_ASSERTION(aParserContext.mDocType == eXML, "What are you trying to send me, here?");
  return new nsExpatDriver();
}

NS_IMETHODIMP
nsParser::CancelParsingEvents()
{
  if (mFlags & NS_PARSER_FLAG_PENDING_CONTINUE_EVENT) {
    NS_ASSERTION(mContinueEvent, "mContinueEvent is null");
    
    mContinueEvent = nsnull;
    mFlags &= ~NS_PARSER_FLAG_PENDING_CONTINUE_EVENT;
  }
  return NS_OK;
}















































#define PREFER_LATTER_ERROR_CODE(EXPR1, EXPR2, RV) {                          \
  nsresult RV##__temp = EXPR1;                                                \
  RV = EXPR2;                                                                 \
  if (NS_FAILED(RV)) {                                                        \
    RV = RV##__temp;                                                          \
  }                                                                           \
}








nsresult
nsParser::WillBuildModel(nsString& aFilename)
{
  if (!mParserContext)
    return kInvalidParserContext;

  if (eUnknownDetect != mParserContext->mAutoDetectStatus)
    return NS_OK;

  if (eDTDMode_unknown == mParserContext->mDTDMode ||
      eDTDMode_autodetect == mParserContext->mDTDMode) {
    PRUnichar buf[1025];
    nsFixedString theBuffer(buf, 1024, 0);

    
    
    mParserContext->mScanner->Peek(theBuffer, 1024, mParserContext->mScanner->FirstNonWhitespacePosition());
    DetermineParseMode(theBuffer, mParserContext->mDTDMode,
                       mParserContext->mDocType, mParserContext->mMimeType);
  }

  NS_ASSERTION(!mDTD || !mParserContext->mPrevContext,
               "Clobbering DTD for non-root parser context!");
  mDTD = FindSuitableDTD(*mParserContext);
  NS_ENSURE_TRUE(mDTD, NS_ERROR_OUT_OF_MEMORY);

  nsITokenizer* tokenizer;
  nsresult rv = mParserContext->GetTokenizer(mDTD, mSink, tokenizer);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDTD->WillBuildModel(*mParserContext, tokenizer, mSink);
  nsresult sinkResult = mSink->WillBuildModel(mDTD->GetMode());
  
  
  
  
  
  
  
  return NS_FAILED(sinkResult) ? sinkResult : rv;
}






nsresult
nsParser::DidBuildModel(nsresult anErrorCode)
{
  nsresult result = anErrorCode;

  if (IsComplete()) {
    if (mParserContext && !mParserContext->mPrevContext) {
      
      
      bool terminated = mInternalState == NS_ERROR_HTMLPARSER_STOPPARSING;
      if (mDTD && mSink) {
        nsresult dtdResult =  mDTD->DidBuildModel(anErrorCode),
                sinkResult = mSink->DidBuildModel(terminated);
        
        
        
        
        
        
        
        result = NS_FAILED(sinkResult) ? sinkResult : dtdResult;
      }

      
      mParserContext->mRequest = 0;
    }
  }

  return result;
}







void
nsParser::PushContext(CParserContext& aContext)
{
  NS_ASSERTION(aContext.mPrevContext == mParserContext,
               "Trying to push a context whose previous context differs from "
               "the current parser context.");
  mParserContext = &aContext;
}








CParserContext*
nsParser::PopContext()
{
  CParserContext* oldContext = mParserContext;
  if (oldContext) {
    mParserContext = oldContext->mPrevContext;
    if (mParserContext) {
      
      
      
      if (mParserContext->mStreamListenerState != eOnStop) {
        mParserContext->mStreamListenerState = oldContext->mStreamListenerState;
      }
      
      
      
      if (mParserContext->mTokenizer) {
        mParserContext->mTokenizer->CopyState(oldContext->mTokenizer);
      }
    }
  }
  return oldContext;
}









void
nsParser::SetUnusedInput(nsString& aBuffer)
{
  mUnusedInput = aBuffer;
}






NS_IMETHODIMP
nsParser::Terminate(void)
{
  
  
  if (mInternalState == NS_ERROR_HTMLPARSER_STOPPARSING) {
    return NS_OK;
  }

  nsresult result = NS_OK;
  
  
  nsCOMPtr<nsIParser> kungFuDeathGrip(this);
  mInternalState = result = NS_ERROR_HTMLPARSER_STOPPARSING;

  
  
  
  
  
  CancelParsingEvents();

  
  
  
  
  
  while (mParserContext && mParserContext->mPrevContext) {
    CParserContext *prev = mParserContext->mPrevContext;
    delete mParserContext;
    mParserContext = prev;
  }

  if (mDTD) {
    mDTD->Terminate();
    DidBuildModel(result);
  } else if (mSink) {
    
    
    result = mSink->DidBuildModel(true);
    NS_ENSURE_SUCCESS(result, result);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsParser::ContinueInterruptedParsing()
{
  
  
  
  if (!IsOkToProcessNetworkData()) {
    return NS_OK;
  }

  
  
  
  
  nsresult result=NS_OK;
  nsCOMPtr<nsIParser> kungFuDeathGrip(this);

#ifdef DEBUG
  if (!(mFlags & NS_PARSER_FLAG_PARSER_ENABLED)) {
    NS_WARNING("Don't call ContinueInterruptedParsing on a blocked parser.");
  }
#endif

  bool isFinalChunk = mParserContext &&
                        mParserContext->mStreamListenerState == eOnStop;

  mProcessingNetworkData = true;
  if (mSink) {
    mSink->WillParse();
  }
  result = ResumeParse(true, isFinalChunk); 
  mProcessingNetworkData = false;

  if (result != NS_OK) {
    result=mInternalState;
  }

  return result;
}





NS_IMETHODIMP_(void)
nsParser::BlockParser()
{
  mFlags &= ~NS_PARSER_FLAG_PARSER_ENABLED;
}







NS_IMETHODIMP_(void)
nsParser::UnblockParser()
{
  if (!(mFlags & NS_PARSER_FLAG_PARSER_ENABLED)) {
    mFlags |= NS_PARSER_FLAG_PARSER_ENABLED;
  } else {
    NS_WARNING("Trying to unblock an unblocked parser.");
  }
}




NS_IMETHODIMP_(bool)
nsParser::IsParserEnabled()
{
  return (mFlags & NS_PARSER_FLAG_PARSER_ENABLED) != 0;
}




NS_IMETHODIMP_(bool)
nsParser::IsComplete()
{
  return !(mFlags & NS_PARSER_FLAG_PENDING_CONTINUE_EVENT);
}


void nsParser::HandleParserContinueEvent(nsParserContinueEvent *ev)
{
  
  if (mContinueEvent != ev)
    return;

  mFlags &= ~NS_PARSER_FLAG_PENDING_CONTINUE_EVENT;
  mContinueEvent = nsnull;

  NS_ASSERTION(IsOkToProcessNetworkData(),
               "Interrupted in the middle of a script?");
  ContinueInterruptedParsing();
}

bool
nsParser::CanInterrupt()
{
  return (mFlags & NS_PARSER_FLAG_CAN_INTERRUPT) != 0;
}

bool
nsParser::IsInsertionPointDefined()
{
  return true;
}

void
nsParser::BeginEvaluatingParserInsertedScript()
{
}

void
nsParser::EndEvaluatingParserInsertedScript()
{
}

void
nsParser::MarkAsNotScriptCreated(const char* aCommand)
{
}

bool
nsParser::IsScriptCreated()
{
  return false;
}

void
nsParser::SetCanInterrupt(bool aCanInterrupt)
{
  if (aCanInterrupt) {
    mFlags |= NS_PARSER_FLAG_CAN_INTERRUPT;
  } else {
    mFlags &= ~NS_PARSER_FLAG_CAN_INTERRUPT;
  }
}








NS_IMETHODIMP
nsParser::Parse(nsIURI* aURL,
                nsIRequestObserver* aListener,
                void* aKey,
                nsDTDMode aMode)
{

  NS_PRECONDITION(aURL, "Error: Null URL given");

  nsresult result=kBadURL;
  mObserver = aListener;

  if (aURL) {
    nsCAutoString spec;
    nsresult rv = aURL->GetSpec(spec);
    if (rv != NS_OK) {
      return rv;
    }
    NS_ConvertUTF8toUTF16 theName(spec);

    nsScanner* theScanner = new nsScanner(theName, false, mCharset,
                                          mCharsetSource);
    CParserContext* pc = new CParserContext(mParserContext, theScanner, aKey,
                                            mCommand, aListener);
    if (pc && theScanner) {
      pc->mMultipart = true;
      pc->mContextType = CParserContext::eCTURL;
      pc->mDTDMode = aMode;
      PushContext(*pc);

      result = NS_OK;
    } else {
      result = mInternalState = NS_ERROR_HTMLPARSER_BADCONTEXT;
    }
  }
  return result;
}









NS_IMETHODIMP
nsParser::Parse(const nsAString& aSourceBuffer,
                void* aKey,
                const nsACString& aMimeType,
                bool aLastCall,
                nsDTDMode aMode)
{
  nsresult result = NS_OK;

  
  if (mInternalState == NS_ERROR_HTMLPARSER_STOPPARSING) {
    return result;
  }

  if (!aLastCall && aSourceBuffer.IsEmpty()) {
    
    
    
    
    
    return result;
  }

  
  
  if (aMode == eDTDMode_fragment)
    mCommand = eViewFragment;

  
  
  nsCOMPtr<nsIParser> kungFuDeathGrip(this);

  if (aLastCall || !aSourceBuffer.IsEmpty() || !mUnusedInput.IsEmpty()) {
    
    
    
    
    CParserContext* pc = mParserContext;
    while (pc && pc->mKey != aKey) {
      pc = pc->mPrevContext;
    }

    if (!pc) {
      
      
      nsScanner* theScanner = new nsScanner(mUnusedInput, mCharset, mCharsetSource);
      NS_ENSURE_TRUE(theScanner, NS_ERROR_OUT_OF_MEMORY);

      eAutoDetectResult theStatus = eUnknownDetect;

      if (mParserContext && mParserContext->mMimeType == aMimeType) {
        
        NS_ASSERTION(mDTD, "How come the DTD is null?");

        if (mParserContext) {
          theStatus = mParserContext->mAutoDetectStatus;
          
        }
      }

      pc = new CParserContext(mParserContext, theScanner, aKey, mCommand,
                              0, theStatus, aLastCall);
      NS_ENSURE_TRUE(pc, NS_ERROR_OUT_OF_MEMORY);

      PushContext(*pc);

      pc->mMultipart = !aLastCall; 
      if (pc->mPrevContext) {
        pc->mMultipart |= pc->mPrevContext->mMultipart;
      }

      
      if (pc->mMultipart) {
        pc->mStreamListenerState = eOnDataAvail;
        if (pc->mScanner) {
          pc->mScanner->SetIncremental(true);
        }
      } else {
        pc->mStreamListenerState = eOnStop;
        if (pc->mScanner) {
          pc->mScanner->SetIncremental(false);
        }
      }
      

      pc->mContextType=CParserContext::eCTString;
      pc->SetMimeType(aMimeType);
      if (pc->mPrevContext && aMode == eDTDMode_autodetect) {
        
        pc->mDTDMode = pc->mPrevContext->mDTDMode;
      } else {
        pc->mDTDMode = aMode;
      }

      mUnusedInput.Truncate();

      pc->mScanner->Append(aSourceBuffer);
      
      result = ResumeParse(false, false, false);
    } else {
      pc->mScanner->Append(aSourceBuffer);
      if (!pc->mPrevContext) {
        
        
        if (aLastCall) {
          pc->mStreamListenerState = eOnStop;
          pc->mScanner->SetIncremental(false);
        }

        if (pc == mParserContext) {
          
          
          
          ResumeParse(false, false, false);
        }
      }
    }
  }

  return result;
}

NS_IMETHODIMP
nsParser::ParseFragment(const nsAString& aSourceBuffer,
                        nsTArray<nsString>& aTagStack)
{
  nsresult result = NS_OK;
  nsAutoString  theContext;
  PRUint32 theCount = aTagStack.Length();
  PRUint32 theIndex = 0;

  
  mFlags &= ~NS_PARSER_FLAG_OBSERVERS_ENABLED;

  for (theIndex = 0; theIndex < theCount; theIndex++) {
    theContext.AppendLiteral("<");
    theContext.Append(aTagStack[theCount - theIndex - 1]);
    theContext.AppendLiteral(">");
  }

  if (theCount == 0) {
    
    
    theContext.AssignLiteral(" ");
  }

  
  
  result = Parse(theContext,
                 (void*)&theContext,
                 NS_LITERAL_CSTRING("application/xml"),
                 false,
                 eDTDMode_full_standards);
  if (NS_FAILED(result)) {
    mFlags |= NS_PARSER_FLAG_OBSERVERS_ENABLED;
    return result;
  }

  if (!mSink) {
    
    return NS_ERROR_HTMLPARSER_STOPPARSING;
  }

  nsCOMPtr<nsIFragmentContentSink> fragSink = do_QueryInterface(mSink);
  NS_ASSERTION(fragSink, "ParseFragment requires a fragment content sink");

  fragSink->WillBuildContent();
  
  
  
  
  if (theCount == 0) {
    result = Parse(aSourceBuffer,
                   &theContext,
                   NS_LITERAL_CSTRING("application/xml"),
                   true,
                   eDTDMode_full_standards);
    fragSink->DidBuildContent();
  } else {
    
    
    result = Parse(aSourceBuffer + NS_LITERAL_STRING("</"),
                   &theContext,
                   NS_LITERAL_CSTRING("application/xml"),
                   false,
                   eDTDMode_full_standards);
    fragSink->DidBuildContent();

    if (NS_SUCCEEDED(result)) {
      nsAutoString endContext;
      for (theIndex = 0; theIndex < theCount; theIndex++) {
         
        if (theIndex > 0) {
          endContext.AppendLiteral("</");
        }

        nsString& thisTag = aTagStack[theIndex];
        
        PRInt32 endOfTag = thisTag.FindChar(PRUnichar(' '));
        if (endOfTag == -1) {
          endContext.Append(thisTag);
        } else {
          endContext.Append(Substring(thisTag,0,endOfTag));
        }

        endContext.AppendLiteral(">");
      }

      result = Parse(endContext,
                     &theContext,
                     NS_LITERAL_CSTRING("application/xml"),
                     true,
                     eDTDMode_full_standards);
    }
  }

  mFlags |= NS_PARSER_FLAG_OBSERVERS_ENABLED;

  return result;
}























nsresult
nsParser::ResumeParse(bool allowIteration, bool aIsFinalChunk,
                      bool aCanInterrupt)
{
  nsresult result = NS_OK;

  if ((mFlags & NS_PARSER_FLAG_PARSER_ENABLED) &&
      mInternalState != NS_ERROR_HTMLPARSER_STOPPARSING) {

    result = WillBuildModel(mParserContext->mScanner->GetFilename());
    if (NS_FAILED(result)) {
      mFlags &= ~NS_PARSER_FLAG_CAN_TOKENIZE;
      return result;
    }

    if (mDTD) {
      mSink->WillResume();
      bool theIterationIsOk = true;

      while (result == NS_OK && theIterationIsOk) {
        if (!mUnusedInput.IsEmpty() && mParserContext->mScanner) {
          
          
          
          
          mParserContext->mScanner->UngetReadable(mUnusedInput);
          mUnusedInput.Truncate(0);
        }

        
        
        SetCanInterrupt(aCanInterrupt);
        nsresult theTokenizerResult = (mFlags & NS_PARSER_FLAG_CAN_TOKENIZE)
                                      ? Tokenize(aIsFinalChunk)
                                      : NS_OK;
        result = BuildModel();

        if (result == NS_ERROR_HTMLPARSER_INTERRUPTED && aIsFinalChunk) {
          PostContinueEvent();
        }
        SetCanInterrupt(false);

        theIterationIsOk = theTokenizerResult != kEOF &&
                           result != NS_ERROR_HTMLPARSER_INTERRUPTED;

        
        
        
        

        
        
        if (NS_ERROR_HTMLPARSER_BLOCK == result) {
          mSink->WillInterrupt();
          if (mFlags & NS_PARSER_FLAG_PARSER_ENABLED) {
            
            BlockParser();
          }
          return NS_OK;
        }
        if (NS_ERROR_HTMLPARSER_STOPPARSING == result) {
          
          if (mInternalState != NS_ERROR_HTMLPARSER_STOPPARSING) {
            DidBuildModel(mStreamStatus);
            mInternalState = result;
          }

          return NS_OK;
        }
        if ((NS_OK == result && theTokenizerResult == kEOF) ||
             result == NS_ERROR_HTMLPARSER_INTERRUPTED) {
          bool theContextIsStringBased =
            CParserContext::eCTString == mParserContext->mContextType;

          if (mParserContext->mStreamListenerState == eOnStop ||
              !mParserContext->mMultipart || theContextIsStringBased) {
            if (!mParserContext->mPrevContext) {
              if (mParserContext->mStreamListenerState == eOnStop) {
                DidBuildModel(mStreamStatus);
                return NS_OK;
              }
            } else {
              CParserContext* theContext = PopContext();
              if (theContext) {
                theIterationIsOk = allowIteration && theContextIsStringBased;
                if (theContext->mCopyUnused) {
                  theContext->mScanner->CopyUnusedData(mUnusedInput);
                }

                delete theContext;
              }

              result = mInternalState;
              aIsFinalChunk = mParserContext &&
                              mParserContext->mStreamListenerState == eOnStop;
              
            }
          }
        }

        if (theTokenizerResult == kEOF ||
            result == NS_ERROR_HTMLPARSER_INTERRUPTED) {
          result = (result == NS_ERROR_HTMLPARSER_INTERRUPTED) ? NS_OK : result;
          mSink->WillInterrupt();
        }
      }
    } else {
      mInternalState = result = NS_ERROR_HTMLPARSER_UNRESOLVEDDTD;
    }
  }

  return (result == NS_ERROR_HTMLPARSER_INTERRUPTED) ? NS_OK : result;
}





nsresult
nsParser::BuildModel()
{
  nsITokenizer* theTokenizer = nsnull;

  nsresult result = NS_OK;
  if (mParserContext) {
    result = mParserContext->GetTokenizer(mDTD, mSink, theTokenizer);
  }

  if (NS_SUCCEEDED(result)) {
    if (mDTD) {
      
      bool inDocWrite = !!mParserContext->mPrevContext;
      result = mDTD->BuildModel(theTokenizer,
                                
                                CanInterrupt() && !inDocWrite,
                                !inDocWrite, 
                                &mCharset);
    }
  } else {
    mInternalState = result = NS_ERROR_HTMLPARSER_BADTOKENIZER;
  }
  return result;
}





nsresult
nsParser::OnStartRequest(nsIRequest *request, nsISupports* aContext)
{
  NS_PRECONDITION(eNone == mParserContext->mStreamListenerState,
                  "Parser's nsIStreamListener API was not setup "
                  "correctly in constructor.");
  if (mObserver) {
    mObserver->OnStartRequest(request, aContext);
  }
  mParserContext->mStreamListenerState = eOnStart;
  mParserContext->mAutoDetectStatus = eUnknownDetect;
  mParserContext->mRequest = request;

  NS_ASSERTION(!mParserContext->mPrevContext,
               "Clobbering DTD for non-root parser context!");
  mDTD = nsnull;

  nsresult rv;
  nsCAutoString contentType;
  nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
  if (channel) {
    rv = channel->GetContentType(contentType);
    if (NS_SUCCEEDED(rv)) {
      mParserContext->SetMimeType(contentType);
    }
  }

  rv = NS_OK;

  return rv;
}


#define UTF16_BOM "UTF-16"
#define UTF16_BE "UTF-16BE"
#define UTF16_LE "UTF-16LE"
#define UTF8 "UTF-8"

static inline bool IsSecondMarker(unsigned char aChar)
{
  switch (aChar) {
    case '!':
    case '?':
    case 'h':
    case 'H':
      return true;
    default:
      return false;
  }
}

static bool
DetectByteOrderMark(const unsigned char* aBytes, PRInt32 aLen,
                    nsCString& oCharset, PRInt32& oCharsetSource)
{
 oCharsetSource= kCharsetFromAutoDetection;
 oCharset.Truncate();
 
 
 
 
 
 switch(aBytes[0])
	 {
   case 0x00:
     if((0x3C==aBytes[1]) && (0x00==aBytes[2])) {
        
        if(IsSecondMarker(aBytes[3])) {
           
           oCharset.Assign(UTF16_BE); 
           oCharsetSource = kCharsetFromByteOrderMark;
        } 
     }
   break;
   case 0x3C:
     if(0x00==aBytes[1] && (0x00==aBytes[3])) {
        
        if(IsSecondMarker(aBytes[2])) {
           
           oCharset.Assign(UTF16_LE); 
           oCharsetSource = kCharsetFromByteOrderMark;
        } 
     
     
     } else if(                     (0x3F==aBytes[1]) &&
               (0x78==aBytes[2]) && (0x6D==aBytes[3]) &&
               (0 == PL_strncmp("<?xml", (char*)aBytes, 5 ))) {
       
       
       
       
       
       PRInt32 i;
       bool versionFound = false, encodingFound = false;
       for (i=6; i < aLen && !encodingFound; ++i) {
         
         if ((((char*)aBytes)[i] == '?') && 
           ((i+1) < aLen) &&
           (((char*)aBytes)[i+1] == '>')) {
           break;
         }
         
         if (!versionFound) {
           
           
           
           
           
           if ((((char*)aBytes)[i] == 'n') &&
             (i >= 12) && 
             (0 == PL_strncmp("versio", (char*)(aBytes+i-6), 6 ))) {
             
             char q = 0;
             for (++i; i < aLen; ++i) {
               char qi = ((char*)aBytes)[i];
               if (qi == '\'' || qi == '"') {
                 if (q && q == qi) {
                   
                   versionFound = true;
                   break;
                 } else {
                   
                   q = qi;
                 }
               }
             }
           }
         } else {
           
           
           
           
           
           
           if ((((char*)aBytes)[i] == 'g') &&
             (i >= 25) && 
             (0 == PL_strncmp("encodin", (char*)(aBytes+i-7), 7 ))) {
             PRInt32 encStart = 0;
             char q = 0;
             for (++i; i < aLen; ++i) {
               char qi = ((char*)aBytes)[i];
               if (qi == '\'' || qi == '"') {
                 if (q && q == qi) {
                   PRInt32 count = i - encStart;
                   
                   if (count > 0 && 
                     (0 != PL_strcmp("UTF-16", (char*)(aBytes+encStart)))) {
                     oCharset.Assign((char*)(aBytes+encStart),count);
                     oCharsetSource = kCharsetFromMetaTag;
                   }
                   encodingFound = true;
                   break;
                 } else {
                   encStart = i+1;
                   q = qi;
                 }
               }
             }
           }
         } 
       } 
     }
   break;
   case 0xEF:  
     if((0xBB==aBytes[1]) && (0xBF==aBytes[2])) {
        
        
        oCharset.Assign(UTF8); 
        oCharsetSource= kCharsetFromByteOrderMark;
     }
   break;
   case 0xFE:
     if(0xFF==aBytes[1]) {
        
        oCharset.Assign(UTF16_BOM); 
        oCharsetSource= kCharsetFromByteOrderMark;
     }
   break;
   case 0xFF:
     if(0xFE==aBytes[1]) {
       
       
       oCharset.Assign(UTF16_BOM); 
       oCharsetSource= kCharsetFromByteOrderMark;
     }
   break;
   
   
   
   
 }  
 return !oCharset.IsEmpty();
}

inline const char
GetNextChar(nsACString::const_iterator& aStart,
            nsACString::const_iterator& aEnd)
{
  NS_ASSERTION(aStart != aEnd, "end of buffer");
  return (++aStart != aEnd) ? *aStart : '\0';
}

bool
nsParser::DetectMetaTag(const char* aBytes,
                        PRInt32 aLen,
                        nsCString& aCharset,
                        PRInt32& aCharsetSource)
{
  aCharsetSource= kCharsetFromMetaTag;
  aCharset.SetLength(0);

  
  
  if (!mParserContext->mMimeType.EqualsLiteral(TEXT_HTML)) {
    return false;
  }

  
  
  const nsASingleFragmentCString& str =
      Substring(aBytes, aBytes + NS_MIN(aLen, 2048));
  
  nsACString::const_iterator begin, end;

  str.BeginReading(begin);
  str.EndReading(end);
  nsACString::const_iterator currPos(begin);
  nsACString::const_iterator tokEnd;
  nsACString::const_iterator tagEnd(begin);

  while (currPos != end) {
    if (!FindCharInReadable('<', currPos, end))
      break; 

    if (GetNextChar(currPos, end) == '!') {
      if (GetNextChar(currPos, end) != '-' ||
          GetNextChar(currPos, end) != '-') {
        
        if (!FindCharInReadable('>', currPos, end)) {
          return false; 
        }

        
        ++currPos;
        continue;
      }

      
      bool foundMDC = false;
      bool foundMatch = false;
      while (!foundMDC) {
        if (GetNextChar(currPos, end) == '-' &&
            GetNextChar(currPos, end) == '-') {
          foundMatch = !foundMatch; 
        } else if (currPos == end) {
          return false; 
        } else if (foundMatch && *currPos == '>') {
          foundMDC = true; 
          ++currPos;
        }
      }
      continue; 
    }

    
    tagEnd = currPos;
    if (!FindCharInReadable('>', tagEnd, end))
      break;

    
    if ( (*currPos != 'm' && *currPos != 'M') ||
         (*(++currPos) != 'e' && *currPos != 'E') ||
         (*(++currPos) != 't' && *currPos != 'T') ||
         (*(++currPos) != 'a' && *currPos != 'A') ||
         !nsCRT::IsAsciiSpace(*(++currPos))) {
      currPos = tagEnd;
      continue;
    }

    
    tokEnd = tagEnd;
    if (!CaseInsensitiveFindInReadable(NS_LITERAL_CSTRING("CHARSET"),
                                       currPos, tokEnd)) {
      currPos = tagEnd;
      continue;
    }
    currPos = tokEnd;

    
    while (*currPos == kSpace || *currPos == kNewLine ||
           *currPos == kCR || *currPos == kTab) {
      ++currPos;
    }
    
    if (*currPos != '=') {
      currPos = tagEnd;
      continue;
    }
    ++currPos;
    
    while (*currPos == kSpace || *currPos == kNewLine ||
           *currPos == kCR || *currPos == kTab) {
      ++currPos;
    }

    
    if (*currPos == '\'' || *currPos == '\"')
      ++currPos;

    
    tokEnd = currPos;
    while (*tokEnd != '\'' && *tokEnd != '\"' && tokEnd != tagEnd)
      ++tokEnd;

    
    if (currPos != tokEnd) {
      aCharset.Assign(currPos.get(), tokEnd.get() - currPos.get());
      return true;
    }

    
    currPos = tagEnd;
  }

  return false;
}

typedef struct {
  bool mNeedCharsetCheck;
  nsParser* mParser;
  nsScanner* mScanner;
  nsIRequest* mRequest;
} ParserWriteStruct;







static NS_METHOD
ParserWriteFunc(nsIInputStream* in,
                void* closure,
                const char* fromRawSegment,
                PRUint32 toOffset,
                PRUint32 count,
                PRUint32 *writeCount)
{
  nsresult result;
  ParserWriteStruct* pws = static_cast<ParserWriteStruct*>(closure);
  const char* buf = fromRawSegment;
  PRUint32 theNumRead = count;

  if (!pws) {
    return NS_ERROR_FAILURE;
  }

  if (pws->mNeedCharsetCheck) {
    PRInt32 guessSource;
    nsCAutoString guess;
    nsCAutoString preferred;

    pws->mNeedCharsetCheck = false;
    if (pws->mParser->DetectMetaTag(buf, theNumRead, guess, guessSource) ||
        ((count >= 4) &&
         DetectByteOrderMark((const unsigned char*)buf,
                             theNumRead, guess, guessSource))) {
      nsCOMPtr<nsICharsetAlias> alias(do_GetService(NS_CHARSETALIAS_CONTRACTID));
      result = alias->GetPreferred(guess, preferred);
      
      
      if (NS_SUCCEEDED(result) &&
          ((kCharsetFromByteOrderMark == guessSource) ||
           (!preferred.EqualsLiteral("UTF-16") &&
            !preferred.EqualsLiteral("UTF-16BE") &&
            !preferred.EqualsLiteral("UTF-16LE")))) {
        guess = preferred;
        pws->mParser->SetDocumentCharset(guess, guessSource);
        pws->mParser->SetSinkCharset(preferred);
        nsCOMPtr<nsICachingChannel> channel(do_QueryInterface(pws->mRequest));
        if (channel) {
          nsCOMPtr<nsISupports> cacheToken;
          channel->GetCacheToken(getter_AddRefs(cacheToken));
          if (cacheToken) {
            nsCOMPtr<nsICacheEntryDescriptor> cacheDescriptor(do_QueryInterface(cacheToken));
            if (cacheDescriptor) {
#ifdef DEBUG
              nsresult rv =
#endif
                cacheDescriptor->SetMetaDataElement("charset",
                                                    guess.get());
              NS_ASSERTION(NS_SUCCEEDED(rv),"cannot SetMetaDataElement");
            }
          }
        }
      }
    }
  }

  result = pws->mScanner->Append(buf, theNumRead, pws->mRequest);
  if (NS_SUCCEEDED(result)) {
    *writeCount = count;
  }

  return result;
}

nsresult
nsParser::OnDataAvailable(nsIRequest *request, nsISupports* aContext,
                          nsIInputStream *pIStream, PRUint32 sourceOffset,
                          PRUint32 aLength)
{
  NS_PRECONDITION((eOnStart == mParserContext->mStreamListenerState ||
                   eOnDataAvail == mParserContext->mStreamListenerState),
            "Error: OnStartRequest() must be called before OnDataAvailable()");
  NS_PRECONDITION(NS_InputStreamIsBuffered(pIStream),
                  "Must have a buffered input stream");

  nsresult rv = NS_OK;

  CParserContext *theContext = mParserContext;

  while (theContext && theContext->mRequest != request) {
    theContext = theContext->mPrevContext;
  }

  if (theContext) {
    theContext->mStreamListenerState = eOnDataAvail;

    if (eInvalidDetect == theContext->mAutoDetectStatus) {
      if (theContext->mScanner) {
        nsScannerIterator iter;
        theContext->mScanner->EndReading(iter);
        theContext->mScanner->SetPosition(iter, true);
      }
    }

    PRUint32 totalRead;
    ParserWriteStruct pws;
    pws.mNeedCharsetCheck =
      (0 == sourceOffset) && (mCharsetSource < kCharsetFromMetaTag);
    pws.mParser = this;
    pws.mScanner = theContext->mScanner;
    pws.mRequest = request;

    rv = pIStream->ReadSegments(ParserWriteFunc, &pws, aLength, &totalRead);
    if (NS_FAILED(rv)) {
      return rv;
    }

    
    
    if (IsOkToProcessNetworkData() &&
        theContext->mScanner->FirstNonWhitespacePosition() >= 0) {
      mProcessingNetworkData = true;
      if (mSink) {
        mSink->WillParse();
      }
      rv = ResumeParse();
      mProcessingNetworkData = false;
    }
  } else {
    rv = NS_ERROR_UNEXPECTED;
  }

  return rv;
}





nsresult
nsParser::OnStopRequest(nsIRequest *request, nsISupports* aContext,
                        nsresult status)
{
  nsresult rv = NS_OK;

  CParserContext *pc = mParserContext;
  while (pc) {
    if (pc->mRequest == request) {
      pc->mStreamListenerState = eOnStop;
      pc->mScanner->SetIncremental(false);
      break;
    }

    pc = pc->mPrevContext;
  }

  mStreamStatus = status;

  if (IsOkToProcessNetworkData() && NS_SUCCEEDED(rv)) {
    mProcessingNetworkData = true;
    if (mSink) {
      mSink->WillParse();
    }
    rv = ResumeParse(true, true);
    mProcessingNetworkData = false;
  }

  
  


  
  
  if (mObserver) {
    mObserver->OnStopRequest(request, aContext, status);
  }

  return rv;
}












bool
nsParser::WillTokenize(bool aIsFinalChunk)
{
  if (!mParserContext) {
    return true;
  }

  nsITokenizer* theTokenizer;
  nsresult result = mParserContext->GetTokenizer(mDTD, mSink, theTokenizer);
  NS_ENSURE_SUCCESS(result, false);
  return NS_SUCCEEDED(theTokenizer->WillTokenize(aIsFinalChunk,
                                                 &mTokenAllocator));
}







nsresult nsParser::Tokenize(bool aIsFinalChunk)
{
  nsITokenizer* theTokenizer;

  nsresult result = NS_ERROR_NOT_AVAILABLE;
  if (mParserContext) {
    result = mParserContext->GetTokenizer(mDTD, mSink, theTokenizer);
  }

  if (NS_SUCCEEDED(result)) {
    if (mFlags & NS_PARSER_FLAG_FLUSH_TOKENS) {
      
      
      
      if (theTokenizer->GetCount() != 0) {
        return result;
      }

      
      mFlags &= ~NS_PARSER_FLAG_FLUSH_TOKENS;
    }

    bool flushTokens = false;

    mParserContext->mNumConsumed = 0;

    bool killSink = false;

    WillTokenize(aIsFinalChunk);
    while (NS_SUCCEEDED(result)) {
      mParserContext->mNumConsumed += mParserContext->mScanner->Mark();
      result = theTokenizer->ConsumeToken(*mParserContext->mScanner,
                                          flushTokens);
      if (NS_FAILED(result)) {
        mParserContext->mScanner->RewindToMark();
        if (kEOF == result){
          break;
        }
        if (NS_ERROR_HTMLPARSER_STOPPARSING == result) {
          killSink = true;
          result = Terminate();
          break;
        }
      } else if (flushTokens && (mFlags & NS_PARSER_FLAG_OBSERVERS_ENABLED)) {
        
        
        
        mFlags |= NS_PARSER_FLAG_FLUSH_TOKENS;
        mParserContext->mNumConsumed += mParserContext->mScanner->Mark();
        break;
      }
    }
    DidTokenize(aIsFinalChunk);

    if (killSink) {
      mSink = nsnull;
    }
  } else {
    result = mInternalState = NS_ERROR_HTMLPARSER_BADTOKENIZER;
  }

  return result;
}






bool
nsParser::DidTokenize(bool aIsFinalChunk)
{
  if (!mParserContext) {
    return true;
  }

  nsITokenizer* theTokenizer;
  nsresult rv = mParserContext->GetTokenizer(mDTD, mSink, theTokenizer);
  NS_ENSURE_SUCCESS(rv, false);

  rv = theTokenizer->DidTokenize(aIsFinalChunk);
  return NS_SUCCEEDED(rv);
}







NS_IMETHODIMP
nsParser::GetChannel(nsIChannel** aChannel)
{
  nsresult result = NS_ERROR_NOT_AVAILABLE;
  if (mParserContext && mParserContext->mRequest) {
    result = CallQueryInterface(mParserContext->mRequest, aChannel);
  }
  return result;
}




NS_IMETHODIMP
nsParser::GetDTD(nsIDTD** aDTD)
{
  if (mParserContext) {
    NS_IF_ADDREF(*aDTD = mDTD);
  }

  return NS_OK;
}




nsIStreamListener*
nsParser::GetStreamListener()
{
  return this;
}

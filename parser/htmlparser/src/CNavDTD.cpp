





#include "mozilla/Util.h"

#include "nsDebug.h"
#include "nsIAtom.h"
#include "CNavDTD.h"
#include "nsHTMLTokens.h"
#include "nsCRT.h"
#include "nsParser.h"
#include "nsIHTMLContentSink.h"
#include "nsScanner.h"
#include "prenv.h"
#include "prtypes.h"
#include "prio.h"
#include "plstr.h"
#include "nsDTDUtils.h"
#include "nsHTMLTokenizer.h"
#include "nsParserNode.h"
#include "nsHTMLEntities.h"
#include "nsLinebreakConverter.h"
#include "nsIFormProcessor.h"
#include "nsTArray.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "prmem.h"
#include "nsIServiceManager.h"
#include "nsParserConstants.h"

using namespace mozilla;




#define FONTSTYLE_IGNORE_DEPTH (MAX_REFLOW_DEPTH * 80 / 100)
#define PHRASE_IGNORE_DEPTH    (MAX_REFLOW_DEPTH * 90 / 100)

static NS_DEFINE_CID(kFormProcessorCID, NS_FORMPROCESSOR_CID);

#ifdef DEBUG
static const  char kNullToken[] = "Error: Null token given";
static const  char kInvalidTagStackPos[] = "Error: invalid tag stack position";
#endif

#include "nsElementTable.h"


#define NS_DTD_FLAG_NONE                   0x00000000
#define NS_DTD_FLAG_HAS_OPEN_HEAD          0x00000001
#define NS_DTD_FLAG_HAS_OPEN_BODY          0x00000002
#define NS_DTD_FLAG_HAS_OPEN_FORM          0x00000004
#define NS_DTD_FLAG_HAS_EXPLICIT_HEAD      0x00000008
#define NS_DTD_FLAG_HAD_BODY               0x00000010
#define NS_DTD_FLAG_HAD_FRAMESET           0x00000020
#define NS_DTD_FLAG_ENABLE_RESIDUAL_STYLE  0x00000040
#define NS_DTD_FLAG_ALTERNATE_CONTENT      0x00000080 // NOFRAMES, NOSCRIPT
#define NS_DTD_FLAG_MISPLACED_CONTENT      0x00000100
#define NS_DTD_FLAG_IN_MISPLACED_CONTENT   0x00000200
#define NS_DTD_FLAG_STOP_PARSING           0x00000400

#define NS_DTD_FLAG_HAS_MAIN_CONTAINER     (NS_DTD_FLAG_HAD_BODY |            \
                                            NS_DTD_FLAG_HAD_FRAMESET)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(CNavDTD)
  NS_INTERFACE_MAP_ENTRY(nsIDTD)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDTD)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(CNavDTD)
NS_IMPL_CYCLE_COLLECTING_RELEASE(CNavDTD)

NS_IMPL_CYCLE_COLLECTION_1(CNavDTD, mSink)

CNavDTD::CNavDTD()
  : mMisplacedContent(0),
    mTokenAllocator(0),
    mBodyContext(new nsDTDContext()),
    mTempContext(0),
    mCountLines(true),
    mTokenizer(0),
    mDTDMode(eDTDMode_quirks),
    mDocType(eHTML_Quirks),
    mParserCommand(eViewNormal),
    mLineNumber(1),
    mOpenMapCount(0),
    mHeadContainerPosition(-1),
    mFlags(NS_DTD_FLAG_NONE)
{
}

CNavDTD::~CNavDTD()
{
  delete mBodyContext;
  delete mTempContext;
}

NS_IMETHODIMP
CNavDTD::WillBuildModel(const CParserContext& aParserContext,
                        nsITokenizer* aTokenizer,
                        nsIContentSink* aSink)
{
  nsresult result = NS_OK;

  mFilename = aParserContext.mScanner->GetFilename();
  mFlags = NS_DTD_FLAG_ENABLE_RESIDUAL_STYLE;
  mLineNumber = 1;
  mDTDMode = aParserContext.mDTDMode;
  mParserCommand = aParserContext.mParserCommand;
  mMimeType = aParserContext.mMimeType;
  mDocType = aParserContext.mDocType;
  mTokenizer = aTokenizer;
  mBodyContext->SetNodeAllocator(&mNodeAllocator);

  if (!aParserContext.mPrevContext && aSink) {

    if (!mSink) {
      mSink = do_QueryInterface(aSink, &result);
      if (NS_FAILED(result)) {
        mFlags |= NS_DTD_FLAG_STOP_PARSING;
        return result;
      }
    }

    mFlags |= nsHTMLTokenizer::GetFlags(aSink);

  }

  return result;
}

NS_IMETHODIMP
CNavDTD::BuildModel(nsITokenizer* aTokenizer,
                    bool aCountLines,
                    const nsCString*)
{
  NS_PRECONDITION(mBodyContext != nullptr,
                  "Create a context before calling build model");

  nsresult result = NS_OK;

  if (!aTokenizer) {
    return NS_OK;
  }

  nsITokenizer* const oldTokenizer = mTokenizer;

  mCountLines     = aCountLines;
  mTokenizer      = aTokenizer;
  mTokenAllocator = mTokenizer->GetTokenAllocator();
  
  if (!mSink) {
    return (mFlags & NS_DTD_FLAG_STOP_PARSING)
           ? NS_ERROR_HTMLPARSER_STOPPARSING
           : result;
  }

  if (mBodyContext->GetCount() == 0) {
    CToken* tempToken;
    if (ePlainText == mDocType) {
      tempToken =
        mTokenAllocator->CreateTokenOfType(eToken_start, eHTMLTag_pre);
      if (tempToken) {
        mTokenizer->PushTokenFront(tempToken);
      }
    }

    
    if (!(mFlags & NS_IPARSER_FLAG_FRAMES_ENABLED)) {
      tempToken =
        mTokenAllocator->CreateTokenOfType(eToken_start,
                                           eHTMLTag_body,
                                           NS_LITERAL_STRING("body"));
      if (tempToken) {
        mTokenizer->PushTokenFront(tempToken);
      }
    }

    
    CStartToken* theToken = (CStartToken*)mTokenizer->GetTokenAt(0);
    if (theToken) {
      eHTMLTags theTag = (eHTMLTags)theToken->GetTypeID();
      eHTMLTokenTypes theType = eHTMLTokenTypes(theToken->GetTokenType());
      if (theTag != eHTMLTag_html || theType != eToken_start) {
        tempToken =
          mTokenAllocator->CreateTokenOfType(eToken_start,
                                             eHTMLTag_html,
                                             NS_LITERAL_STRING("html"));
        if (tempToken) {
          mTokenizer->PushTokenFront(tempToken);
        }
      }
    } else {
      tempToken =
        mTokenAllocator->CreateTokenOfType(eToken_start,
                                           eHTMLTag_html,
                                           NS_LITERAL_STRING("html"));
      if (tempToken) {
        mTokenizer->PushTokenFront(tempToken);
      }
    }
  }

  while (NS_SUCCEEDED(result)) {
    if (!(mFlags & NS_DTD_FLAG_STOP_PARSING)) {
      CToken* theToken = mTokenizer->PopToken();
      if (!theToken) {
        break;
      }
      result = HandleToken(theToken);
    } else {
      result = NS_ERROR_HTMLPARSER_STOPPARSING;
      break;
    }

    if (NS_ERROR_HTMLPARSER_INTERRUPTED == mSink->DidProcessAToken()) {
      
      
      
      if (NS_SUCCEEDED(result)) {
        result = NS_ERROR_HTMLPARSER_INTERRUPTED;
        break;
      }
    }
  }

  mTokenizer = oldTokenizer;
  return result;
}

nsresult
CNavDTD::BuildNeglectedTarget(eHTMLTags aTarget,
                              eHTMLTokenTypes aType)
{ 
  NS_ASSERTION(mTokenizer, "tokenizer is null! unable to build target.");
  NS_ASSERTION(mTokenAllocator, "unable to create tokens without an allocator.");
  if (!mTokenizer || !mTokenAllocator) {
    return NS_OK;
  }

  CToken* target = mTokenAllocator->CreateTokenOfType(aType, aTarget);
  NS_ENSURE_TRUE(target, NS_ERROR_OUT_OF_MEMORY);
  mTokenizer->PushTokenFront(target);
  
  
  
  return BuildModel(mTokenizer, mCountLines, 0);
}

NS_IMETHODIMP
CNavDTD::DidBuildModel(nsresult anErrorCode)
{
  nsresult result = NS_OK;

  if (mSink) {
    if (NS_OK == anErrorCode) {
      if (!(mFlags & NS_DTD_FLAG_HAS_MAIN_CONTAINER)) {
        
        
        
        
        
        BuildNeglectedTarget(eHTMLTag_body, eToken_start);
      }
      if (mFlags & NS_DTD_FLAG_MISPLACED_CONTENT) {
        
        

        
        PRInt32 topIndex = mBodyContext->mContextTopIndex;
        
        
        do {
          mFlags &= ~NS_DTD_FLAG_MISPLACED_CONTENT; 

          
          result = HandleSavedTokens(mBodyContext->mContextTopIndex);
          if (NS_FAILED(result)) {
            NS_ERROR("Bug in the DTD");
            break;
          }

          
          
          
          
          
          mBodyContext->mContextTopIndex = topIndex;
        } while (mFlags & NS_DTD_FLAG_MISPLACED_CONTENT);

        mBodyContext->mContextTopIndex = -1; 
      }

      
      
      mFlags &= ~NS_DTD_FLAG_ENABLE_RESIDUAL_STYLE;
      while (mBodyContext->GetCount() > 0) { 
        result = CloseContainersTo(mBodyContext->Last(), false);
        NS_ENSURE_SUCCESS(result, result);
      } 
    } else {
      
      
      
      while (mBodyContext->GetCount() > 0) { 
        nsEntryStack* theChildStyles = 0;
        nsCParserNode* theNode = mBodyContext->Pop(theChildStyles);
        IF_DELETE(theChildStyles, &mNodeAllocator);
        IF_FREE(theNode, &mNodeAllocator);
      } 
    }

    
    
    CToken* theToken = 0;
    while ((theToken = (CToken*)mMisplacedContent.Pop())) {
      IF_FREE(theToken, mTokenAllocator);
    }
  }

  return result;
}

NS_IMETHODIMP_(void) 
CNavDTD::Terminate() 
{ 
  mFlags |= NS_DTD_FLAG_STOP_PARSING; 
}


NS_IMETHODIMP_(PRInt32) 
CNavDTD::GetType() 
{ 
  return NS_IPARSER_FLAG_HTML; 
}

NS_IMETHODIMP_(nsDTDMode)
CNavDTD::GetMode() const
{
  return mDTDMode;
}









static bool
DoesRequireBody(CToken* aToken, nsITokenizer* aTokenizer)
{
  bool result = false;

  if (aToken) {
    eHTMLTags theTag = (eHTMLTags)aToken->GetTypeID();
    if (gHTMLElements[theTag].HasSpecialProperty(kRequiresBody)) {
      if (theTag == eHTMLTag_input) {
        
        
        
        PRInt32 ac = aToken->GetAttributeCount();
        for(PRInt32 i = 0; i < ac; ++i) {
          CAttributeToken* attr = static_cast<CAttributeToken*>
                                             (aTokenizer->GetTokenAt(i));
          const nsSubstring& name = attr->GetKey();
          const nsAString& value = attr->GetValue();
          
          
          if ((name.EqualsLiteral("type") || 
               name.EqualsLiteral("TYPE"))    
              && 
              !(value.EqualsLiteral("hidden") || 
              value.EqualsLiteral("HIDDEN"))) {
            result = true; 
            break;
          }
        }
      } else {
        result = true;
      }
    }
  }
 
  return result;
}

static bool
ValueIsHidden(const nsAString& aValue)
{
  
  
  nsAutoString str(aValue);
  str.Trim("\n\r\t\b");
  return str.LowerCaseEqualsLiteral("hidden");
}




static bool
IsHiddenInput(CToken* aToken, nsITokenizer* aTokenizer)
{
  NS_PRECONDITION(eHTMLTokenTypes(aToken->GetTokenType()) == eToken_start,
                  "Must be start token");
  NS_PRECONDITION(eHTMLTags(aToken->GetTypeID()) == eHTMLTag_input,
                  "Must be <input> tag");
  
  PRInt32 ac = aToken->GetAttributeCount();
  NS_ASSERTION(ac <= aTokenizer->GetCount(),
               "Not enough tokens in the tokenizer");
  
  ac = NS_MIN(ac, aTokenizer->GetCount());
  
  for (PRInt32 i = 0; i < ac; ++i) {
    NS_ASSERTION(eHTMLTokenTypes(aTokenizer->GetTokenAt(i)->GetTokenType()) ==
                   eToken_attribute, "Unexpected token type");
    
    if (eHTMLTokenTypes(aTokenizer->GetTokenAt(i)->GetTokenType()) !=
        eToken_attribute) {
      break;
    }
    
    CAttributeToken* attrToken =
      static_cast<CAttributeToken*>(aTokenizer->GetTokenAt(i));
    if (!attrToken->GetKey().LowerCaseEqualsLiteral("type")) {
      continue;
    }

    return ValueIsHidden(attrToken->GetValue());
  }

  return false;    
}




static bool
HasOpenTagOfType(PRInt32 aType, const nsDTDContext& aContext)
{
  PRInt32 count = aContext.GetCount();

  while (--count >= 0) {
    if (gHTMLElements[aContext.TagAt(count)].IsMemberOf(aType)) {
      return true;
    }
  }

  return false;
}

nsresult
CNavDTD::HandleToken(CToken* aToken)
{
  if (!aToken) {
    return NS_OK;
  }

  nsresult        result   = NS_OK;
  CHTMLToken*     theToken = static_cast<CHTMLToken*>(aToken);
  eHTMLTokenTypes theType  = eHTMLTokenTypes(theToken->GetTokenType());
  eHTMLTags       theTag   = (eHTMLTags)theToken->GetTypeID();

  aToken->SetLineNumber(mLineNumber);

  if (mCountLines) {
    mLineNumber += aToken->GetNewlineCount();
  }

  if (mFlags & NS_DTD_FLAG_MISPLACED_CONTENT) {
    
    static eHTMLTags gLegalElements[] = {
      eHTMLTag_table, eHTMLTag_thead, eHTMLTag_tbody,
      eHTMLTag_tr, eHTMLTag_td, eHTMLTag_th, eHTMLTag_tfoot
    };
    
    
    if (mFlags & NS_DTD_FLAG_IN_MISPLACED_CONTENT) {
      PushIntoMisplacedStack(theToken);
      return NS_OK;
    }

    eHTMLTags theParentTag = mBodyContext->Last();
    if (FindTagInSet(theTag, gLegalElements,
                     ArrayLength(gLegalElements)) ||
        (gHTMLElements[theParentTag].CanContain(theTag, mDTDMode) &&
         
         
         
         
         
         
         
         
         
         
         (!gHTMLElements[theTag].HasSpecialProperty(kLegalOpen) ||
          theTag == eHTMLTag_script)) ||
        (theTag == eHTMLTag_input && theType == eToken_start &&
         FindTagInSet(theParentTag, gLegalElements,
                      ArrayLength(gLegalElements)) &&
         IsHiddenInput(theToken, mTokenizer))) {
      
      
      mFlags &= ~NS_DTD_FLAG_MISPLACED_CONTENT;

      result = HandleSavedTokens(mBodyContext->mContextTopIndex);
      NS_ENSURE_SUCCESS(result, result);

      mBodyContext->mContextTopIndex = -1;
    } else {
      PushIntoMisplacedStack(theToken);
      return result;
    }
  }

  





  switch(theTag) {
    case eHTMLTag_html:
    case eHTMLTag_noframes:
    case eHTMLTag_script:
    case eHTMLTag_doctypeDecl:
    case eHTMLTag_instruction:
      break;

    default:
      if (!gHTMLElements[eHTMLTag_html].SectionContains(theTag, false)) {
        if (!(mFlags & (NS_DTD_FLAG_HAS_MAIN_CONTAINER |
                        NS_DTD_FLAG_ALTERNATE_CONTENT))) {
          
          
          
          

          bool isExclusive = false;
          bool theChildBelongsInHead =
            gHTMLElements[eHTMLTag_head].IsChildOfHead(theTag, isExclusive);
          if (theChildBelongsInHead &&
              !isExclusive &&
              !gHTMLElements[theTag].HasSpecialProperty(kPreferHead)) {
            if (mMisplacedContent.GetSize() == 0 &&
                (!gHTMLElements[theTag].HasSpecialProperty(kPreferBody) ||
                 (mFlags & NS_DTD_FLAG_HAS_EXPLICIT_HEAD))) {
              
              
              
              break;
            }

            
            
            theChildBelongsInHead = false;
          }

          if (!theChildBelongsInHead) {
            eHTMLTags top = mBodyContext->Last();
            NS_ASSERTION(top != eHTMLTag_userdefined,
                         "Userdefined tags should act as leaves in the head");
            if (top != eHTMLTag_html && top != eHTMLTag_head &&
                gHTMLElements[top].CanContain(theTag, mDTDMode)) {
              
              
              
              break;
            }

            
            
            
            
            PushIntoMisplacedStack(aToken);

            if (IsAlternateTag(theTag)) {
              
              
              
              
              
              
              CToken *current = aToken;
              while (current->GetTokenType() != eToken_end ||
                     current->GetTypeID() != theTag) {
                current = static_cast<CToken *>(mTokenizer->PopToken());
                NS_ASSERTION(current, "The tokenizer is not creating good "
                                      "alternate tags");
                PushIntoMisplacedStack(current);
              }

              
              
            }

            if (DoesRequireBody(aToken, mTokenizer)) {
              CToken* theBodyToken =
                mTokenAllocator->CreateTokenOfType(eToken_start,
                                                   eHTMLTag_body,
                                                   NS_LITERAL_STRING("body"));
              result = HandleToken(theBodyToken);
            }
            return result;
          }
        }
      }
  }

  if (theToken) {
    switch (theType) {
      case eToken_text:
      case eToken_start:
      case eToken_whitespace:
      case eToken_newline:
        result = HandleStartToken(theToken);
        break;

      case eToken_end:
        result = HandleEndToken(theToken);
        break;

      case eToken_cdatasection:
      case eToken_comment:
      case eToken_markupDecl:
        result = HandleCommentToken(theToken);
        break;

      case eToken_entity:
        result = HandleEntityToken(theToken);
        break;

      case eToken_attribute:
        result = HandleAttributeToken(theToken);
        break;

      case eToken_instruction:
        result = HandleProcessingInstructionToken(theToken);
        break;

      case eToken_doctypeDecl:
        result = HandleDocTypeDeclToken(theToken);
        break;

      default:
        break;
    }

    IF_FREE(theToken, mTokenAllocator);
    if (result == NS_ERROR_HTMLPARSER_STOPPARSING) {
      mFlags |= NS_DTD_FLAG_STOP_PARSING;
    } else if (NS_FAILED(result) && result != NS_ERROR_HTMLPARSER_BLOCK) {
      result = NS_OK;
    }
  }

  return result;
}

nsresult
CNavDTD::DidHandleStartTag(nsIParserNode& aNode, eHTMLTags aChildTag)
{
  nsresult result = NS_OK;

  switch (aChildTag) {
    case eHTMLTag_pre:
    case eHTMLTag_listing:
      {
        
        

        
        
        CToken* theNextToken = mTokenizer->PeekToken();
        if (ePlainText != mDocType && theNextToken) {
          eHTMLTokenTypes theType = eHTMLTokenTypes(theNextToken->GetTokenType());
          if (eToken_newline == theType) {
            if (mCountLines) {
              mLineNumber += theNextToken->GetNewlineCount();
            }
            theNextToken = mTokenizer->PopToken();
            IF_FREE(theNextToken, mTokenAllocator); 
          }
        }
      }
      break;

    default:
      break;
  }

  return result;
}

PRInt32
CNavDTD::LastOf(eHTMLTags aTagSet[], PRInt32 aCount) const
{
  for (PRInt32 theIndex = mBodyContext->GetCount() - 1; theIndex >= 0;
       --theIndex) {
    if (FindTagInSet((*mBodyContext)[theIndex], aTagSet, aCount)) {
      return theIndex;
    }
  }

  return kNotFound;
}

static bool
CanBeContained(eHTMLTags aChildTag, nsDTDContext& aContext)
{
  





  bool    result = true;
  PRInt32 theCount = aContext.GetCount();

  if (0 < theCount) {
    const TagList* theRootTags = gHTMLElements[aChildTag].GetRootTags();
    const TagList* theSpecialParents =
      gHTMLElements[aChildTag].GetSpecialParents();

    if (theRootTags) {
      PRInt32 theRootIndex = LastOf(aContext, *theRootTags);
      PRInt32 theSPIndex = theSpecialParents
                           ? LastOf(aContext, *theSpecialParents)
                           : kNotFound;
      PRInt32 theChildIndex =
        nsHTMLElement::GetIndexOfChildOrSynonym(aContext, aChildTag);
      PRInt32 theTargetIndex = (theRootIndex > theSPIndex)
                               ? theRootIndex
                               : theSPIndex;

      if (theTargetIndex == theCount-1 ||
          (theTargetIndex == theChildIndex &&
           gHTMLElements[aChildTag].CanContainSelf())) {
        result = true;
      } else {
        result = false;

        static eHTMLTags gTableElements[] = { eHTMLTag_td, eHTMLTag_th };

        PRInt32 theIndex = theCount - 1;
        while (theChildIndex < theIndex) {
          eHTMLTags theParentTag = aContext.TagAt(theIndex--);
          if (gHTMLElements[theParentTag].IsMemberOf(kBlockEntity)  ||
              gHTMLElements[theParentTag].IsMemberOf(kHeading)      ||
              gHTMLElements[theParentTag].IsMemberOf(kPreformatted) ||
              gHTMLElements[theParentTag].IsMemberOf(kFormControl)  ||  
              gHTMLElements[theParentTag].IsMemberOf(kList)) {
            if (!HasOptionalEndTag(theParentTag)) {
              result = true;
              break;
            }
          } else if (FindTagInSet(theParentTag, gTableElements,
                                  ArrayLength(gTableElements))) {
            
            result = true;
            break;
          }
        }
      }
    }
  }

  return result;
}

enum eProcessRule { eNormalOp, eLetInlineContainBlock };

nsresult
CNavDTD::HandleDefaultStartToken(CToken* aToken, eHTMLTags aChildTag,
                                 nsCParserNode *aNode)
{
  NS_PRECONDITION(nullptr != aToken, kNullToken);

  nsresult  result = NS_OK;
  bool    theChildIsContainer = nsHTMLElement::IsContainer(aChildTag);

  
  
  if (mParserCommand != eViewFragment) {
    bool    theChildAgrees = true;
    PRInt32 theIndex = mBodyContext->GetCount();
    PRInt32 theParentContains = 0;

    do {
      eHTMLTags theParentTag = mBodyContext->TagAt(--theIndex);
      if (theParentTag == eHTMLTag_userdefined) {
        continue;
      }

      
      
      static eHTMLTags sTableElements[] = {
        eHTMLTag_table, eHTMLTag_thead, eHTMLTag_tbody,
        eHTMLTag_tr, eHTMLTag_tfoot
      };

      bool isHiddenInputInsideTableElement = false;
      if (aChildTag == eHTMLTag_input &&
          FindTagInSet(theParentTag, sTableElements,
                       ArrayLength(sTableElements))) {
        PRInt32 attrCount = aNode->GetAttributeCount();
        for (PRInt32 attrIndex = 0; attrIndex < attrCount; ++attrIndex) {
          const nsAString& key = aNode->GetKeyAt(attrIndex);
          if (key.LowerCaseEqualsLiteral("type")) {
            isHiddenInputInsideTableElement =
              ValueIsHidden(aNode->GetValueAt(attrIndex));
            break;
          }
        }
      }
      
      
      theParentContains =
        isHiddenInputInsideTableElement || CanContain(theParentTag, aChildTag);
      if (!isHiddenInputInsideTableElement &&
          CanOmit(theParentTag, aChildTag, theParentContains)) {
        HandleOmittedTag(aToken, aChildTag, theParentTag, aNode);
        return NS_OK;
      }

      eProcessRule theRule = eNormalOp;

      if (!theParentContains &&
          (IsBlockElement(aChildTag, theParentTag) &&
           IsInlineElement(theParentTag, theParentTag))) {
        
        if (eHTMLTag_li != aChildTag) {
          nsCParserNode* theParentNode = mBodyContext->PeekNode();
          if (theParentNode && theParentNode->mToken->IsWellFormed()) {
            theRule = eLetInlineContainBlock;
          }
        }
      }

      switch (theRule) {
        case eNormalOp:
          theChildAgrees = true;
          if (theParentContains) {
            eHTMLTags theAncestor = gHTMLElements[aChildTag].mRequiredAncestor;
            if (eHTMLTag_unknown != theAncestor) {
              theChildAgrees = HasOpenContainer(theAncestor);
            }

            if (theChildAgrees && theChildIsContainer) {
              if (theParentTag != aChildTag) {
                
                
                if (gHTMLElements[aChildTag].ShouldVerifyHierarchy()) {
                  PRInt32 theChildIndex =
                    nsHTMLElement::GetIndexOfChildOrSynonym(*mBodyContext,
                                                            aChildTag);

                  if (kNotFound < theChildIndex && theChildIndex < theIndex) {
                    
























                    theChildAgrees = CanBeContained(aChildTag, *mBodyContext);
                  }
                }
              }
            }
          }

          if (!(theParentContains && theChildAgrees)) {
            if (!CanPropagate(theParentTag, aChildTag, theParentContains)) {
              if (theChildIsContainer || !theParentContains) {
                if (!theChildAgrees &&
                    !gHTMLElements[aChildTag].CanAutoCloseTag(*mBodyContext,
                                                              theIndex,
                                                              aChildTag)) {
                  
                  
                  
                  
                  
                  
                  
                  return result;
                } else if (mBodyContext->mContextTopIndex > 0 &&
                           theIndex <= mBodyContext->mContextTopIndex) {
                  
                  
                  
                  
                  theParentContains = true;
                } else {
                  CloseContainersTo(theIndex, aChildTag, true);
                }
              } else {
                break;
              }
            } else {
              CreateContextStackFor(theParentTag, aChildTag);
              theIndex = mBodyContext->GetCount();
            }
          }
          break;

        case eLetInlineContainBlock:
          
          theParentContains = theChildAgrees = true;
          break;

        default:
          NS_NOTREACHED("Invalid rule detected");
      }
    } while (!(theParentContains && theChildAgrees));
  }

  if (theChildIsContainer) {
    result = OpenContainer(aNode, aChildTag);
  } else {
    result = AddLeaf(aNode);
  }

  return result;
}

nsresult
CNavDTD::WillHandleStartTag(CToken* aToken, eHTMLTags aTag,
                            nsIParserNode& aNode)
{
  nsresult result = NS_OK;

  PRInt32 stackDepth = mBodyContext->GetCount();
  if (stackDepth >= FONTSTYLE_IGNORE_DEPTH &&
      gHTMLElements[aTag].IsMemberOf(kFontStyle)) {
    
    return kHierarchyTooDeep;
  }

  if (stackDepth >= PHRASE_IGNORE_DEPTH &&
      gHTMLElements[aTag].IsMemberOf(kPhrase)) {
    
    return kHierarchyTooDeep;
  }

  






  if (stackDepth > MAX_REFLOW_DEPTH) {
    if (nsHTMLElement::IsContainer(aTag) &&
        !gHTMLElements[aTag].HasSpecialProperty(kHandleStrayTag)) {
      
      
      
      
      
      while (stackDepth != MAX_REFLOW_DEPTH && NS_SUCCEEDED(result)) {
        result = CloseContainersTo(mBodyContext->Last(), false);
        --stackDepth;
      }
    }
  }

  return result;
}

static void
PushMisplacedAttributes(nsIParserNode& aNode, nsDeque& aDeque)
{
  nsCParserNode& theAttrNode = static_cast<nsCParserNode &>(aNode);

  for (PRInt32 count = aNode.GetAttributeCount(); count > 0; --count) {
    CToken* theAttrToken = theAttrNode.PopAttributeTokenFront();
    if (theAttrToken) {
      theAttrToken->SetNewlineCount(0);
      aDeque.Push(theAttrToken);
    }
  }
}

void
CNavDTD::HandleOmittedTag(CToken* aToken, eHTMLTags aChildTag,
                          eHTMLTags aParent, nsIParserNode* aNode)
{
  NS_PRECONDITION(mBodyContext != nullptr, "need a context to work with");

  
  
  
  
  PRInt32 theTagCount = mBodyContext->GetCount();
  bool pushToken = false;

  if (gHTMLElements[aParent].HasSpecialProperty(kBadContentWatch) &&
      !nsHTMLElement::IsWhitespaceTag(aChildTag)) {
    eHTMLTags theTag = eHTMLTag_unknown;

    
    
    if (mFlags & NS_DTD_FLAG_HAS_OPEN_HEAD) {
      NS_ASSERTION(!(mFlags & NS_DTD_FLAG_HAS_MAIN_CONTAINER),
                   "Bad state");
      return;
    }

    
    while (theTagCount > 0) {
      theTag = mBodyContext->TagAt(--theTagCount);
      if (!gHTMLElements[theTag].HasSpecialProperty(kBadContentWatch)) {
        
        mBodyContext->mContextTopIndex = theTagCount;
        break;
      }
    }

    if (mBodyContext->mContextTopIndex > -1) {
      pushToken = true;

      
      mFlags |= NS_DTD_FLAG_MISPLACED_CONTENT;
    }
  }

  if (aChildTag != aParent &&
      gHTMLElements[aParent].HasSpecialProperty(kSaveMisplaced)) {
    NS_ASSERTION(!pushToken, "A strange element has both kBadContentWatch "
                             "and kSaveMisplaced");
    pushToken = true;
  }

  if (pushToken) {
    
    IF_HOLD(aToken);
    PushIntoMisplacedStack(aToken);

    
    PushMisplacedAttributes(*aNode, mMisplacedContent);
  }
}








nsresult
CNavDTD::HandleKeyGen(nsIParserNode* aNode)
{
  nsresult result = NS_OK;

  nsCOMPtr<nsIFormProcessor> theFormProcessor =
           do_GetService(kFormProcessorCID, &result);
  if (NS_FAILED(result)) {
    return result;
  }

  PRInt32      theAttrCount = aNode->GetAttributeCount();
  nsTArray<nsString> theContent;
  nsAutoString theAttribute;
  nsAutoString theFormType;
  CToken*      theToken = nullptr;

  theFormType.AssignLiteral("select");

  result = theFormProcessor->ProvideContent(theFormType, theContent,
                                            theAttribute);
  if (NS_FAILED(result)) {
    return result;
  }
  PRInt32   theIndex = 0;

  
  
  
  theToken = mTokenAllocator->CreateTokenOfType(eToken_end,
                                                eHTMLTag_select);
  NS_ENSURE_TRUE(theToken, NS_ERROR_OUT_OF_MEMORY);
  mTokenizer->PushTokenFront(theToken);

  for (theIndex = theContent.Length()-1; theIndex > -1; --theIndex) {
    theToken = mTokenAllocator->CreateTokenOfType(eToken_text,
                                                  eHTMLTag_text,
                                                  theContent[theIndex]);
    NS_ENSURE_TRUE(theToken, NS_ERROR_OUT_OF_MEMORY);
    mTokenizer->PushTokenFront(theToken);

    theToken = mTokenAllocator->CreateTokenOfType(eToken_start,
                                                  eHTMLTag_option);
    NS_ENSURE_TRUE(theToken, NS_ERROR_OUT_OF_MEMORY);
    mTokenizer->PushTokenFront(theToken);
  }

  
  
  
  theToken = mTokenAllocator->CreateTokenOfType(eToken_attribute,
                                                eHTMLTag_unknown,
                                                theAttribute);
  NS_ENSURE_TRUE(theToken, NS_ERROR_OUT_OF_MEMORY);

  ((CAttributeToken*)theToken)->SetKey(NS_LITERAL_STRING("_moz-type"));
  mTokenizer->PushTokenFront(theToken);

  
  
  
  for (theIndex = theAttrCount; theIndex > 0; --theIndex) {
    mTokenizer->PushTokenFront(((nsCParserNode*)aNode)->PopAttributeToken());
  }

  theToken = mTokenAllocator->CreateTokenOfType(eToken_start,
                                                eHTMLTag_select);
  NS_ENSURE_TRUE(theToken, NS_ERROR_OUT_OF_MEMORY);

  
  theToken->SetAttributeCount(theAttrCount + 1);
  mTokenizer->PushTokenFront(theToken);

  return result;
}

bool
CNavDTD::IsAlternateTag(eHTMLTags aTag)
{
  switch (aTag) {
    case eHTMLTag_noembed:
      return true;

    case eHTMLTag_noscript:
      return (mFlags & NS_IPARSER_FLAG_SCRIPT_ENABLED) != 0;

    case eHTMLTag_iframe:
    case eHTMLTag_noframes:
      return (mFlags & NS_IPARSER_FLAG_FRAMES_ENABLED) != 0;

    default:
      return false;
  }
}

nsresult
CNavDTD::HandleStartToken(CToken* aToken)
{
  NS_PRECONDITION(nullptr != aToken, kNullToken);

  nsCParserNode* theNode = mNodeAllocator.CreateNode(aToken, mTokenAllocator);
  NS_ENSURE_TRUE(theNode, NS_ERROR_OUT_OF_MEMORY);

  eHTMLTags     theChildTag = (eHTMLTags)aToken->GetTypeID();
  PRInt16       attrCount   = aToken->GetAttributeCount();
  eHTMLTags     theParent   = mBodyContext->Last();
  nsresult      result      = NS_OK;

  if (attrCount > 0) {
    result = CollectAttributes(theNode, theChildTag, attrCount);
  }

  if (NS_OK == result) {
    result = WillHandleStartTag(aToken, theChildTag, *theNode);
    if (NS_OK == result) {
      bool isTokenHandled = false;
      bool theHeadIsParent = false;

      if (nsHTMLElement::IsSectionTag(theChildTag)) {
        switch (theChildTag) {
          case eHTMLTag_html:
            if (mBodyContext->GetCount() > 0) {
              result = OpenContainer(theNode, theChildTag);
              isTokenHandled = true;
            }
            break;

          case eHTMLTag_body:
            if (mFlags & NS_DTD_FLAG_HAS_OPEN_BODY) {
              result = OpenContainer(theNode, theChildTag);
              isTokenHandled=true;
            }
            break;

          case eHTMLTag_head:
            mFlags |= NS_DTD_FLAG_HAS_EXPLICIT_HEAD;

            if (mFlags & NS_DTD_FLAG_HAS_MAIN_CONTAINER) {
              HandleOmittedTag(aToken, theChildTag, theParent, theNode);
              isTokenHandled = true;
            }
            break;

          default:
            break;
        }
      }

      bool isExclusive = false;
      theHeadIsParent = nsHTMLElement::IsChildOfHead(theChildTag, isExclusive);

      switch (theChildTag) {
        case eHTMLTag_area:
          if (!mOpenMapCount) {
            isTokenHandled = true;
          }

          if (mOpenMapCount > 0 && mSink) {
            result = mSink->AddLeaf(*theNode);
            isTokenHandled = true;
          }
	  
          break;

        case eHTMLTag_image:
          aToken->SetTypeID(theChildTag = eHTMLTag_img);
          break;

        case eHTMLTag_keygen:
          result = HandleKeyGen(theNode);
          isTokenHandled = true;
          break;

        case eHTMLTag_script:
          
          
          
          
          isExclusive = !(mFlags & NS_DTD_FLAG_HAD_BODY);
          break;

        default:;
      }

      if (!isTokenHandled) {
        bool prefersBody =
          gHTMLElements[theChildTag].HasSpecialProperty(kPreferBody);

        
        
        
        
        theHeadIsParent = theHeadIsParent &&
          (isExclusive ||
           (prefersBody
            ? (mFlags & NS_DTD_FLAG_HAS_EXPLICIT_HEAD) &&
              (mFlags & NS_DTD_FLAG_HAS_OPEN_HEAD)
            : !(mFlags & NS_DTD_FLAG_HAS_MAIN_CONTAINER)));

        if (theHeadIsParent) {
          
          result = AddHeadContent(theNode);
        } else {
          result = HandleDefaultStartToken(aToken, theChildTag, theNode);
        }
      }

      
      if (NS_OK == result) {
        DidHandleStartTag(*theNode, theChildTag);
      }
    }
  }

  if (kHierarchyTooDeep == result) {
    
    
    result = NS_OK;
  }

  IF_FREE(theNode, &mNodeAllocator);
  return result;
}










static bool
HasCloseablePeerAboveRoot(const TagList& aRootTagList, nsDTDContext& aContext,
                          eHTMLTags aTag, bool anEndTag)
{
  PRInt32  theRootIndex = LastOf(aContext, aRootTagList);
  const TagList* theCloseTags = anEndTag
                                ? gHTMLElements[aTag].GetAutoCloseEndTags()
                                : gHTMLElements[aTag].GetAutoCloseStartTags();
  PRInt32 theChildIndex = -1;

  if (theCloseTags) {
    theChildIndex=LastOf(aContext, *theCloseTags);
  } else if (anEndTag || !gHTMLElements[aTag].CanContainSelf()) {
    theChildIndex = aContext.LastOf(aTag);
  }

  
  
  return theRootIndex<=theChildIndex;
}











static eHTMLTags
FindAutoCloseTargetForEndTag(eHTMLTags aCurrentTag, nsDTDContext& aContext,
                             nsDTDMode aMode)
{
  int theTopIndex = aContext.GetCount();
  eHTMLTags thePrevTag = aContext.Last();

  if (nsHTMLElement::IsContainer(aCurrentTag)) {
    PRInt32 theChildIndex =
      nsHTMLElement::GetIndexOfChildOrSynonym(aContext, aCurrentTag);

    if (kNotFound < theChildIndex) {
      if (thePrevTag == aContext[theChildIndex]) {
        return aContext[theChildIndex];
      }

      if (nsHTMLElement::IsBlockCloser(aCurrentTag)) {
        











        const TagList* theCloseTags =
          gHTMLElements[aCurrentTag].GetAutoCloseEndTags();
        const TagList* theRootTags =
          gHTMLElements[aCurrentTag].GetEndRootTags();

        if (theCloseTags) {
          
          while (theChildIndex < --theTopIndex) {
            eHTMLTags theNextTag = aContext[theTopIndex];
            if (!FindTagInSet(theNextTag, theCloseTags->mTags,
                              theCloseTags->mCount) &&
                FindTagInSet(theNextTag, theRootTags->mTags,
                             theRootTags->mCount)) {
              
              return eHTMLTag_unknown;
            }

            
            
          }

          eHTMLTags theTarget = aContext.TagAt(theChildIndex);
          if (aCurrentTag != theTarget) {
            aCurrentTag = theTarget;
          }
          
          return aCurrentTag;
        } else if (theRootTags) {
          
          
          if (HasCloseablePeerAboveRoot(*theRootTags, aContext, aCurrentTag,
                                        true)) {
            return aCurrentTag;
          } else {
            return eHTMLTag_unknown;
          }
        }
      } else {
        
        
        
        return gHTMLElements[aCurrentTag].GetCloseTargetForEndTag(aContext,
                                                                  theChildIndex,
                                                                  aMode);
      }
    }
  }

  return eHTMLTag_unknown;
}

static void
StripWSFollowingTag(eHTMLTags aChildTag, nsITokenizer* aTokenizer,
                    nsTokenAllocator* aTokenAllocator, PRInt32* aNewlineCount)
{
  if (!aTokenizer || !aTokenAllocator) {
    return;
  }

  CToken* theToken = aTokenizer->PeekToken();

  PRInt32 newlineCount = 0;
  while (theToken) {
    eHTMLTokenTypes theType = eHTMLTokenTypes(theToken->GetTokenType());

    switch(theType) {
      case eToken_newline:
      case eToken_whitespace:
        theToken = aTokenizer->PopToken();
        newlineCount += theToken->GetNewlineCount();
        IF_FREE(theToken, aTokenAllocator);

        theToken = aTokenizer->PeekToken();
        break;

      default:
        theToken = nullptr;
        break;
    }
  }

  if (aNewlineCount) {
    *aNewlineCount += newlineCount;
  }
}













nsresult
CNavDTD::HandleEndToken(CToken* aToken)
{
  NS_PRECONDITION(nullptr != aToken, kNullToken);

  nsresult    result = NS_OK;
  eHTMLTags   theChildTag = (eHTMLTags)aToken->GetTypeID();

  
  CollectAttributes(nullptr, theChildTag, aToken->GetAttributeCount());

  switch (theChildTag) {
    case eHTMLTag_link:
    case eHTMLTag_meta:
      break;

    case eHTMLTag_head:
      StripWSFollowingTag(theChildTag, mTokenizer, mTokenAllocator,
                          !mCountLines ? nullptr : &mLineNumber);
      if (mBodyContext->LastOf(eHTMLTag_head) != kNotFound) {
        result = CloseContainersTo(eHTMLTag_head, false);
      }
      mFlags &= ~NS_DTD_FLAG_HAS_EXPLICIT_HEAD;
      break;

    case eHTMLTag_form:
      result = CloseContainer(eHTMLTag_form, false);
      break;

    case eHTMLTag_br:
      {
        
        
        if (eDTDMode_quirks == mDTDMode) {
          
          
          CToken* theToken = mTokenAllocator->CreateTokenOfType(eToken_start,
                                                                theChildTag);
          result = HandleToken(theToken);
        }
      }
      break;

    case eHTMLTag_body:
    case eHTMLTag_html:
      StripWSFollowingTag(theChildTag, mTokenizer, mTokenAllocator,
                          !mCountLines ? nullptr : &mLineNumber);
      break;

    case eHTMLTag_script:
      
      
      
      
      
      if (mBodyContext->Last() != eHTMLTag_script) {
        
        NS_ASSERTION(mBodyContext->LastOf(eHTMLTag_script) == kNotFound,
                     "Mishandling scripts in CNavDTD");
        break;
      }

      mBodyContext->Pop();
      result = CloseContainer(eHTMLTag_script, aToken->IsInError());
      break;

    default:
     {
        
        
        if (gHTMLElements[theChildTag].CanOmitEndTag()) {
          PopStyle(theChildTag);
        } else {
          eHTMLTags theParentTag = mBodyContext->Last();

          
          
          if (nsHTMLElement::IsResidualStyleTag(theChildTag)) {
            result = OpenTransientStyles(theChildTag);
            if (NS_FAILED(result)) {
              return result;
            }
          }

          if (kNotFound ==
                nsHTMLElement::GetIndexOfChildOrSynonym(*mBodyContext,
                                                        theChildTag)) {
            
            
            
            
            
            
            static eHTMLTags gBarriers[] = {
              eHTMLTag_thead, eHTMLTag_tbody, eHTMLTag_tfoot, eHTMLTag_table
            };

            if (!FindTagInSet(theParentTag, gBarriers,
                              ArrayLength(gBarriers)) &&
                nsHTMLElement::IsResidualStyleTag(theChildTag)) {
              
              mBodyContext->RemoveStyle(theChildTag);
            }

            
            
            
            if (gHTMLElements[theChildTag].HasSpecialProperty(kHandleStrayTag) &&
                mDTDMode != eDTDMode_full_standards &&
                mDTDMode != eDTDMode_almost_standards) {
              
              
              
              
              PRInt32 theParentContains = -1;
              if (!CanOmit(theParentTag, theChildTag, theParentContains)) {
                CToken* theStartToken =
                  mTokenAllocator->CreateTokenOfType(eToken_start, theChildTag);
                NS_ENSURE_TRUE(theStartToken, NS_ERROR_OUT_OF_MEMORY);

                
                
                if (!(mFlags & NS_DTD_FLAG_IN_MISPLACED_CONTENT)) {
                  
                  
                  
                  IF_HOLD(aToken);
                  mTokenizer->PushTokenFront(aToken);
                  mTokenizer->PushTokenFront(theStartToken);
                } else {
                  
                  
                  
                  result = HandleToken(theStartToken);
                  NS_ENSURE_SUCCESS(result, result);

                  IF_HOLD(aToken);
                  result = HandleToken(aToken);
                }
              }
            }
            return result;
          }
          if (result == NS_OK) {
            eHTMLTags theTarget =
              FindAutoCloseTargetForEndTag(theChildTag, *mBodyContext,
                                           mDTDMode);
            if (eHTMLTag_unknown != theTarget) {
              result = CloseContainersTo(theTarget, false);
            }
          }
        }
      }
      break;
  }

  return result;
}











nsresult
CNavDTD::HandleSavedTokens(PRInt32 anIndex)
{
  NS_PRECONDITION(mBodyContext != nullptr && mBodyContext->GetCount() > 0, "invalid context");

  nsresult  result = NS_OK;

  if (mSink && (anIndex > kNotFound)) {
    PRInt32 theBadTokenCount = mMisplacedContent.GetSize();

    if (theBadTokenCount > 0) {
      mFlags |= NS_DTD_FLAG_IN_MISPLACED_CONTENT;

      if (!mTempContext && !(mTempContext = new nsDTDContext())) {
        return NS_ERROR_OUT_OF_MEMORY;
      }

      CToken*   theToken;
      eHTMLTags theTag;
      PRInt32   attrCount;
      PRInt32   theTopIndex = anIndex + 1;
      PRInt32   theTagCount = mBodyContext->GetCount();

      
      result = mSink->BeginContext(anIndex);
      
      NS_ENSURE_SUCCESS(result, result);

      
      mBodyContext->MoveEntries(*mTempContext, theTagCount - theTopIndex);

      
      while (theBadTokenCount-- > 0){
        theToken = (CToken*)mMisplacedContent.PopFront();
        if (theToken) {
          theTag       = (eHTMLTags)theToken->GetTypeID();
          attrCount    = theToken->GetAttributeCount();
          
          
          
          
          
          nsDeque temp;
          for (PRInt32 j = 0; j < attrCount; ++j) {
            CToken* theAttrToken = (CToken*)mMisplacedContent.PopFront();
            if (theAttrToken) {
              temp.Push(theAttrToken);
            }
            theBadTokenCount--;
          }
          mTokenizer->PrependTokens(temp);

          if (eToken_end == theToken->GetTokenType()) {
            
            
            
            
            
            eHTMLTags closed = FindAutoCloseTargetForEndTag(theTag, *mBodyContext,
                                                            mDTDMode);
            PRInt32 theIndex = closed != eHTMLTag_unknown
                               ? mBodyContext->LastOf(closed)
                               : kNotFound;

            if (theIndex != kNotFound &&
                theIndex <= mBodyContext->mContextTopIndex) {
              IF_FREE(theToken, mTokenAllocator);
              continue;
            }
          }

          
          
          
          
          result = HandleToken(theToken);
        }
      }

      if (theTopIndex != mBodyContext->GetCount()) {
        
        
        
        CloseContainersTo(theTopIndex, mBodyContext->TagAt(theTopIndex),
                          true);
      }      

      
      
      mTempContext->MoveEntries(*mBodyContext, theTagCount - theTopIndex);

      
      mSink->EndContext(anIndex);

      mFlags &= ~NS_DTD_FLAG_IN_MISPLACED_CONTENT;
    }
  }
  return result;
}










nsresult
CNavDTD::HandleEntityToken(CToken* aToken)
{
  NS_PRECONDITION(nullptr != aToken, kNullToken);

  nsresult  result = NS_OK;

  const nsSubstring& theStr = aToken->GetStringValue();

  if (kHashsign != theStr.First() &&
      -1 == nsHTMLEntities::EntityToUnicode(theStr)) {
    
    
    CToken *theToken;

    nsAutoString entityName;
    entityName.AssignLiteral("&");
    entityName.Append(theStr);
    theToken = mTokenAllocator->CreateTokenOfType(eToken_text, eHTMLTag_text,
                                                  entityName);
    NS_ENSURE_TRUE(theToken, NS_ERROR_OUT_OF_MEMORY);

    
    return HandleToken(theToken);
  }

  eHTMLTags theParentTag = mBodyContext->Last();
  nsCParserNode* theNode = mNodeAllocator.CreateNode(aToken, mTokenAllocator);
  NS_ENSURE_TRUE(theNode, NS_ERROR_OUT_OF_MEMORY);

  PRInt32 theParentContains = -1;
  if (CanOmit(theParentTag, eHTMLTag_entity, theParentContains)) {
    eHTMLTags theCurrTag = (eHTMLTags)aToken->GetTypeID();
    HandleOmittedTag(aToken, theCurrTag, theParentTag, theNode);
  } else {
    result = AddLeaf(theNode);
  }
  IF_FREE(theNode, &mNodeAllocator);
  return result;
}











nsresult
CNavDTD::HandleCommentToken(CToken* aToken)
{
  NS_PRECONDITION(nullptr != aToken, kNullToken);
  return NS_OK;
}












nsresult
CNavDTD::HandleAttributeToken(CToken* aToken)
{
  NS_ERROR("attribute encountered -- this shouldn't happen.");
  return NS_OK;
}









nsresult
CNavDTD::HandleProcessingInstructionToken(CToken* aToken)
{
  NS_PRECONDITION(nullptr != aToken, kNullToken);
  return NS_OK;
}









nsresult
CNavDTD::HandleDocTypeDeclToken(CToken* aToken)
{
  NS_PRECONDITION(nullptr != aToken, kNullToken);

  CDoctypeDeclToken* theToken = static_cast<CDoctypeDeclToken*>(aToken);
  nsAutoString docTypeStr(theToken->GetStringValue());
  
  if (mCountLines) {
    mLineNumber += docTypeStr.CountChar(kNewLine);
  }

  PRInt32 len = docTypeStr.Length();
  PRInt32 pos = docTypeStr.RFindChar(kGreaterThan);
  if (pos != kNotFound) {
    
    docTypeStr.Cut(pos, len - pos);
  }

  
  docTypeStr.Cut(0, 2);
  theToken->SetStringValue(docTypeStr);
  return NS_OK;
}










nsresult
CNavDTD::CollectAttributes(nsIParserNode *aNode, eHTMLTags aTag, PRInt32 aCount)
{
  int attr = 0;
  nsresult result = NS_OK;
  int theAvailTokenCount = mTokenizer->GetCount();

  if (aCount <= theAvailTokenCount) {
    CToken* theToken;
    for (attr = 0; attr < aCount; ++attr) {
      theToken = mTokenizer->PopToken();
      if (theToken) {
        eHTMLTokenTypes theType = eHTMLTokenTypes(theToken->GetTokenType());
        if (theType != eToken_attribute) {
          
          
          
          mTokenizer->PushTokenFront(theToken);
          break;
        }

        if (mCountLines) {
          mLineNumber += theToken->GetNewlineCount();
        }

        if (aNode) {
          
          
          if (!((CAttributeToken*)theToken)->GetKey().IsEmpty()) {
            aNode->AddAttribute(theToken);
          } else {
            IF_FREE(theToken, mTokenAllocator);
          }
        } else {
          IF_FREE(theToken, mTokenAllocator);
        }
      }
    }
  } else {
    result = kEOF;
  }
  return result;
}










NS_IMETHODIMP_(bool)
CNavDTD::CanContain(PRInt32 aParent, PRInt32 aChild) const
{
  bool result = gHTMLElements[aParent].CanContain((eHTMLTags)aChild, mDTDMode);

  if (eHTMLTag_nobr == aChild &&
      IsInlineElement(aParent, aParent) &&
      HasOpenContainer(eHTMLTag_nobr)) {
    return false;
  }

  return result;
}










bool
CNavDTD::IsBlockElement(PRInt32 aTagID, PRInt32 aParentID) const
{
  eHTMLTags theTag = (eHTMLTags)aTagID;

  return (theTag > eHTMLTag_unknown && theTag < eHTMLTag_userdefined) &&
          (gHTMLElements[theTag].IsMemberOf(kBlock)        ||
           gHTMLElements[theTag].IsMemberOf(kBlockEntity)  ||
           gHTMLElements[theTag].IsMemberOf(kHeading)      ||
           gHTMLElements[theTag].IsMemberOf(kPreformatted) ||
           gHTMLElements[theTag].IsMemberOf(kList));
}










bool
CNavDTD::IsInlineElement(PRInt32 aTagID, PRInt32 aParentID) const
{
  eHTMLTags theTag = (eHTMLTags)aTagID;

  return (theTag > eHTMLTag_unknown && theTag < eHTMLTag_userdefined) &&
          (gHTMLElements[theTag].IsMemberOf(kInlineEntity) ||
           gHTMLElements[theTag].IsMemberOf(kFontStyle)    ||
           gHTMLElements[theTag].IsMemberOf(kPhrase)       ||
           gHTMLElements[theTag].IsMemberOf(kSpecial)      ||
           gHTMLElements[theTag].IsMemberOf(kFormControl));
}











bool
CNavDTD::CanPropagate(eHTMLTags aParent, eHTMLTags aChild,
                      PRInt32 aParentContains)
{
  bool result = false;
  if (aParentContains == -1) {
    aParentContains = CanContain(aParent, aChild);
  }

  if (aParent == aChild) {
    return result;
  }

  if (nsHTMLElement::IsContainer(aChild)) {
    mScratch.Truncate();
    if (!gHTMLElements[aChild].HasSpecialProperty(kNoPropagate)) {
      if (nsHTMLElement::IsBlockParent(aParent) ||
          gHTMLElements[aParent].GetSpecialChildren()) {
        result = ForwardPropagate(mScratch, aParent, aChild);
        if (!result) {
          if (eHTMLTag_unknown != aParent) {
            result = BackwardPropagate(mScratch, aParent, aChild);
          } else {
            result = BackwardPropagate(mScratch, eHTMLTag_html, aChild);
          }
        }
      }
    }
    if (mScratch.Length() - 1 > gHTMLElements[aParent].mPropagateRange) {
      result = false;
    }
  } else {
    result = !!aParentContains;
  }


  return result;
}











bool
CNavDTD::CanOmit(eHTMLTags aParent, eHTMLTags aChild, PRInt32& aParentContains)
{
  eHTMLTags theAncestor = gHTMLElements[aChild].mExcludingAncestor;
  if (eHTMLTag_unknown != theAncestor && HasOpenContainer(theAncestor)) {
    return true;
  }

  theAncestor = gHTMLElements[aChild].mRequiredAncestor;
  if (eHTMLTag_unknown != theAncestor) {
    
    
    return !HasOpenContainer(theAncestor) &&
           !CanPropagate(aParent, aChild, aParentContains);
  }

  if (gHTMLElements[aParent].CanExclude(aChild)) {
    return true;
  }

  
  if (-1 == aParentContains) {
    aParentContains = CanContain(aParent, aChild);
  }

  if (aParentContains || aChild == aParent) {
    return false;
  }

  if (gHTMLElements[aParent].IsBlockEntity() &&
      nsHTMLElement::IsInlineEntity(aChild)) {
    
    return true;
  }

  if (gHTMLElements[aParent].HasSpecialProperty(kBadContentWatch)) {
    
    
    return !gHTMLElements[aChild].HasSpecialProperty(kBadContentWatch);
  }

  if (gHTMLElements[aParent].HasSpecialProperty(kSaveMisplaced)) {
    return true;
  }

  if (aParent == eHTMLTag_body) {
    
    
    return true;
  }

  return false;
}









NS_IMETHODIMP_(bool)
CNavDTD::IsContainer(PRInt32 aTag) const
{
  return nsHTMLElement::IsContainer((eHTMLTags)aTag);
}


bool
CNavDTD::ForwardPropagate(nsString& aSequence, eHTMLTags aParent,
                          eHTMLTags aChild)
{
  bool result = false;

  switch(aParent) {
    case eHTMLTag_table:
      if (eHTMLTag_tr == aChild || eHTMLTag_td == aChild) {
        return BackwardPropagate(aSequence, aParent, aChild);
      }
      

    case eHTMLTag_tr:
      if (CanContain(eHTMLTag_td, aChild)) {
        aSequence.Append((PRUnichar)eHTMLTag_td);
        result = BackwardPropagate(aSequence, aParent, eHTMLTag_td);
      }
      break;

    default:
      break;
  }

  return result;
}

bool
CNavDTD::BackwardPropagate(nsString& aSequence, eHTMLTags aParent,
                           eHTMLTags aChild) const
{
  eHTMLTags theParent = aParent;

  do {
    const TagList* theRootTags = gHTMLElements[aChild].GetRootTags();
    if (!theRootTags) {
      break;
    }

    theParent = theRootTags->mTags[0];
    NS_ASSERTION(CanContain(theParent, aChild),
                 "Children must be contained by their root tags");

    aChild = theParent;
    aSequence.Append((PRUnichar)theParent);
  } while (theParent != eHTMLTag_unknown && theParent != aParent);

  return aParent == theParent;
}

bool CNavDTD::HasOpenContainer(eHTMLTags aContainer) const
{
  switch (aContainer) {
    case eHTMLTag_form:
      return !(~mFlags & NS_DTD_FLAG_HAS_OPEN_FORM);
    case eHTMLTag_map:
      return mOpenMapCount > 0;
    default:
      return mBodyContext->HasOpenContainer(aContainer);
  }
}

bool
CNavDTD::HasOpenContainer(const eHTMLTags aTagSet[], PRInt32 aCount) const
{
  int theIndex;
  int theTopIndex = mBodyContext->GetCount() - 1;

  for (theIndex = theTopIndex; theIndex > 0; --theIndex) {
    if (FindTagInSet((*mBodyContext)[theIndex], aTagSet, aCount)) {
      return true;
    }
  }

  return false;
}

eHTMLTags
CNavDTD::GetTopNode() const
{
  return mBodyContext->Last();
}












nsresult
CNavDTD::OpenTransientStyles(eHTMLTags aChildTag, bool aCloseInvalid)
{
  nsresult result = NS_OK;

  
  if ((mFlags & NS_DTD_FLAG_ENABLE_RESIDUAL_STYLE) &&
      eHTMLTag_newline != aChildTag &&
      !(mFlags & NS_DTD_FLAG_HAS_OPEN_HEAD)) {
    if (CanContain(eHTMLTag_font, aChildTag)) {
      PRUint32 theCount = mBodyContext->GetCount();
      PRUint32 theLevel = theCount;

      
      
      while (1 < theLevel) {
        eHTMLTags theParentTag = mBodyContext->TagAt(--theLevel);
        if (gHTMLElements[theParentTag].HasSpecialProperty(kNoStyleLeaksIn)) {
          break;
        }
      }

      mFlags &= ~NS_DTD_FLAG_ENABLE_RESIDUAL_STYLE;
      for (; theLevel < theCount; ++theLevel) {
        nsEntryStack* theStack = mBodyContext->GetStylesAt(theLevel);
        if (theStack) {
          
          if (theCount + theStack->mCount >= FONTSTYLE_IGNORE_DEPTH) {
            break;
          }

          PRInt32 sindex = 0;

          nsTagEntry *theEntry = theStack->mEntries;
          bool isHeadingOpen = HasOpenTagOfType(kHeading, *mBodyContext);
          for (sindex = 0; sindex < theStack->mCount; ++sindex) {
            nsCParserNode* theNode = (nsCParserNode*)theEntry->mNode;
            if (1 == theNode->mUseCount) {
              eHTMLTags theNodeTag = (eHTMLTags)theNode->GetNodeType();
              if (gHTMLElements[theNodeTag].CanContain(aChildTag, mDTDMode)) {
                
                
                
                theEntry->mParent = theStack;
                if (isHeadingOpen) {
                  
                  
                  
                  
                  
                  
                  CAttributeToken theAttrToken(NS_LITERAL_STRING("_moz-rs-heading"),
                                               EmptyString());
                  theNode->AddAttribute(&theAttrToken);
                  result = OpenContainer(theNode, theNodeTag, theStack);
                  theNode->PopAttributeToken();
                } else {
                  result = OpenContainer(theNode, theNodeTag, theStack);
                }
              } else if (aCloseInvalid) {
                
                
                nsCParserNode* node = theStack->Remove(sindex, theNodeTag);
                IF_FREE(node, &mNodeAllocator);
                --theEntry;
              }
            }
            ++theEntry;
          }
        }
      }
      mFlags |= NS_DTD_FLAG_ENABLE_RESIDUAL_STYLE;
    }
  }

  return result;
}









void
CNavDTD::PopStyle(eHTMLTags aTag)
{
  if ((mFlags & NS_DTD_FLAG_ENABLE_RESIDUAL_STYLE) &&
      nsHTMLElement::IsResidualStyleTag(aTag)) {
    nsCParserNode* node = mBodyContext->PopStyle(aTag);
    IF_FREE(node, &mNodeAllocator);  
  }
}










nsresult
CNavDTD::OpenHTML(const nsCParserNode *aNode)
{
  NS_PRECONDITION(mBodyContext->GetCount() >= 0, kInvalidTagStackPos);

  nsresult result = mSink ? mSink->OpenContainer(*aNode) : NS_OK; 

  
  if (mBodyContext->GetCount() == 0)  {
    mBodyContext->Push(const_cast<nsCParserNode*>(aNode), 0, false); 
  }

  return result;
}









nsresult
CNavDTD::OpenBody(const nsCParserNode *aNode)
{
  NS_PRECONDITION(mBodyContext->GetCount() >= 0, kInvalidTagStackPos);

  nsresult result = NS_OK;
  
  if (!(mFlags & NS_DTD_FLAG_HAD_FRAMESET)) {
    mFlags |= NS_DTD_FLAG_HAD_BODY;

    
    CloseContainer(eHTMLTag_head, false);

    
    result = mSink ? mSink->OpenContainer(*aNode) : NS_OK; 

    if (!HasOpenContainer(eHTMLTag_body)) {
      mBodyContext->Push(const_cast<nsCParserNode*>(aNode), 0, false);
      mTokenizer->PrependTokens(mMisplacedContent);
    }
  }

  return result;
}










nsresult
CNavDTD::OpenContainer(const nsCParserNode *aNode,
                       eHTMLTags aTag,
                       nsEntryStack* aStyleStack)
{
  NS_PRECONDITION(mBodyContext->GetCount() >= 0, kInvalidTagStackPos);

  nsresult   result = NS_OK;
  bool       done   = true;
  bool       rs_tag = nsHTMLElement::IsResidualStyleTag(aTag);
  
  
  bool       li_tag = aTag == eHTMLTag_li;

  if (rs_tag || li_tag) {
    







    OpenTransientStyles(aTag, !li_tag);
  }

  switch (aTag) {
    case eHTMLTag_html:
      result = OpenHTML(aNode);
      break;

    case eHTMLTag_head:
      if (!(mFlags & NS_DTD_FLAG_HAS_OPEN_HEAD)) {
        mFlags |= NS_DTD_FLAG_HAS_OPEN_HEAD;
        done = false;
      }
      break;

    case eHTMLTag_body:
      {
        eHTMLTags theParent = mBodyContext->Last();
        if (!gHTMLElements[aTag].IsSpecialParent(theParent)) {
          mFlags |= NS_DTD_FLAG_HAS_OPEN_BODY;
          result = OpenBody(aNode);
        } else {
          done = false;
        }
      }
      break;

    case eHTMLTag_map:
      ++mOpenMapCount;
      done = false;
      break;

    case eHTMLTag_form:
      
      if (!(mFlags & NS_DTD_FLAG_HAS_OPEN_FORM)) {
        mFlags |= NS_DTD_FLAG_HAS_OPEN_FORM;
        result = mSink ? mSink->OpenContainer(*aNode) : NS_OK;
      }
      break;

    case eHTMLTag_frameset:
      
      CloseContainer(eHTMLTag_head, false);

      
      mFlags |= NS_DTD_FLAG_HAD_FRAMESET;
      done = false;
      break;

    case eHTMLTag_noembed:
      
      done = false;
      mFlags |= NS_DTD_FLAG_ALTERNATE_CONTENT;
      break;

    case eHTMLTag_noscript:
      
      
      done = false;

      if (mFlags & NS_IPARSER_FLAG_SCRIPT_ENABLED) {
        
        
        mFlags |= NS_DTD_FLAG_ALTERNATE_CONTENT;
      }
      break;

    case eHTMLTag_iframe: 
    case eHTMLTag_noframes:
      done = false;
      if (mFlags & NS_IPARSER_FLAG_FRAMES_ENABLED) {
        mFlags |= NS_DTD_FLAG_ALTERNATE_CONTENT;
      }
      break;

    default:
      done = false;
      break;
  }

  if (!done) {

    result = mSink ? mSink->OpenContainer(*aNode) : NS_OK;

    
    
    mBodyContext->Push(const_cast<nsCParserNode*>(aNode), aStyleStack, rs_tag); 
  }

  return result;
}

nsresult
CNavDTD::CloseResidualStyleTags(const eHTMLTags aTag,
                                bool aClosedByStartTag)
{
  const PRInt32 count = mBodyContext->GetCount();
  PRInt32 pos = count;
  while (nsHTMLElement::IsResidualStyleTag(mBodyContext->TagAt(pos - 1)))
    --pos;
  if (pos < count)
    return CloseContainersTo(pos, aTag, aClosedByStartTag);
  return NS_OK;
}









nsresult
CNavDTD::CloseContainer(const eHTMLTags aTag, bool aMalformed)
{
  nsresult   result = NS_OK;
  bool       done   = true;

  switch (aTag) {
    case eHTMLTag_head:
      if (mFlags & NS_DTD_FLAG_HAS_OPEN_HEAD) {
        mFlags &= ~NS_DTD_FLAG_HAS_OPEN_HEAD;
        if (mBodyContext->Last() == eHTMLTag_head) {
          mBodyContext->Pop();
        } else {
          
          
          
          NS_ASSERTION(mBodyContext->LastOf(eHTMLTag_head) == kNotFound,
                       "Closing the wrong tag");
        }
        done = false;
      }
      break;

    case eHTMLTag_map:
      if (mOpenMapCount) {
        mOpenMapCount--;
        done = false;
      }
      break;

    case eHTMLTag_form:
      if (mFlags & NS_DTD_FLAG_HAS_OPEN_FORM) {
        mFlags &= ~NS_DTD_FLAG_HAS_OPEN_FORM;
        done = false;
        
        
        
        CloseResidualStyleTags(eHTMLTag_form, false);
      }
      break;

    case eHTMLTag_iframe:
    case eHTMLTag_noembed:
    case eHTMLTag_noscript:
    case eHTMLTag_noframes:
      
      mFlags &= ~NS_DTD_FLAG_ALTERNATE_CONTENT;

      
    default:
      done = false;
  }

  if (!done) {

    if (mSink) {
      result = !aMalformed
               ? mSink->CloseContainer(aTag)
               : mSink->CloseMalformedContainer(aTag);
    }

    
    
    
    if (mBodyContext->GetCount() == mHeadContainerPosition) {
      mHeadContainerPosition = -1;
      nsresult headresult = CloseContainer(eHTMLTag_head, false);

      
      
      
      if (NS_SUCCEEDED(result)) {
        result = headresult;
      }
    }
  }

  return result;
}











nsresult
CNavDTD::CloseContainersTo(PRInt32 anIndex, eHTMLTags aTarget,
                           bool aClosedByStartTag)
{
  NS_PRECONDITION(mBodyContext->GetCount() > 0, kInvalidTagStackPos);
  nsresult result = NS_OK;

  if (anIndex < mBodyContext->GetCount() && anIndex >= 0) {
    PRInt32 count = 0;
    while ((count = mBodyContext->GetCount()) > anIndex) {
      nsEntryStack* theChildStyleStack = 0;
      eHTMLTags theTag = mBodyContext->Last();
      nsCParserNode* theNode = mBodyContext->Pop(theChildStyleStack);
      result = CloseContainer(theTag, false);

      bool theTagIsStyle = nsHTMLElement::IsResidualStyleTag(theTag);
      
      bool theStyleDoesntLeakOut = gHTMLElements[theTag].HasSpecialProperty(kNoStyleLeaksOut);
      if (!theStyleDoesntLeakOut) {
        theStyleDoesntLeakOut = gHTMLElements[aTarget].HasSpecialProperty(kNoStyleLeaksOut);
      }

      
      
      if (theTagIsStyle && !(mFlags & NS_DTD_FLAG_ALTERNATE_CONTENT)) {
        NS_ASSERTION(theNode, "residual style node should not be null");
        if (!theNode) {
          if (theChildStyleStack) {
            mBodyContext->PushStyles(theChildStyleStack);
          }
          return NS_OK;
        }

        bool theTargetTagIsStyle = nsHTMLElement::IsResidualStyleTag(aTarget);
        if (aClosedByStartTag) {
          
          
          
          
          if (theNode->mUseCount == 0) {
            if (theTag != aTarget) {
              if (theChildStyleStack) {
                theChildStyleStack->PushFront(theNode);
              } else {
                mBodyContext->PushStyle(theNode);
              }
            }
          } else if (theTag == aTarget && !gHTMLElements[aTarget].CanContainSelf()) {
            
            
            
            nsCParserNode* node = mBodyContext->PopStyle(theTag);
            IF_FREE(node, &mNodeAllocator);
          }

          if (theChildStyleStack) {
            mBodyContext->PushStyles(theChildStyleStack);
          }
        } else {
          






















          if (theChildStyleStack) {
            if (!theStyleDoesntLeakOut) {
              if (theTag != aTarget) {
                if (theNode->mUseCount == 0) {
                  theChildStyleStack->PushFront(theNode);
                }
              } else if (theNode->mUseCount == 1) {
                
                
                
                
                
                
                
                mBodyContext->RemoveStyle(theTag);
              }
              mBodyContext->PushStyles(theChildStyleStack);
            } else{
              IF_DELETE(theChildStyleStack, &mNodeAllocator);
            }
          } else if (theNode->mUseCount == 0) {
            
            
            
            if (aTarget != theTag) {
              mBodyContext->PushStyle(theNode);
            }
          } else {
            
            
            
            
            
            
            
            
            if (theTargetTagIsStyle && theTag == aTarget) {
              mBodyContext->RemoveStyle(theTag);
            }
          }
        }
      } else {
        
        if (theChildStyleStack) {
          if (theStyleDoesntLeakOut) {
            IF_DELETE(theChildStyleStack, &mNodeAllocator);
          } else {
            mBodyContext->PushStyles(theChildStyleStack);
          }
        }
      }
      IF_FREE(theNode, &mNodeAllocator);
    }
  }
  return result;
}










nsresult
CNavDTD::CloseContainersTo(eHTMLTags aTag, bool aClosedByStartTag)
{
  NS_PRECONDITION(mBodyContext->GetCount() > 0, kInvalidTagStackPos);

  PRInt32 pos = mBodyContext->LastOf(aTag);

  if (kNotFound != pos) {
    
    return CloseContainersTo(pos, aTag, aClosedByStartTag);
  }

  eHTMLTags theTopTag = mBodyContext->Last();

  bool theTagIsSynonymous = (nsHTMLElement::IsResidualStyleTag(aTag) &&
                               nsHTMLElement::IsResidualStyleTag(theTopTag)) ||
                              (gHTMLElements[aTag].IsMemberOf(kHeading) &&
                               gHTMLElements[theTopTag].IsMemberOf(kHeading));

  if (theTagIsSynonymous) {
    
    
    
    aTag = theTopTag;
    pos = mBodyContext->LastOf(aTag);
    if (kNotFound != pos) {
      
      return CloseContainersTo(pos, aTag, aClosedByStartTag);
    }
  }

  nsresult result = NS_OK;
  const TagList* theRootTags = gHTMLElements[aTag].GetRootTags();
  
  eHTMLTags theParentTag = theRootTags ? theRootTags->mTags[0] : eHTMLTag_unknown;
  pos = mBodyContext->LastOf(theParentTag);
  if (kNotFound != pos) {
    
    result = CloseContainersTo(pos + 1, aTag, aClosedByStartTag);
  }
  return result;
}









nsresult
CNavDTD::AddLeaf(const nsIParserNode *aNode)
{
  nsresult result = NS_OK;

  if (mSink) {
    eHTMLTags theTag = (eHTMLTags)aNode->GetNodeType();
    OpenTransientStyles(theTag);

    result = mSink->AddLeaf(*aNode);
  }

  return result;
}









nsresult
CNavDTD::AddHeadContent(nsIParserNode *aNode)
{
  nsresult result = NS_OK;

  static eHTMLTags gNoXTags[] = { eHTMLTag_noembed, eHTMLTag_noframes };

  eHTMLTags theTag = (eHTMLTags)aNode->GetNodeType();

  
  
  
  if (eHTMLTag_meta == theTag || eHTMLTag_script == theTag) {
    if (HasOpenContainer(gNoXTags, ArrayLength(gNoXTags))) {
      return result;
    }
  }

  if (mSink) {
    
    if (!(mFlags & NS_DTD_FLAG_HAS_OPEN_HEAD)) {
      result = mSink->OpenHead();
      mBodyContext->PushTag(eHTMLTag_head);
      mFlags |= NS_DTD_FLAG_HAS_OPEN_HEAD;
    }

    
    if (!nsHTMLElement::IsContainer(theTag) || theTag == eHTMLTag_userdefined) {
      result = mSink->AddLeaf(*aNode);

      if (mFlags & NS_DTD_FLAG_HAS_MAIN_CONTAINER) {
        
        CloseContainer(eHTMLTag_head, false);
      }
    } else {
      if ((mFlags & NS_DTD_FLAG_HAS_MAIN_CONTAINER) &&
          mHeadContainerPosition == -1) {
        
        
        mHeadContainerPosition = mBodyContext->GetCount();
      }

      
      result = mSink->OpenContainer(*aNode);

      mBodyContext->Push(static_cast<nsCParserNode*>(aNode), nullptr,
                         false);
    }
  }

  return result;
}

void
CNavDTD::CreateContextStackFor(eHTMLTags aParent, eHTMLTags aChild)
{
  mScratch.Truncate();

  bool      result = ForwardPropagate(mScratch, aParent, aChild);

  if (!result) {
    if (eHTMLTag_unknown == aParent) {
      result = BackwardPropagate(mScratch, eHTMLTag_html, aChild);
    } else if (aParent != aChild) {
      
      result = BackwardPropagate(mScratch, aParent, aChild);
    }
  }

  if (!result) {
    return;
  }

  PRInt32   theLen = mScratch.Length();
  eHTMLTags theTag = (eHTMLTags)mScratch[--theLen];

  
  while (theLen) {
    theTag = (eHTMLTags)mScratch[--theLen];

    
    
    CToken *theToken = mTokenAllocator->CreateTokenOfType(eToken_start, theTag);
    HandleToken(theToken);
  }
}

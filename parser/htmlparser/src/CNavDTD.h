






























































#ifndef NS_NAVHTMLDTD__
#define NS_NAVHTMLDTD__

#include "nsIDTD.h"
#include "nsISupports.h"
#include "nsHTMLTags.h"
#include "nsDeque.h"
#include "nsParserCIID.h"
#include "nsDTDUtils.h"
#include "nsParser.h"
#include "nsCycleCollectionParticipant.h"

class nsIHTMLContentSink;
class nsIParserNode;
class nsDTDContext;
class nsEntryStack;
class nsITokenizer;
class nsCParserNode;
class nsTokenAllocator;









#ifdef _MSC_VER
#pragma warning( disable : 4275 )
#endif

class CNavDTD : public nsIDTD
{
#ifdef _MSC_VER
#pragma warning( default : 4275 )
#endif

public:
    



    CNavDTD();
    virtual ~CNavDTD();

    












    nsresult OpenContainer(const nsCParserNode *aNode,
                           eHTMLTags aTag,
                           nsEntryStack* aStyleStack = nsnull);

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_NSIDTD
    NS_DECL_CYCLE_COLLECTION_CLASS(CNavDTD)

private:
    







    bool CanPropagate(eHTMLTags aParent,
                        eHTMLTags aChild,
                        PRInt32 aParentContains);

    









    bool CanOmit(eHTMLTags aParent, 
                   eHTMLTags aChild,
                   PRInt32& aParentContains);

    








    bool ForwardPropagate(nsString& aSequence,
                            eHTMLTags aParent,
                            eHTMLTags aChild);

    









    bool BackwardPropagate(nsString& aSequence,
                             eHTMLTags aParent,
                             eHTMLTags aChild) const;

    






    void CreateContextStackFor(eHTMLTags aParent, eHTMLTags aChild);

    





    bool HasOpenContainer(eHTMLTags aContainer) const;

    






    bool HasOpenContainer(const eHTMLTags aTagSet[], PRInt32 aCount) const;

    





    eHTMLTags GetTopNode() const;

    





    PRInt32 LastOf(eHTMLTags aTagSet[], PRInt32 aCount) const;

    nsresult HandleToken(CToken* aToken);

    











    nsresult    HandleStartToken(CToken* aToken);

    













    nsresult    HandleDefaultStartToken(CToken* aToken, eHTMLTags aChildTag,
                                        nsCParserNode *aNode);
    nsresult    HandleEndToken(CToken* aToken);
    nsresult    HandleEntityToken(CToken* aToken);
    nsresult    HandleCommentToken(CToken* aToken);
    nsresult    HandleAttributeToken(CToken* aToken);
    nsresult    HandleProcessingInstructionToken(CToken* aToken);
    nsresult    HandleDocTypeDeclToken(CToken* aToken);
    nsresult    BuildNeglectedTarget(eHTMLTags aTarget, eHTMLTokenTypes aType);

    nsresult OpenHTML(const nsCParserNode *aNode);
    nsresult OpenBody(const nsCParserNode *aNode);

    




    nsresult CloseContainer(const eHTMLTags aTag, bool aMalformed);
    nsresult CloseContainersTo(eHTMLTags aTag, bool aClosedByStartTag);
    nsresult CloseContainersTo(PRInt32 anIndex, eHTMLTags aTag,
                               bool aClosedByStartTag);
    nsresult CloseResidualStyleTags(const eHTMLTags aTag,
                                    bool aClosedByStartTag);

    




    nsresult AddLeaf(const nsIParserNode *aNode);
    nsresult AddHeadContent(nsIParserNode *aNode);

    







    nsresult  OpenTransientStyles(eHTMLTags aChildTag,
                                  bool aCloseInvalid = true);
    void      PopStyle(eHTMLTags aTag);

    nsresult  PushIntoMisplacedStack(CToken* aToken)
    {
      NS_ENSURE_ARG_POINTER(aToken);
      aToken->SetNewlineCount(0); 

      mMisplacedContent.Push(aToken);
      return NS_OK;
    }

protected:

    nsresult        CollectAttributes(nsIParserNode* aNode,eHTMLTags aTag,PRInt32 aCount);

    








    nsresult        WillHandleStartTag(CToken* aToken,eHTMLTags aChildTag,nsIParserNode& aNode);
    nsresult        DidHandleStartTag(nsIParserNode& aNode,eHTMLTags aChildTag);

    









    void            HandleOmittedTag(CToken* aToken, eHTMLTags aChildTag,
                                     eHTMLTags aParent, nsIParserNode *aNode);
    nsresult        HandleSavedTokens(PRInt32 anIndex);
    nsresult        HandleKeyGen(nsIParserNode *aNode);
    bool            IsAlternateTag(eHTMLTags aTag);
    bool            IsBlockElement(PRInt32 aTagID, PRInt32 aParentID) const;
    bool            IsInlineElement(PRInt32 aTagID, PRInt32 aParentID) const;

    nsDeque             mMisplacedContent;
    
    nsCOMPtr<nsIHTMLContentSink> mSink;
    nsTokenAllocator*   mTokenAllocator;
    nsDTDContext*       mBodyContext;
    nsDTDContext*       mTempContext;
    bool                mCountLines;
    nsITokenizer*       mTokenizer; 
   
    nsString            mFilename; 
    nsString            mScratch;  
    nsCString           mMimeType;

    nsNodeAllocator     mNodeAllocator;
    nsDTDMode           mDTDMode;
    eParserDocType      mDocType;
    eParserCommands     mParserCommand;   

    PRInt32             mLineNumber;
    PRInt32             mOpenMapCount;
    PRInt32             mHeadContainerPosition;

    PRUint16            mFlags;
};

#endif 




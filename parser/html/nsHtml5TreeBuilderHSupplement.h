




































#define NS_HTML5_TREE_BUILDER_HANDLE_ARRAY_LENGTH 512

  private:
    nsHtml5Highlighter*                    mViewSource;
    nsTArray<nsHtml5TreeOperation>         mOpQueue;
    nsTArray<nsHtml5SpeculativeLoad>       mSpeculativeLoadQueue;
    nsAHtml5TreeOpSink*                    mOpSink;
    nsAutoArrayPtr<nsIContent*>            mHandles;
    PRInt32                                mHandlesUsed;
    nsTArray<nsAutoArrayPtr<nsIContent*> > mOldHandles;
    nsHtml5TreeOpStage*                    mSpeculativeLoadStage;
    nsIContent**                           mDeepTreeSurrogateParent;
    bool                                   mCurrentHtmlScriptIsAsyncOrDefer;
#ifdef DEBUG
    bool                                   mActive;
#endif

    
    


    void documentMode(nsHtml5DocumentMode m);

    






















    nsIContent** AllocateContentHandle();
    
    void accumulateCharactersForced(const PRUnichar* aBuf, PRInt32 aStart, PRInt32 aLength)
    {
      accumulateCharacters(aBuf, aStart, aLength);
    }

  public:

    nsHtml5TreeBuilder(nsAHtml5TreeOpSink* aOpSink,
                       nsHtml5TreeOpStage* aStage);

    ~nsHtml5TreeBuilder();
    
    void StartPlainTextViewSource(const nsAutoString& aTitle);

    void StartPlainText();

    bool HasScript();
    
    void SetOpSink(nsAHtml5TreeOpSink* aOpSink) {
      mOpSink = aOpSink;
    }

    void ClearOps() {
      mOpQueue.Clear();
    }
    
    bool Flush(bool aDiscretionary = false);
    
    void FlushLoads();

    void SetDocumentCharset(nsACString& aCharset, PRInt32 aCharsetSource);

    void StreamEnded();

    void NeedsCharsetSwitchTo(const nsACString& aEncoding, PRInt32 aSource);

    void AddSnapshotToScript(nsAHtml5TreeBuilderState* aSnapshot, PRInt32 aLine);

    void DropHandles();

    void EnableViewSource(nsHtml5Highlighter* aHighlighter);

    void errStrayStartTag(nsIAtom* aName);

    void errStrayEndTag(nsIAtom* aName);

    void errUnclosedElements(PRInt32 aIndex, nsIAtom* aName);

    void errUnclosedElementsImplied(PRInt32 aIndex, nsIAtom* aName);

    void errUnclosedElementsCell(PRInt32 aIndex);

    void errStrayDoctype();

    void errAlmostStandardsDoctype();

    void errQuirkyDoctype();

    void errNonSpaceInTrailer();

    void errNonSpaceAfterFrameset();

    void errNonSpaceInFrameset();

    void errNonSpaceAfterBody();

    void errNonSpaceInColgroupInFragment();

    void errNonSpaceInNoscriptInHead();

    void errFooBetweenHeadAndBody(nsIAtom* aName);

    void errStartTagWithoutDoctype();

    void errNoSelectInTableScope();

    void errStartSelectWhereEndSelectExpected();

    void errStartTagWithSelectOpen(nsIAtom* aName);

    void errBadStartTagInHead(nsIAtom* aName);

    void errImage();

    void errIsindex();

    void errFooSeenWhenFooOpen(nsIAtom* aName);

    void errHeadingWhenHeadingOpen();

    void errFramesetStart();

    void errNoCellToClose();

    void errStartTagInTable(nsIAtom* aName);

    void errFormWhenFormOpen();

    void errTableSeenWhileTableOpen();

    void errStartTagInTableBody(nsIAtom* aName);

    void errEndTagSeenWithoutDoctype();

    void errEndTagAfterBody();

    void errEndTagSeenWithSelectOpen(nsIAtom* aName);

    void errGarbageInColgroup();

    void errEndTagBr();

    void errNoElementToCloseButEndTagSeen(nsIAtom* aName);

    void errHtmlStartTagInForeignContext(nsIAtom* aName);

    void errTableClosedWhileCaptionOpen();

    void errNoTableRowToClose();

    void errNonSpaceInTable();

    void errUnclosedChildrenInRuby();

    void errStartTagSeenWithoutRuby(nsIAtom* aName);

    void errSelfClosing();

    void errNoCheckUnclosedElementsOnStack();

    void errEndTagDidNotMatchCurrentOpenElement(nsIAtom* aName, nsIAtom* aOther);

    void errEndTagViolatesNestingRules(nsIAtom* aName);

    void errEndWithUnclosedElements(nsIAtom* aName);

    void MarkAsBroken();

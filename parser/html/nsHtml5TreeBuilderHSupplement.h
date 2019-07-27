



#define NS_HTML5_TREE_BUILDER_HANDLE_ARRAY_LENGTH 512

  private:
    nsHtml5OplessBuilder*                  mBuilder;
    
    
    
    
    nsHtml5Highlighter*                    mViewSource;
    nsTArray<nsHtml5TreeOperation>         mOpQueue;
    nsTArray<nsHtml5SpeculativeLoad>       mSpeculativeLoadQueue;
    nsAHtml5TreeOpSink*                    mOpSink;
    nsAutoArrayPtr<nsIContent*>            mHandles;
    int32_t                                mHandlesUsed;
    nsTArray<nsAutoArrayPtr<nsIContent*> > mOldHandles;
    nsHtml5TreeOpStage*                    mSpeculativeLoadStage;
    bool                                   mCurrentHtmlScriptIsAsyncOrDefer;
    bool                                   mPreventScriptExecution;
#ifdef DEBUG
    bool                                   mActive;
#endif

    
    


    void documentMode(nsHtml5DocumentMode m);

    nsIContentHandle* getDocumentFragmentForTemplate(nsIContentHandle* aTemplate);

    nsIContentHandle* getFormPointerForContext(nsIContentHandle* aContext);

    






















    nsIContentHandle* AllocateContentHandle();
    
    void accumulateCharactersForced(const char16_t* aBuf, int32_t aStart, int32_t aLength)
    {
      accumulateCharacters(aBuf, aStart, aLength);
    }

    void MarkAsBrokenAndRequestSuspension(nsresult aRv)
    {
      mBuilder->MarkAsBroken(aRv);
      requestSuspension();
    }

  public:

    explicit nsHtml5TreeBuilder(nsHtml5OplessBuilder* aBuilder);

    nsHtml5TreeBuilder(nsAHtml5TreeOpSink* aOpSink,
                       nsHtml5TreeOpStage* aStage);

    ~nsHtml5TreeBuilder();
    
    void StartPlainTextViewSource(const nsAutoString& aTitle);

    void StartPlainText();

    void StartPlainTextBody();

    bool HasScript();
    
    void SetOpSink(nsAHtml5TreeOpSink* aOpSink)
    {
      mOpSink = aOpSink;
    }

    void ClearOps()
    {
      mOpQueue.Clear();
    }
    
    bool Flush(bool aDiscretionary = false);
    
    void FlushLoads();

    void SetDocumentCharset(nsACString& aCharset, int32_t aCharsetSource);

    void StreamEnded();

    void NeedsCharsetSwitchTo(const nsACString& aEncoding,
                              int32_t aSource,
                              int32_t aLineNumber);

    void MaybeComplainAboutCharset(const char* aMsgId,
                                   bool aError,
                                   int32_t aLineNumber);

    void AddSnapshotToScript(nsAHtml5TreeBuilderState* aSnapshot, int32_t aLine);

    void DropHandles();

    void SetPreventScriptExecution(bool aPrevent)
    {
      mPreventScriptExecution = aPrevent;
    }

    bool HasBuilder()
    {
      return mBuilder;
    }

    void EnableViewSource(nsHtml5Highlighter* aHighlighter);

    void errStrayStartTag(nsIAtom* aName);

    void errStrayEndTag(nsIAtom* aName);

    void errUnclosedElements(int32_t aIndex, nsIAtom* aName);

    void errUnclosedElementsImplied(int32_t aIndex, nsIAtom* aName);

    void errUnclosedElementsCell(int32_t aIndex);

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

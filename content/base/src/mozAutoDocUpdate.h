










































class mozAutoDocUpdate
{
public:
  mozAutoDocUpdate(nsIDocument* aDocument, nsUpdateType aUpdateType,
                   PRBool aNotify) :
    mDocument(aNotify ? aDocument : nsnull),
    mUpdateType(aUpdateType)
  {
    if (mDocument) {
      mDocument->BeginUpdate(mUpdateType);
    }
    else if (aUpdateType == UPDATE_CONTENT_MODEL) {
      nsContentUtils::AddRemovableScriptBlocker();
    }
    else {
      nsContentUtils::AddScriptBlocker();
    }
  }

  ~mozAutoDocUpdate()
  {
    if (mDocument) {
      mDocument->EndUpdate(mUpdateType);
    }
    else if (mUpdateType == UPDATE_CONTENT_MODEL) {
      nsContentUtils::RemoveRemovableScriptBlocker();
    }
    else {
      nsContentUtils::RemoveScriptBlocker();
    }
  }

private:
  nsCOMPtr<nsIDocument> mDocument;
  nsUpdateType mUpdateType;
};

#define MOZ_AUTO_DOC_UPDATE_PASTE2(tok,line) tok##line
#define MOZ_AUTO_DOC_UPDATE_PASTE(tok,line) \
  MOZ_AUTO_DOC_UPDATE_PASTE2(tok,line)
#define MOZ_AUTO_DOC_UPDATE(doc,type,notify) \
  mozAutoDocUpdate MOZ_AUTO_DOC_UPDATE_PASTE(_autoDocUpdater_, __LINE__) \
  (doc,type,notify)










class mozAutoDocConditionalContentUpdateBatch
{
public:
  mozAutoDocConditionalContentUpdateBatch(nsIDocument* aDocument,
                                          PRBool aNotify) :
    mDocument(aNotify ? aDocument : nsnull)
  {
    if (mDocument) {
      mDocument->BeginUpdate(UPDATE_CONTENT_MODEL);
    }
  }

  ~mozAutoDocConditionalContentUpdateBatch()
  {
    if (mDocument) {
      mDocument->EndUpdate(UPDATE_CONTENT_MODEL);
    }
  }

private:
  nsCOMPtr<nsIDocument> mDocument;
};





































#include "nsContentUtils.h"
#include "nsIDocument.h"
#include "nsIDocumentObserver.h"








class NS_STACK_CLASS mozAutoDocUpdate
{
public:
  mozAutoDocUpdate(nsIDocument* aDocument, nsUpdateType aUpdateType,
                   bool aNotify) :
    mDocument(aNotify ? aDocument : nsnull),
    mUpdateType(aUpdateType)
  {
    if (mDocument) {
      mDocument->BeginUpdate(mUpdateType);
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










class NS_STACK_CLASS mozAutoDocConditionalContentUpdateBatch
{
public:
  mozAutoDocConditionalContentUpdateBatch(nsIDocument* aDocument,
                                          bool aNotify) :
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

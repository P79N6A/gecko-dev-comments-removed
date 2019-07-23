



































 
#ifndef nsHtml5TreeOperation_h__
#define nsHtml5TreeOperation_h__

#include "nsIContent.h"
#include "nsHtml5DocumentMode.h"

class nsHtml5TreeOpExecutor;

enum eHtml5TreeOperation {
  
  eTreeOpAppend,
  eTreeOpDetach,
  eTreeOpAppendChildrenToNewParent,
  eTreeOpFosterParent,
  eTreeOpAppendToDocument,
  eTreeOpAddAttributes,
  eTreeOpDocumentMode,
  
  eTreeOpDoneAddingChildren,
  eTreeOpDoneCreatingElement,
  eTreeOpUpdateStyleSheet,
  eTreeOpProcessBase,
  eTreeOpProcessMeta,
  eTreeOpProcessOfflineManifest,
  eTreeOpStartLayout
};

class nsHtml5TreeOperation {

  public:
    nsHtml5TreeOperation();

    ~nsHtml5TreeOperation();

    inline void Init(nsIContent* aNode, nsIContent* aParent) {
      mNode = aNode;
      mParent = aParent;
    }

    inline void Init(eHtml5TreeOperation aOpCode, nsIContent* aNode) {
      mOpCode = aOpCode;
      mNode = aNode;
    }

    inline void Init(eHtml5TreeOperation aOpCode, 
                     nsIContent* aNode,
                     nsIContent* aParent) {
      mOpCode = aOpCode;
      mNode = aNode;
      mParent = aParent;
    }

    inline void Init(eHtml5TreeOperation aOpCode,
                     nsIContent* aNode,
                     nsIContent* aParent, 
                     nsIContent* aTable) {
      mOpCode = aOpCode;
      mNode = aNode;
      mParent = aParent;
      mTable = aTable;
    }

    inline void Init(nsHtml5DocumentMode aMode) {
      mOpCode = eTreeOpDocumentMode;
      mMode = aMode;
    }

    inline void DoTraverse(nsCycleCollectionTraversalCallback &cb) {
      nsHtml5TreeOperation* tmp = this;
      NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mNode);
      NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mParent);
      NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mTable);
    }

    nsresult Perform(nsHtml5TreeOpExecutor* aBuilder);

  private:
    eHtml5TreeOperation mOpCode;
    nsCOMPtr<nsIContent> mNode;
    nsCOMPtr<nsIContent> mParent;
    nsCOMPtr<nsIContent> mTable;
    nsHtml5DocumentMode  mMode; 
};

#endif 

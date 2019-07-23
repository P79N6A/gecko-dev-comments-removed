




































#ifndef nsHtml5TreeOperation_h__
#define nsHtml5TreeOperation_h__

#include "nsIContent.h"

class nsHtml5TreeBuilder;

enum eHtml5TreeOperation {
  
  eTreeOpAppend,
  eTreeOpDetach,
  eTreeOpAppendChildrenToNewParent,
  eTreeOpFosterParent,
  eTreeOpAppendToDocument,
  eTreeOpAddAttributes,
  
  eTreeOpScriptEnd,
  eTreeOpDoneAddingChildren,
  eTreeOpUpdateStyleSheet,
  eTreeOpProcessBase,
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
    inline void Init(eHtml5TreeOperation aOpCode, nsIContent* aNode, nsIContent* aParent) {
      mOpCode = aOpCode;
      mNode = aNode;
      mParent = aParent;
    }
    inline void Init(eHtml5TreeOperation aOpCode, nsIContent* aNode, nsIContent* aParent, nsIContent* aTable) {
      mOpCode = aOpCode;
      mNode = aNode;
      mParent = aParent;
      mTable = aTable;
    }
    nsresult Perform(nsHtml5TreeBuilder* aBuilder);
  private:
    eHtml5TreeOperation mOpCode;
    nsCOMPtr<nsIContent> mNode;
    nsCOMPtr<nsIContent> mParent;
    nsCOMPtr<nsIContent> mTable;    
};

#endif 
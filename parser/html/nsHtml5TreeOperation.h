



































 
#ifndef nsHtml5TreeOperation_h__
#define nsHtml5TreeOperation_h__

#include "nsIContent.h"
#include "nsHtml5DocumentMode.h"
#include "nsHtml5HtmlAttributes.h"

class nsHtml5TreeOpExecutor;
class nsHtml5StateSnapshot;

enum eHtml5TreeOperation {
#ifdef DEBUG
  eTreeOpUninitialized,
#endif
  
  eTreeOpAppend,
  eTreeOpDetach,
  eTreeOpAppendChildrenToNewParent,
  eTreeOpFosterParent,
  eTreeOpAppendToDocument,
  eTreeOpAddAttributes,
  eTreeOpDocumentMode,
  eTreeOpCreateElement,
  eTreeOpSetFormElement,
  eTreeOpCreateTextNode,
  eTreeOpCreateComment,
  eTreeOpCreateDoctype,
  
  eTreeOpRunScript,
  eTreeOpDoneAddingChildren,
  eTreeOpDoneCreatingElement,
  eTreeOpSetDocumentCharset,
  eTreeOpNeedsCharsetSwitchTo,
  eTreeOpUpdateStyleSheet,
  eTreeOpProcessBase,
  eTreeOpProcessMeta,
  eTreeOpProcessOfflineManifest,
  eTreeOpMarkMalformedIfScript,
  eTreeOpStreamEnded,
  eTreeOpSetStyleLineNumber,
  eTreeOpSetScriptLineNumber,
  eTreeOpStartLayout
};

class nsHtml5TreeOperationStringPair {
  private:
    nsString mPublicId;
    nsString mSystemId;
  public:
    nsHtml5TreeOperationStringPair(const nsAString& aPublicId, 
                                   const nsAString& aSystemId)
      : mPublicId(aPublicId)
      , mSystemId(aSystemId) {
      MOZ_COUNT_CTOR(nsHtml5TreeOperationStringPair);
    }
    
    ~nsHtml5TreeOperationStringPair() {
      MOZ_COUNT_DTOR(nsHtml5TreeOperationStringPair);    
    }
    
    inline void Get(nsAString& aPublicId, nsAString& aSystemId) {
      aPublicId.Assign(mPublicId);
      aSystemId.Assign(mSystemId);
    }
};

class nsHtml5TreeOperation {

  public:
    nsHtml5TreeOperation();

    ~nsHtml5TreeOperation();

    inline void Init(eHtml5TreeOperation aOpCode) {
      NS_PRECONDITION(mOpCode == eTreeOpUninitialized,
        "Op code must be uninitialized when initializing.");
      mOpCode = aOpCode;
    }

    inline void Init(eHtml5TreeOperation aOpCode, nsIContent** aNode) {
      NS_PRECONDITION(mOpCode == eTreeOpUninitialized,
        "Op code must be uninitialized when initializing.");
      mOpCode = aOpCode;
      mOne.node = aNode;
    }

    inline void Init(eHtml5TreeOperation aOpCode, 
                     nsIContent** aNode,
                     nsIContent** aParent) {
      NS_PRECONDITION(mOpCode == eTreeOpUninitialized,
        "Op code must be uninitialized when initializing.");
      mOpCode = aOpCode;
      mOne.node = aNode;
      mTwo.node = aParent;
    }

    inline void Init(eHtml5TreeOperation aOpCode,
                     nsIContent** aNode,
                     nsIContent** aParent, 
                     nsIContent** aTable) {
      NS_PRECONDITION(mOpCode == eTreeOpUninitialized,
        "Op code must be uninitialized when initializing.");
      mOpCode = aOpCode;
      mOne.node = aNode;
      mTwo.node = aParent;
      mThree.node = aTable;
    }

    inline void Init(nsHtml5DocumentMode aMode) {
      NS_PRECONDITION(mOpCode == eTreeOpUninitialized,
        "Op code must be uninitialized when initializing.");
      mOpCode = eTreeOpDocumentMode;
      mOne.mode = aMode;
    }
    
    inline void InitScript(nsIContent** aNode) {
      NS_PRECONDITION(mOpCode == eTreeOpUninitialized,
        "Op code must be uninitialized when initializing.");
      mOpCode = eTreeOpRunScript;
      mOne.node = aNode;
      mTwo.state = nsnull;
    }
    
    inline void Init(PRInt32 aNamespace, 
                     nsIAtom* aName, 
                     nsHtml5HtmlAttributes* aAttributes,
                     nsIContent** aTarget) {
      NS_PRECONDITION(mOpCode == eTreeOpUninitialized,
        "Op code must be uninitialized when initializing.");
      mOpCode = eTreeOpCreateElement;
      mInt = aNamespace;
      mOne.node = aTarget;
      mTwo.atom = aName;
      if (aAttributes == nsHtml5HtmlAttributes::EMPTY_ATTRIBUTES) {
        mThree.attributes = nsnull;
      } else {
        mThree.attributes = aAttributes;
      }
    }

    inline void Init(eHtml5TreeOperation aOpCode, 
                     PRUnichar* aBuffer, 
                     PRInt32 aLength, 
                     nsIContent** aTarget) {
      NS_PRECONDITION(mOpCode == eTreeOpUninitialized,
        "Op code must be uninitialized when initializing.");
      mOpCode = aOpCode;
      mOne.node = aTarget;
      mTwo.unicharPtr = aBuffer;
      mInt = aLength;
    }
    
    inline void Init(nsIContent** aElement,
                     nsHtml5HtmlAttributes* aAttributes) {
      NS_PRECONDITION(mOpCode == eTreeOpUninitialized,
        "Op code must be uninitialized when initializing.");
      mOpCode = eTreeOpAddAttributes;
      mOne.node = aElement;
      mTwo.attributes = aAttributes;
    }
    
    inline void Init(nsIAtom* aName, 
                     const nsAString& aPublicId, 
                     const nsAString& aSystemId, nsIContent** aTarget) {
      NS_PRECONDITION(mOpCode == eTreeOpUninitialized,
        "Op code must be uninitialized when initializing.");
      mOpCode = eTreeOpCreateDoctype;
      mOne.atom = aName;
      mTwo.stringPair = new nsHtml5TreeOperationStringPair(aPublicId, aSystemId);
      mThree.node = aTarget;
    }
    
    inline void Init(eHtml5TreeOperation aOpCode, const nsACString& aString) {
      NS_PRECONDITION(mOpCode == eTreeOpUninitialized,
        "Op code must be uninitialized when initializing.");

      PRInt32 len = aString.Length();
      char* str = new char[len + 1];
      const char* start = aString.BeginReading();
      for (PRInt32 i = 0; i < len; ++i) {
        str[i] = start[i];
      }
      str[len] = '\0';

      mOpCode = aOpCode;
      mOne.charPtr = str;
    }

    inline void Init(eHtml5TreeOperation aOpCode,
                     nsIContent** aNode,
                     PRInt32 aInt) {
      NS_PRECONDITION(mOpCode == eTreeOpUninitialized,
        "Op code must be uninitialized when initializing.");

      mOpCode = aOpCode;
      mOne.node = aNode;
      mInt = aInt;
    }

    inline PRBool IsRunScript() {
      return mOpCode == eTreeOpRunScript;
    }
    
    inline void SetSnapshot(nsAHtml5TreeBuilderState* aSnapshot, PRInt32 aLine) {
      NS_ASSERTION(IsRunScript(), 
        "Setting a snapshot for a tree operation other than eTreeOpRunScript!");
      mTwo.state = aSnapshot;
      mInt = aLine;
    }

    nsresult Perform(nsHtml5TreeOpExecutor* aBuilder);

    inline already_AddRefed<nsIAtom> Reget(nsIAtom* aAtom) {
      if (!aAtom || aAtom->IsStaticAtom()) {
        return aAtom;
      }
      nsAutoString str;
      aAtom->ToString(str);
      return do_GetAtom(str);
    }

  private:
  
    
    
    
    eHtml5TreeOperation mOpCode;
    union {
      nsIContent**                    node;
      nsIAtom*                        atom;
      nsHtml5HtmlAttributes*          attributes;
      nsHtml5DocumentMode             mode;
      PRUnichar*                      unicharPtr;
      char*                           charPtr;
      nsHtml5TreeOperationStringPair* stringPair;
      nsAHtml5TreeBuilderState*       state;
    }                   mOne, mTwo, mThree;
    PRInt32             mInt; 
                              
                              
                              
};

#endif 

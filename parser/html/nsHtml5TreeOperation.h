



































 
#ifndef nsHtml5TreeOperation_h__
#define nsHtml5TreeOperation_h__

#include "nsIContent.h"
#include "nsHtml5DocumentMode.h"
#include "nsHtml5HtmlAttributes.h"

class nsHtml5TreeOpExecutor;

enum eHtml5TreeOperation {
  
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
  eTreeOpUpdateStyleSheet,
  eTreeOpProcessBase,
  eTreeOpProcessMeta,
  eTreeOpProcessOfflineManifest,
  eTreeOpMarkMalformedIfScript,
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

    inline void Init(nsIContent** aNode, nsIContent** aParent) {
      mOne.node = aNode;
      mTwo.node = aParent;
    }

    inline void Init(eHtml5TreeOperation aOpCode, nsIContent** aNode) {
      mOpCode = aOpCode;
      mOne.node = aNode;
    }

    inline void Init(eHtml5TreeOperation aOpCode, 
                     nsIContent** aNode,
                     nsIContent** aParent) {
      mOpCode = aOpCode;
      mOne.node = aNode;
      mTwo.node = aParent;
    }

    inline void Init(eHtml5TreeOperation aOpCode,
                     nsIContent** aNode,
                     nsIContent** aParent, 
                     nsIContent** aTable) {
      mOpCode = aOpCode;
      mOne.node = aNode;
      mTwo.node = aParent;
      mThree.node = aTable;
    }

    inline void Init(nsHtml5DocumentMode aMode) {
      mOpCode = eTreeOpDocumentMode;
      mOne.mode = aMode;
    }
    
    inline void Init(PRInt32 aNamespace, 
                     nsIAtom* aName, 
                     nsHtml5HtmlAttributes* aAttributes,
                     nsIContent** aTarget) {
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
      mOpCode = aOpCode;
      mOne.node = aTarget;
      mTwo.unicharPtr = aBuffer;
      mInt = aLength;
    }
    
    inline void Init(nsIContent** aElement,
                     nsHtml5HtmlAttributes* aAttributes) {
      mOpCode = eTreeOpAddAttributes;
      mOne.node = aElement;
      mTwo.attributes = aAttributes;
    }
    
    inline void Init(nsIAtom* aName, 
                     const nsAString& aPublicId, 
                     const nsAString& aSystemId, nsIContent** aTarget) {
      mOpCode = eTreeOpCreateDoctype;
      mOne.atom = aName;
      mTwo.stringPair = new nsHtml5TreeOperationStringPair(aPublicId, aSystemId);
      mThree.node = aTarget;
    }

    inline PRBool IsRunScript() {
      return mOpCode == eTreeOpRunScript;
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
      nsHtml5TreeOperationStringPair* stringPair;
    }                   mOne, mTwo, mThree;
    PRInt32             mInt; 
                              
                              
                              
};

#endif 

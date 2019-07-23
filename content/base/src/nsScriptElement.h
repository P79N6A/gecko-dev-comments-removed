




































#include "nsIScriptLoaderObserver.h"
#include "nsIScriptElement.h"
#include "nsStubMutationObserver.h"
#include "prtypes.h"







class nsScriptElement : public nsIScriptElement,
                        public nsStubMutationObserver
{
public:
  
  NS_DECL_NSISCRIPTLOADEROBSERVER

  
  virtual void CharacterDataChanged(nsIDocument* aDocument,
                                    nsIContent* aContent,
                                    CharacterDataChangeInfo* aInfo);
  virtual void AttributeChanged(nsIDocument* aDocument,
                                nsIContent* aContent,
                                PRInt32 aNameSpaceID,
                                nsIAtom* aAttribute,
                                PRInt32 aModType);
  virtual void ContentAppended(nsIDocument* aDocument,
                               nsIContent* aContainer,
                               PRInt32 aNewIndexInContainer);
  virtual void ContentInserted(nsIDocument* aDocument,
                               nsIContent* aContainer,
                               nsIContent* aChild,
                               PRInt32 aIndexInContainer);

protected:
  

  


  virtual PRBool HasScriptContent() = 0;

  












  virtual nsresult MaybeProcessScript();
};

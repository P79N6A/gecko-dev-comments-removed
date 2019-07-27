





#ifndef nsChangeObserver_h_
#define nsChangeObserver_h_

class nsIContent;
class nsIDocument;
class nsIAtom;

#define NS_DECL_CHANGEOBSERVER \
void ObserveAttributeChanged(nsIDocument *aDocument, nsIContent *aContent, nsIAtom *aAttribute) override; \
void ObserveContentRemoved(nsIDocument *aDocument, nsIContent *aChild, int32_t aIndexInContainer) override; \
void ObserveContentInserted(nsIDocument *aDocument, nsIContent* aContainer, nsIContent *aChild) override;









class nsChangeObserver
{
public:
  
  virtual void ObserveAttributeChanged(nsIDocument* aDocument,
                                       nsIContent* aContent,
                                       nsIAtom* aAttribute)=0;

  virtual void ObserveContentRemoved(nsIDocument* aDocument,
                                     nsIContent* aChild, 
                                     int32_t aIndexInContainer)=0;

  virtual void ObserveContentInserted(nsIDocument* aDocument,
                                      nsIContent* aContainer,
                                      nsIContent* aChild)=0;
};

#endif 

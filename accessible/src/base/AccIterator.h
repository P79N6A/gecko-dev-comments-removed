




































#ifndef nsAccIterator_h_
#define nsAccIterator_h_

#include "filters.h"
#include "nscore.h"
#include "nsDocAccessible.h"





class AccIterator
{
public:
  


  enum IterationType {
    


    eFlatNav,

    



    eTreeNav
  };

  AccIterator(nsAccessible* aRoot, filters::FilterFuncPtr aFilterFunc,
              IterationType aIterationType = eFlatNav);
  ~AccIterator();

  



  nsAccessible *GetNext();

private:
  AccIterator();
  AccIterator(const AccIterator&);
  AccIterator& operator =(const AccIterator&);

  struct IteratorState
  {
    IteratorState(nsAccessible *aParent, IteratorState *mParentState = nsnull);

    nsAccessible *mParent;
    PRInt32 mIndex;
    IteratorState *mParentState;
  };

  filters::FilterFuncPtr mFilterFunc;
  PRBool mIsDeep;
  IteratorState *mState;
};






class RelatedAccIterator
{
public:
  









  RelatedAccIterator(nsDocAccessible* aDocument, nsIContent* aDependentContent,
                     nsIAtom* aRelAttr);

  


  nsAccessible* Next();

private:
  RelatedAccIterator();
  RelatedAccIterator(const RelatedAccIterator&);
  RelatedAccIterator& operator = (const RelatedAccIterator&);

  nsIAtom* mRelAttr;
  nsDocAccessible::AttrRelProviderArray* mProviders;
  nsIContent* mBindingParent;
  PRUint32 mIndex;
};





class HTMLLabelIterator
{
public:
  enum LabelFilter {
    eAllLabels,
    eSkipAncestorLabel
  };

  HTMLLabelIterator(nsDocAccessible* aDocument, nsIContent* aElement,
                    LabelFilter aFilter = eAllLabels);

  


  nsAccessible* Next();

private:
  HTMLLabelIterator();
  HTMLLabelIterator(const HTMLLabelIterator&);
  HTMLLabelIterator& operator = (const HTMLLabelIterator&);

  RelatedAccIterator mRelIter;
  nsIContent* mElement;
  LabelFilter mLabelFilter;
};





class HTMLOutputIterator
{
public:
  HTMLOutputIterator(nsDocAccessible* aDocument, nsIContent* aElement);

  


  nsAccessible* Next();

private:
  HTMLOutputIterator();
  HTMLOutputIterator(const HTMLOutputIterator&);
  HTMLOutputIterator& operator = (const HTMLOutputIterator&);

  RelatedAccIterator mRelIter;
};





class XULLabelIterator
{
public:
  XULLabelIterator(nsDocAccessible* aDocument, nsIContent* aElement);

  


  nsAccessible* Next();

private:
  XULLabelIterator();
  XULLabelIterator(const XULLabelIterator&);
  XULLabelIterator& operator = (const XULLabelIterator&);

  RelatedAccIterator mRelIter;
};





class XULDescriptionIterator
{
public:
  XULDescriptionIterator(nsDocAccessible* aDocument, nsIContent* aElement);

  


  nsAccessible* Next();

private:
  XULDescriptionIterator();
  XULDescriptionIterator(const XULDescriptionIterator&);
  XULDescriptionIterator& operator = (const XULDescriptionIterator&);

  RelatedAccIterator mRelIter;
};

#endif

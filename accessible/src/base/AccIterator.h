






































#ifndef nsAccIterator_h_
#define nsAccIterator_h_

#include "nsAccessibilityService.h"
#include "filters.h"
#include "nscore.h"
#include "nsDocAccessible.h"

#include "nsIDOMDocumentXBL.h"




class AccIterable
{
public:
  virtual ~AccIterable() { }
  virtual nsAccessible* Next() = 0;

private:
  friend class Relation;
  nsAutoPtr<AccIterable> mNextIter;
};





class AccIterator : public AccIterable
{
public:
  


  enum IterationType {
    


    eFlatNav,

    



    eTreeNav
  };

  AccIterator(nsAccessible* aRoot, filters::FilterFuncPtr aFilterFunc,
              IterationType aIterationType = eFlatNav);
  virtual ~AccIterator();

  



  virtual nsAccessible *Next();

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
  bool mIsDeep;
  IteratorState *mState;
};






class RelatedAccIterator : public AccIterable
{
public:
  









  RelatedAccIterator(nsDocAccessible* aDocument, nsIContent* aDependentContent,
                     nsIAtom* aRelAttr);

  virtual ~RelatedAccIterator() { }

  


  virtual nsAccessible* Next();

private:
  RelatedAccIterator();
  RelatedAccIterator(const RelatedAccIterator&);
  RelatedAccIterator& operator = (const RelatedAccIterator&);

  nsDocAccessible* mDocument;
  nsIAtom* mRelAttr;
  nsDocAccessible::AttrRelProviderArray* mProviders;
  nsIContent* mBindingParent;
  PRUint32 mIndex;
};





class HTMLLabelIterator : public AccIterable
{
public:
  enum LabelFilter {
    eAllLabels,
    eSkipAncestorLabel
  };

  HTMLLabelIterator(nsDocAccessible* aDocument, const nsAccessible* aAccessible,
                    LabelFilter aFilter = eAllLabels);

  virtual ~HTMLLabelIterator() { }

  


  virtual nsAccessible* Next();

private:
  HTMLLabelIterator();
  HTMLLabelIterator(const HTMLLabelIterator&);
  HTMLLabelIterator& operator = (const HTMLLabelIterator&);

  RelatedAccIterator mRelIter;
  
  
  const nsAccessible* mAcc;
  LabelFilter mLabelFilter;
};





class HTMLOutputIterator : public AccIterable
{
public:
  HTMLOutputIterator(nsDocAccessible* aDocument, nsIContent* aElement);
  virtual ~HTMLOutputIterator() { }

  


  virtual nsAccessible* Next();

private:
  HTMLOutputIterator();
  HTMLOutputIterator(const HTMLOutputIterator&);
  HTMLOutputIterator& operator = (const HTMLOutputIterator&);

  RelatedAccIterator mRelIter;
};





class XULLabelIterator : public AccIterable
{
public:
  XULLabelIterator(nsDocAccessible* aDocument, nsIContent* aElement);
  virtual ~XULLabelIterator() { }

  


  virtual nsAccessible* Next();

private:
  XULLabelIterator();
  XULLabelIterator(const XULLabelIterator&);
  XULLabelIterator& operator = (const XULLabelIterator&);

  RelatedAccIterator mRelIter;
};





class XULDescriptionIterator : public AccIterable
{
public:
  XULDescriptionIterator(nsDocAccessible* aDocument, nsIContent* aElement);
  virtual ~XULDescriptionIterator() { }

  


  virtual nsAccessible* Next();

private:
  XULDescriptionIterator();
  XULDescriptionIterator(const XULDescriptionIterator&);
  XULDescriptionIterator& operator = (const XULDescriptionIterator&);

  RelatedAccIterator mRelIter;
};






class IDRefsIterator : public AccIterable
{
public:
  IDRefsIterator(nsIContent* aContent, nsIAtom* aIDRefsAttr);
  virtual ~IDRefsIterator() { }

  


  const nsDependentSubstring NextID();

  


  nsIContent* NextElem();

  


  nsIContent* GetElem(const nsDependentSubstring& aID);

  
  virtual nsAccessible* Next();

private:
  IDRefsIterator();
  IDRefsIterator(const IDRefsIterator&);
  IDRefsIterator operator = (const IDRefsIterator&);

  nsString mIDs;
  nsAString::index_type mCurrIdx;

  nsIDocument* mDocument;
  nsCOMPtr<nsIDOMDocumentXBL> mXBLDocument;
  nsCOMPtr<nsIDOMElement> mBindingParent;
};





class SingleAccIterator : public AccIterable
{
public:
  SingleAccIterator(nsAccessible* aTarget): mAcc(aTarget) { }
  virtual ~SingleAccIterator() { }

  virtual nsAccessible* Next();

private:
  SingleAccIterator();
  SingleAccIterator(const SingleAccIterator&);
  SingleAccIterator& operator = (const SingleAccIterator&);

  nsRefPtr<nsAccessible> mAcc;
};

#endif

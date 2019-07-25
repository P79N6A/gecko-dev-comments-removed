





#ifndef nsAccIterator_h_
#define nsAccIterator_h_

#include "nsAccessibilityService.h"
#include "filters.h"
#include "nscore.h"
#include "DocAccessible.h"




class AccIterable
{
public:
  virtual ~AccIterable() { }
  virtual Accessible* Next() = 0;

private:
  friend class mozilla::a11y::Relation;
  nsAutoPtr<AccIterable> mNextIter;
};





class AccIterator : public AccIterable
{
public:
  


  enum IterationType {
    


    eFlatNav,

    



    eTreeNav
  };

  AccIterator(Accessible* aRoot, filters::FilterFuncPtr aFilterFunc,
              IterationType aIterationType = eFlatNav);
  virtual ~AccIterator();

  



  virtual Accessible* Next();

private:
  AccIterator();
  AccIterator(const AccIterator&);
  AccIterator& operator =(const AccIterator&);

  struct IteratorState
  {
    IteratorState(Accessible* aParent, IteratorState* mParentState = nullptr);

    Accessible* mParent;
    int32_t mIndex;
    IteratorState *mParentState;
  };

  filters::FilterFuncPtr mFilterFunc;
  bool mIsDeep;
  IteratorState *mState;
};






class RelatedAccIterator : public AccIterable
{
public:
  









  RelatedAccIterator(DocAccessible* aDocument, nsIContent* aDependentContent,
                     nsIAtom* aRelAttr);

  virtual ~RelatedAccIterator() { }

  


  virtual Accessible* Next();

private:
  RelatedAccIterator();
  RelatedAccIterator(const RelatedAccIterator&);
  RelatedAccIterator& operator = (const RelatedAccIterator&);

  DocAccessible* mDocument;
  nsIAtom* mRelAttr;
  DocAccessible::AttrRelProviderArray* mProviders;
  nsIContent* mBindingParent;
  uint32_t mIndex;
};





class HTMLLabelIterator : public AccIterable
{
public:
  enum LabelFilter {
    eAllLabels,
    eSkipAncestorLabel
  };

  HTMLLabelIterator(DocAccessible* aDocument, const Accessible* aAccessible,
                    LabelFilter aFilter = eAllLabels);

  virtual ~HTMLLabelIterator() { }

  


  virtual Accessible* Next();

private:
  HTMLLabelIterator();
  HTMLLabelIterator(const HTMLLabelIterator&);
  HTMLLabelIterator& operator = (const HTMLLabelIterator&);

  RelatedAccIterator mRelIter;
  
  
  const Accessible* mAcc;
  LabelFilter mLabelFilter;
};





class HTMLOutputIterator : public AccIterable
{
public:
  HTMLOutputIterator(DocAccessible* aDocument, nsIContent* aElement);
  virtual ~HTMLOutputIterator() { }

  


  virtual Accessible* Next();

private:
  HTMLOutputIterator();
  HTMLOutputIterator(const HTMLOutputIterator&);
  HTMLOutputIterator& operator = (const HTMLOutputIterator&);

  RelatedAccIterator mRelIter;
};





class XULLabelIterator : public AccIterable
{
public:
  XULLabelIterator(DocAccessible* aDocument, nsIContent* aElement);
  virtual ~XULLabelIterator() { }

  


  virtual Accessible* Next();

private:
  XULLabelIterator();
  XULLabelIterator(const XULLabelIterator&);
  XULLabelIterator& operator = (const XULLabelIterator&);

  RelatedAccIterator mRelIter;
};





class XULDescriptionIterator : public AccIterable
{
public:
  XULDescriptionIterator(DocAccessible* aDocument, nsIContent* aElement);
  virtual ~XULDescriptionIterator() { }

  


  virtual Accessible* Next();

private:
  XULDescriptionIterator();
  XULDescriptionIterator(const XULDescriptionIterator&);
  XULDescriptionIterator& operator = (const XULDescriptionIterator&);

  RelatedAccIterator mRelIter;
};






class IDRefsIterator : public AccIterable
{
public:
  IDRefsIterator(DocAccessible* aDoc, nsIContent* aContent,
                 nsIAtom* aIDRefsAttr);
  virtual ~IDRefsIterator() { }

  


  const nsDependentSubstring NextID();

  


  nsIContent* NextElem();

  


  nsIContent* GetElem(const nsDependentSubstring& aID);

  
  virtual Accessible* Next();

private:
  IDRefsIterator();
  IDRefsIterator(const IDRefsIterator&);
  IDRefsIterator operator = (const IDRefsIterator&);

  nsString mIDs;
  nsIContent* mContent;
  DocAccessible* mDoc;
  nsAString::index_type mCurrIdx;
};





class SingleAccIterator : public AccIterable
{
public:
  SingleAccIterator(Accessible* aTarget): mAcc(aTarget) { }
  virtual ~SingleAccIterator() { }

  virtual Accessible* Next();

private:
  SingleAccIterator();
  SingleAccIterator(const SingleAccIterator&);
  SingleAccIterator& operator = (const SingleAccIterator&);

  nsRefPtr<Accessible> mAcc;
};

#endif

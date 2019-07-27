





#ifndef mozilla_a11y_AccIterator_h__
#define mozilla_a11y_AccIterator_h__

#include "DocAccessible.h"
#include "Filters.h"

class nsITreeView;

namespace mozilla {
namespace a11y {




class AccIterable
{
public:
  virtual ~AccIterable() { }
  virtual Accessible* Next() = 0;

private:
  friend class Relation;
  nsAutoPtr<AccIterable> mNextIter;
};





class AccIterator : public AccIterable
{
public:
  AccIterator(Accessible* aRoot, filters::FilterFuncPtr aFilterFunc);
  virtual ~AccIterator();

  



  virtual Accessible* Next() override;

private:
  AccIterator();
  AccIterator(const AccIterator&);
  AccIterator& operator =(const AccIterator&);

  struct IteratorState
  {
    explicit IteratorState(Accessible* aParent, IteratorState* mParentState = nullptr);

    Accessible* mParent;
    int32_t mIndex;
    IteratorState* mParentState;
  };

  filters::FilterFuncPtr mFilterFunc;
  IteratorState* mState;
};






class RelatedAccIterator : public AccIterable
{
public:
  









  RelatedAccIterator(DocAccessible* aDocument, nsIContent* aDependentContent,
                     nsIAtom* aRelAttr);

  virtual ~RelatedAccIterator() { }

  


  virtual Accessible* Next() override;

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

  


  virtual Accessible* Next() override;

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

  


  virtual Accessible* Next() override;

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

  


  virtual Accessible* Next() override;

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

  


  virtual Accessible* Next() override;

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

  
  virtual Accessible* Next() override;

private:
  IDRefsIterator();
  IDRefsIterator(const IDRefsIterator&);
  IDRefsIterator operator = (const IDRefsIterator&);

  nsString mIDs;
  nsIContent* mContent;
  DocAccessible* mDoc;
  nsAString::index_type mCurrIdx;
};





class ARIAOwnedByIterator final : public RelatedAccIterator
{
public:
  explicit ARIAOwnedByIterator(const Accessible* aDependent);
  virtual ~ARIAOwnedByIterator() { }

  virtual Accessible* Next() override;

private:
  ARIAOwnedByIterator() = delete;
  ARIAOwnedByIterator(const ARIAOwnedByIterator&) = delete;
  ARIAOwnedByIterator& operator = (const ARIAOwnedByIterator&) = delete;

  const Accessible* mDependent;
};





class ARIAOwnsIterator final : public AccIterable
{
public:
  explicit ARIAOwnsIterator(const Accessible* aOwner);
  virtual ~ARIAOwnsIterator() { }

  virtual Accessible* Next() override;

private:
  ARIAOwnsIterator() = delete;
  ARIAOwnsIterator(const ARIAOwnsIterator&) = delete;
  ARIAOwnsIterator& operator = (const ARIAOwnsIterator&) = delete;

  IDRefsIterator mIter;
  const Accessible* mOwner;
};






class SingleAccIterator : public AccIterable
{
public:
  explicit SingleAccIterator(Accessible* aTarget): mAcc(aTarget) { }
  virtual ~SingleAccIterator() { }

  virtual Accessible* Next() override;

private:
  SingleAccIterator();
  SingleAccIterator(const SingleAccIterator&);
  SingleAccIterator& operator = (const SingleAccIterator&);

  nsRefPtr<Accessible> mAcc;
};





class ItemIterator : public AccIterable
{
public:
  explicit ItemIterator(Accessible* aItemContainer) :
    mContainer(aItemContainer), mAnchor(nullptr) { }
  virtual ~ItemIterator() { }

  virtual Accessible* Next() override;

private:
  ItemIterator() = delete;
  ItemIterator(const ItemIterator&) = delete;
  ItemIterator& operator = (const ItemIterator&) = delete;

  Accessible* mContainer;
  Accessible* mAnchor;
};





class XULTreeItemIterator : public AccIterable
{
public:
  XULTreeItemIterator(XULTreeAccessible* aXULTree, nsITreeView* aTreeView,
                      int32_t aRowIdx);
  virtual ~XULTreeItemIterator() { }

  virtual Accessible* Next() override;

private:
  XULTreeItemIterator() = delete;
  XULTreeItemIterator(const XULTreeItemIterator&) = delete;
  XULTreeItemIterator& operator = (const XULTreeItemIterator&) = delete;

  XULTreeAccessible* mXULTree;
  nsITreeView* mTreeView;
  int32_t mRowCount;
  int32_t mContainerLevel;
  int32_t mCurrRowIdx;
};

} 
} 

#endif

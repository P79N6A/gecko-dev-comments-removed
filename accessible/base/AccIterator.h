





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

  



  virtual Accessible* Next();

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
  explicit SingleAccIterator(Accessible* aTarget): mAcc(aTarget) { }
  virtual ~SingleAccIterator() { }

  virtual Accessible* Next();

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

  virtual Accessible* Next();

private:
  ItemIterator() MOZ_DELETE;
  ItemIterator(const ItemIterator&) MOZ_DELETE;
  ItemIterator& operator = (const ItemIterator&) MOZ_DELETE;

  Accessible* mContainer;
  Accessible* mAnchor;
};





class XULTreeItemIterator : public AccIterable
{
public:
  XULTreeItemIterator(XULTreeAccessible* aXULTree, nsITreeView* aTreeView,
                      int32_t aRowIdx);
  virtual ~XULTreeItemIterator() { }

  virtual Accessible* Next();

private:
  XULTreeItemIterator() MOZ_DELETE;
  XULTreeItemIterator(const XULTreeItemIterator&) MOZ_DELETE;
  XULTreeItemIterator& operator = (const XULTreeItemIterator&) MOZ_DELETE;

  XULTreeAccessible* mXULTree;
  nsITreeView* mTreeView;
  int32_t mRowCount;
  int32_t mContainerLevel;
  int32_t mCurrRowIdx;
};

} 
} 

#endif

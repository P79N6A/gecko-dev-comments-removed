





#ifndef mozilla_a11y_ProxyAccessible_h
#define mozilla_a11y_ProxyAccessible_h

#include "mozilla/a11y/Role.h"
#include "nsIAccessibleText.h"
#include "Accessible.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsRect.h"
#include "Accessible.h"

namespace mozilla {
namespace a11y {

class Attribute;
class DocAccessibleParent;
enum class RelationType;

class ProxyAccessible
{
public:

  ProxyAccessible(uint64_t aID, ProxyAccessible* aParent,
                  DocAccessibleParent* aDoc, role aRole) :
     mParent(aParent), mDoc(aDoc), mWrapper(0), mID(aID), mRole(aRole),
     mOuterDoc(false)
  {
    MOZ_COUNT_CTOR(ProxyAccessible);
  }
  ~ProxyAccessible()
  {
    MOZ_COUNT_DTOR(ProxyAccessible);
    MOZ_ASSERT(!mWrapper);
  }

  void AddChildAt(uint32_t aIdx, ProxyAccessible* aChild)
  { mChildren.InsertElementAt(aIdx, aChild); }

  uint32_t ChildrenCount() const { return mChildren.Length(); }
  ProxyAccessible* ChildAt(uint32_t aIdx) const { return mChildren[aIdx]; }

  
  size_t IndexInParent() const { return mParent->mChildren.IndexOf(this); }
  bool MustPruneChildren() const;

  void Shutdown();

  void SetChildDoc(DocAccessibleParent*);

  


  void RemoveChild(ProxyAccessible* aChild)
    { mChildren.RemoveElement(aChild); }

  


  ProxyAccessible* Parent() const { return mParent; }

  


  role Role() const { return mRole; }

  


  uint64_t State() const;

  


  void Name(nsString& aName) const;

  


  void Value(nsString& aValue) const;

  


  void Description(nsString& aDesc) const;

  


  void Attributes(nsTArray<Attribute> *aAttrs) const;

  


  nsTArray<ProxyAccessible*> RelationByType(RelationType aType) const;

  


  void Relations(nsTArray<RelationType>* aTypes,
                 nsTArray<nsTArray<ProxyAccessible*>>* aTargetSets) const;

  int32_t CaretOffset();
  bool SetCaretOffset(int32_t aOffset);

  int32_t CharacterCount();
  int32_t SelectionCount();

  


  void TextSubstring(int32_t aStartOffset, int32_t aEndOfset,
                     nsString& aText) const;

  void GetTextAfterOffset(int32_t aOffset, AccessibleTextBoundary aBoundaryType,
                          nsString& aText, int32_t* aStartOffset,
                          int32_t* aEndOffset);

  void GetTextAtOffset(int32_t aOffset, AccessibleTextBoundary aBoundaryType,
                       nsString& aText, int32_t* aStartOffset,
                       int32_t* aEndOffset);

  void GetTextBeforeOffset(int32_t aOffset, AccessibleTextBoundary aBoundaryType,
                           nsString& aText, int32_t* aStartOffset,
                           int32_t* aEndOffset);

  char16_t CharAt(int32_t aOffset);

  void TextAttributes(bool aIncludeDefAttrs,
                      const int32_t aOffset,
                      nsTArray<Attribute>* aAttributes,
                      int32_t* aStartOffset,
                      int32_t* aEndOffset);
  void DefaultTextAttributes(nsTArray<Attribute>* aAttrs);

  nsIntRect TextBounds(int32_t aStartOffset, int32_t aEndOffset,
                       uint32_t aCoordType);

  nsIntRect CharBounds(int32_t aOffset, uint32_t aCoordType);

  int32_t OffsetAtPoint(int32_t aX, int32_t aY, uint32_t aCoordType);

  bool SelectionBoundsAt(int32_t aSelectionNum,
                         nsString& aData,
                         int32_t* aStartOffset,
                         int32_t* aEndOffset);

  bool SetSelectionBoundsAt(int32_t aSelectionNum,
                            int32_t aStartOffset,
                            int32_t aEndOffset);

  bool AddToSelection(int32_t aStartOffset,
                      int32_t aEndOffset);

  bool RemoveFromSelection(int32_t aSelectionNum);

  void ScrollSubstringTo(int32_t aStartOffset, int32_t aEndOffset,
                         uint32_t aScrollType);

  void ScrollSubstringToPoint(int32_t aStartOffset,
                              int32_t aEndOffset,
                              uint32_t aCoordinateType,
                              int32_t aX, int32_t aY);

  void ReplaceText(const nsString& aText);

  void InsertText(const nsString& aText, int32_t aPosition);

  void CopyText(int32_t aStartPos, int32_t aEndPos);

  void CutText(int32_t aStartPos, int32_t aEndPos);

  void DeleteText(int32_t aStartPos, int32_t aEndPos);

  void PasteText(int32_t aPosition);

  nsIntPoint ImagePosition(uint32_t aCoordType);

  nsIntSize ImageSize();

  uint32_t StartOffset(bool* aOk);

  uint32_t EndOffset(bool* aOk);

  bool IsLinkValid();

  uint32_t AnchorCount(bool* aOk);

  void AnchorURIAt(uint32_t aIndex, nsCString& aURI, bool* aOk);

  ProxyAccessible* AnchorAt(uint32_t aIndex);

  uint32_t LinkCount();

  ProxyAccessible* LinkAt(const uint32_t& aIndex);

  int32_t LinkIndexOf(ProxyAccessible* aLink);

  int32_t LinkIndexAtOffset(uint32_t aOffset);

  ProxyAccessible* TableOfACell();

  uint32_t ColIdx();

  uint32_t RowIdx();

  uint32_t ColExtent();

  uint32_t RowExtent();

  void ColHeaderCells(nsTArray<uint64_t>* aCells);

  void RowHeaderCells(nsTArray<uint64_t>* aCells);

  bool IsCellSelected();

  ProxyAccessible* TableCaption();
  void TableSummary(nsString& aSummary);
  uint32_t TableColumnCount();
  uint32_t TableRowCount();
  ProxyAccessible* TableCellAt(uint32_t aRow, uint32_t aCol);
  int32_t TableCellIndexAt(uint32_t aRow, uint32_t aCol);
  int32_t TableColumnIndexAt(uint32_t aCellIndex);
  int32_t TableRowIndexAt(uint32_t aCellIndex);
  void TableRowAndColumnIndicesAt(uint32_t aCellIndex,
                                  int32_t* aRow, int32_t* aCol);
  uint32_t TableColumnExtentAt(uint32_t aRow, uint32_t aCol);
  uint32_t TableRowExtentAt(uint32_t aRow, uint32_t aCol);
  void TableColumnDescription(uint32_t aCol, nsString& aDescription);
  void TableRowDescription(uint32_t aRow, nsString& aDescription);
  bool TableColumnSelected(uint32_t aCol);
  bool TableRowSelected(uint32_t aRow);
  bool TableCellSelected(uint32_t aRow, uint32_t aCol);
  uint32_t TableSelectedCellCount();
  uint32_t TableSelectedColumnCount();
  uint32_t TableSelectedRowCount();
  void TableSelectedCells(nsTArray<ProxyAccessible*>* aCellIDs);
  void TableSelectedCellIndices(nsTArray<uint32_t>* aCellIndices);
  void TableSelectedColumnIndices(nsTArray<uint32_t>* aColumnIndices);
  void TableSelectedRowIndices(nsTArray<uint32_t>* aRowIndices);
  void TableSelectColumn(uint32_t aCol);
  void TableSelectRow(uint32_t aRow);
  void TableUnselectColumn(uint32_t aCol);
  void TableUnselectRow(uint32_t aRow);
  bool TableIsProbablyForLayout();

  void SelectedItems(nsTArray<ProxyAccessible*>* aSelectedItems);
  uint32_t SelectedItemCount();
  ProxyAccessible* GetSelectedItem(uint32_t aIndex);
  bool IsItemSelected(uint32_t aIndex);
  bool AddItemToSelection(uint32_t aIndex);
  bool RemoveItemFromSelection(uint32_t aIndex);
  bool SelectAll();
  bool UnselectAll();

  bool DoAction(uint8_t aIndex);
  uint8_t ActionCount();
  void ActionDescriptionAt(uint8_t aIndex, nsString& aDescription);
  void ActionNameAt(uint8_t aIndex, nsString& aName);
  KeyBinding AccessKey();
  KeyBinding KeyboardShortcut();

  double CurValue();
  bool SetCurValue(double aValue);
  double MinValue();
  double MaxValue();
  double Step();

  void TakeFocus();
  ProxyAccessible* ChildAtPoint(int32_t aX, int32_t aY,
                                Accessible::EWhichChildAtPoint aWhichChild);
  nsIntRect Bounds();

  


  uintptr_t GetWrapper() const { return mWrapper; }
  void SetWrapper(uintptr_t aWrapper) { mWrapper = aWrapper; }

  


  uint64_t ID() const { return mID; }

protected:
  explicit ProxyAccessible(DocAccessibleParent* aThisAsDoc) :
    mParent(nullptr), mDoc(aThisAsDoc), mWrapper(0), mID(0),
    mRole(roles::DOCUMENT)
  { MOZ_COUNT_CTOR(ProxyAccessible); }

protected:
  ProxyAccessible* mParent;

private:
  nsTArray<ProxyAccessible*> mChildren;
  DocAccessibleParent* mDoc;
  uintptr_t mWrapper;
  uint64_t mID;
  role mRole : 31;
  bool mOuterDoc : 1;
};

enum Interfaces
{
  HYPERTEXT = 1
};

}
}

#endif

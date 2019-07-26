




#ifndef nsTreeColumns_h__
#define nsTreeColumns_h__

#include "nsITreeColumns.h"
#include "nsITreeBoxObject.h"
#include "mozilla/Attributes.h"
#include "nsCoord.h"
#include "nsCycleCollectionParticipant.h"
#include "nsAutoPtr.h"

class nsTreeBodyFrame;
class nsTreeColumns;
class nsIFrame;
class nsIContent;
struct nsRect;

#define NS_TREECOLUMN_IMPL_CID                       \
{ /* 02cd1963-4b5d-4a6c-9223-814d3ade93a3 */         \
    0x02cd1963,                                      \
    0x4b5d,                                          \
    0x4a6c,                                          \
    {0x92, 0x23, 0x81, 0x4d, 0x3a, 0xde, 0x93, 0xa3} \
}



class nsTreeColumn MOZ_FINAL : public nsITreeColumn {
public:
  nsTreeColumn(nsTreeColumns* aColumns, nsIContent* aContent);
  ~nsTreeColumn();

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_TREECOLUMN_IMPL_CID)

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsTreeColumn)
  NS_DECL_NSITREECOLUMN

  friend class nsTreeBodyFrame;
  friend class nsTreeColumns;

protected:
  nsIFrame* GetFrame();
  nsIFrame* GetFrame(nsTreeBodyFrame* aBodyFrame);
  
  bool IsLastVisible(nsTreeBodyFrame* aBodyFrame);

  



  nsresult GetRect(nsTreeBodyFrame* aBodyFrame, nscoord aY, nscoord aHeight,
                   nsRect* aResult);

  nsresult GetXInTwips(nsTreeBodyFrame* aBodyFrame, nscoord* aResult);
  nsresult GetWidthInTwips(nsTreeBodyFrame* aBodyFrame, nscoord* aResult);

  void SetColumns(nsTreeColumns* aColumns) { mColumns = aColumns; }

  const nsAString& GetId() { return mId; }
  nsIAtom* GetAtom() { return mAtom; }

  int32_t GetIndex() { return mIndex; }

  bool IsPrimary() { return mIsPrimary; }
  bool IsCycler() { return mIsCycler; }
  bool IsEditable() { return mIsEditable; }
  bool IsSelectable() { return mIsSelectable; }
  bool Overflow() { return mOverflow; }

  int16_t GetType() { return mType; }

  int8_t GetCropStyle() { return mCropStyle; }
  int32_t GetTextAlignment() { return mTextAlignment; }

  nsTreeColumn* GetNext() { return mNext; }
  nsTreeColumn* GetPrevious() { return mPrevious; }
  void SetNext(nsTreeColumn* aNext) {
    NS_ASSERTION(!mNext, "already have a next sibling");
    mNext = aNext;
  }
  void SetPrevious(nsTreeColumn* aPrevious) { mPrevious = aPrevious; }

private:
  


  nsCOMPtr<nsIContent> mContent;

  nsTreeColumns* mColumns;

  nsString mId;
  nsCOMPtr<nsIAtom> mAtom;

  int32_t mIndex;

  bool mIsPrimary;
  bool mIsCycler;
  bool mIsEditable;
  bool mIsSelectable;
  bool mOverflow;

  int16_t mType;

  int8_t mCropStyle;
  int8_t mTextAlignment;

  nsRefPtr<nsTreeColumn> mNext;
  nsTreeColumn* mPrevious;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsTreeColumn, NS_TREECOLUMN_IMPL_CID)

class nsTreeColumns MOZ_FINAL : public nsITreeColumns {
public:
  nsTreeColumns(nsTreeBodyFrame* aTree);
  ~nsTreeColumns();

  NS_DECL_ISUPPORTS
  NS_DECL_NSITREECOLUMNS

  nsITreeColumn* GetColumnAt(int32_t aIndex);
  nsITreeColumn* GetNamedColumn(const nsAString& aId);

  static nsTreeColumns* FromSupports(nsISupports* aSupports)
  {
#ifdef DEBUG
    {
      nsCOMPtr<nsITreeColumns> columns_qi = do_QueryInterface(aSupports);

      
      
      
      NS_ASSERTION(columns_qi == static_cast<nsITreeColumns*>(aSupports),
                   "Uh, fix QI!");
    }
#endif

    return static_cast<nsTreeColumns*>(aSupports);
  }

  friend class nsTreeBodyFrame;
protected:
  void SetTree(nsTreeBodyFrame* aTree) { mTree = aTree; }

  
  void EnsureColumns();

  nsTreeColumn* GetFirstColumn() { EnsureColumns(); return mFirstColumn; }
  nsTreeColumn* GetPrimaryColumn();

private:
  nsTreeBodyFrame* mTree;

  







  nsTreeColumn* mFirstColumn;
};

#endif 

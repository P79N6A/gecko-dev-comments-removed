











































#ifndef NSDISPLAYLIST_H_
#define NSDISPLAYLIST_H_

#include "nsCOMPtr.h"
#include "nsIFrame.h"
#include "nsPoint.h"
#include "nsRect.h"
#include "nsISelection.h"
#include "nsCaret.h"
#include "plarena.h"

#include <stdlib.h>

class nsIPresShell;
class nsIContent;
class nsRegion;
class nsIRenderingContext;
class nsIDeviceContext;
class nsDisplayTableItem;
class nsDisplayItem;







































#ifdef NS_DEBUG
#define NS_DISPLAY_DECL_NAME(n) virtual const char* Name() { return n; }
#else
#define NS_DISPLAY_DECL_NAME(n) 
#endif










class NS_STACK_CLASS nsDisplayListBuilder {
public:
  







  nsDisplayListBuilder(nsIFrame* aReferenceFrame, PRBool aIsForEvents,
                       PRBool aBuildCaret);
  ~nsDisplayListBuilder();

  



  PRBool IsForEventDelivery() { return mEventDelivery; }
  



  PRBool IsBackgroundOnly() { return mIsBackgroundOnly; }
  



  void SetBackgroundOnly(PRBool aIsBackgroundOnly) { mIsBackgroundOnly = aIsBackgroundOnly; }
  






  PRBool IsAtRootOfPseudoStackingContext() { return mIsAtRootOfPseudoStackingContext; }

  










  void SetMovingFrame(nsIFrame* aMovingFrame, const nsPoint& aMoveDelta,
                      nsRegion* aSaveVisibleRegionOfMovingContent) {
    mMovingFrame = aMovingFrame;
    mMoveDelta = aMoveDelta;
    mSaveVisibleRegionOfMovingContent = aSaveVisibleRegionOfMovingContent;
  }

  


  PRBool HasMovingFrames() { return mMovingFrame != nsnull; }
  


  nsIFrame* GetRootMovingFrame() { return mMovingFrame; }
  



  const nsPoint& GetMoveDelta() { return mMoveDelta; }
  




  void AccumulateVisibleRegionOfMovingContent(const nsRegion& aMovingContent,
                                              const nsRegion& aVisibleRegion);

  



  PRBool IsMovingFrame(nsIFrame* aFrame);
  



  nsISelection* GetBoundingSelection() { return mBoundingSelection; }
  



  nsIFrame* ReferenceFrame() { return mReferenceFrame; }
  





  nsPoint ToReferenceFrame(const nsIFrame* aFrame) {
    return aFrame->GetOffsetTo(ReferenceFrame());
  }
  





  void SetIgnoreScrollFrame(nsIFrame* aFrame) { mIgnoreScrollFrame = aFrame; }
  


  nsIFrame* GetIgnoreScrollFrame() { return mIgnoreScrollFrame; }
  



  void SetPaintAllFrames() { mPaintAllFrames = PR_TRUE; }
  PRBool GetPaintAllFrames() { return mPaintAllFrames; }
  



  void SetAccurateVisibleRegions() { mAccurateVisibleRegions = PR_TRUE; }
  PRBool GetAccurateVisibleRegions() { return mAccurateVisibleRegions; }
  



  void IgnorePaintSuppression() { mIsBackgroundOnly = PR_FALSE; }
  


  nsresult DisplayCaret(nsIFrame* aFrame, const nsRect& aDirtyRect,
      const nsDisplayListSet& aLists) {
    nsIFrame* frame = GetCaretFrame();
    if (aFrame != frame) {
      return NS_OK;
    }
    return frame->DisplayCaret(this, aDirtyRect, aLists);
  }
  



  nsIFrame* GetCaretFrame() {
    return CurrentPresShellState()->mCaretFrame;
  }
  


  nsCaret* GetCaret();
  




  void EnterPresShell(nsIFrame* aReferenceFrame, const nsRect& aDirtyRect);
  


  void LeavePresShell(nsIFrame* aReferenceFrame, const nsRect& aDirtyRect);

  




  PRBool IsInTransform() { return mInTransform; }
  



  void SetInTransform(PRBool aInTransform) { mInTransform = aInTransform; }

  


  PRBool ShouldSyncDecodeImages() { return mSyncDecodeImages; }

  




  void SetSyncDecodeImages(PRBool aSyncDecodeImages) {
    mSyncDecodeImages = aSyncDecodeImages;
  }

  




  PRUint32 GetBackgroundPaintFlags();

  





  void SubtractFromVisibleRegion(nsRegion* aVisibleRegion,
                                 const nsRegion& aRegion);

  







  void MarkFramesForDisplayList(nsIFrame* aDirtyFrame,
                                const nsFrameList& aFrames,
                                const nsRect& aDirtyRect);
  
  




  void* Allocate(size_t aSize);
  
  



  class AutoIsRootSetter;
  friend class AutoIsRootSetter;
  class AutoIsRootSetter {
  public:
    AutoIsRootSetter(nsDisplayListBuilder* aBuilder, PRBool aIsRoot)
      : mBuilder(aBuilder), mOldValue(aBuilder->mIsAtRootOfPseudoStackingContext) { 
      aBuilder->mIsAtRootOfPseudoStackingContext = aIsRoot;
    }
    ~AutoIsRootSetter() {
      mBuilder->mIsAtRootOfPseudoStackingContext = mOldValue;
    }
  private:
    nsDisplayListBuilder* mBuilder;
    PRPackedBool          mOldValue;
  };

  


  class AutoInTransformSetter;
  friend class AutoInTransformSetter;
  class AutoInTransformSetter {
  public:
    AutoInTransformSetter(nsDisplayListBuilder* aBuilder, PRBool aInTransform)
      : mBuilder(aBuilder), mOldValue(aBuilder->mInTransform) { 
      aBuilder->mInTransform = aInTransform;
    }
    ~AutoInTransformSetter() {
      mBuilder->mInTransform = mOldValue;
    }
  private:
    nsDisplayListBuilder* mBuilder;
    PRPackedBool          mOldValue;
  };  
  
  
  nsDisplayTableItem* GetCurrentTableItem() { return mCurrentTableItem; }
  void SetCurrentTableItem(nsDisplayTableItem* aTableItem) { mCurrentTableItem = aTableItem; }

private:
  
  
  void* operator new(size_t sz) CPP_THROW_NEW;
  
  struct PresShellState {
    nsIPresShell* mPresShell;
    nsIFrame*     mCaretFrame;
    PRUint32      mFirstFrameMarkedForDisplay;
  };
  PresShellState* CurrentPresShellState() {
    NS_ASSERTION(mPresShellStates.Length() > 0,
                 "Someone forgot to enter a presshell");
    return &mPresShellStates[mPresShellStates.Length() - 1];
  }
  
  nsIFrame*                      mReferenceFrame;
  nsIFrame*                      mMovingFrame;
  nsRegion*                      mSaveVisibleRegionOfMovingContent;
  nsIFrame*                      mIgnoreScrollFrame;
  nsPoint                        mMoveDelta; 
  PLArenaPool                    mPool;
  nsCOMPtr<nsISelection>         mBoundingSelection;
  nsAutoTArray<PresShellState,8> mPresShellStates;
  nsAutoTArray<nsIFrame*,100>    mFramesMarkedForDisplay;
  nsDisplayTableItem*            mCurrentTableItem;
  PRPackedBool                   mBuildCaret;
  PRPackedBool                   mEventDelivery;
  PRPackedBool                   mIsBackgroundOnly;
  PRPackedBool                   mIsAtRootOfPseudoStackingContext;
  PRPackedBool                   mPaintAllFrames;
  PRPackedBool                   mAccurateVisibleRegions;
  
  
  PRPackedBool                   mInTransform;
  PRPackedBool                   mSyncDecodeImages;
};

class nsDisplayItem;
class nsDisplayList;





class nsDisplayItemLink {
  
  
protected:
  nsDisplayItemLink() : mAbove(nsnull) {}
  nsDisplayItem* mAbove;  
  
  friend class nsDisplayList;
};
















class nsDisplayItem : public nsDisplayItemLink {
public:
  
  
  nsDisplayItem(nsIFrame* aFrame) : mFrame(aFrame) {}
  virtual ~nsDisplayItem() {}
  
  void* operator new(size_t aSize,
                     nsDisplayListBuilder* aBuilder) CPP_THROW_NEW {
    return aBuilder->Allocate(aSize);
  }

  



  enum Type {
    TYPE_GENERIC,

    TYPE_BORDER,
    TYPE_CLIP,
    TYPE_OPACITY,
    TYPE_OUTLINE,
    TYPE_PLUGIN,
#ifdef MOZ_SVG
    TYPE_SVG_EFFECTS,
#endif
    TYPE_TRANSFORM,
    TYPE_WRAPLIST
  };

  struct HitTestState {
    ~HitTestState() {
      NS_ASSERTION(mItemBuffer.Length() == 0,
                   "mItemBuffer should have been cleared");
    }
    nsAutoTArray<nsDisplayItem*, 100> mItemBuffer;
  };

  




  virtual Type GetType() { return TYPE_GENERIC; }
  








  virtual nsIFrame* HitTest(nsDisplayListBuilder* aBuilder, nsPoint aPt,
                            HitTestState* aState) { return nsnull; }
  





  inline nsIFrame* GetUnderlyingFrame() { return mFrame; }
  




  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder) {
    nsIFrame* f = GetUnderlyingFrame();
    return nsRect(aBuilder->ToReferenceFrame(f), f->GetSize());
  }
  



  virtual PRBool IsOpaque(nsDisplayListBuilder* aBuilder) { return PR_FALSE; }
  



  virtual PRBool IsUniform(nsDisplayListBuilder* aBuilder) { return PR_FALSE; }
  





  virtual PRBool IsVaryingRelativeToMovingFrame(nsDisplayListBuilder* aBuilder)
  { return PR_FALSE; }
  



  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
                     const nsRect& aDirtyRect) {}

  









  virtual PRBool OptimizeVisibility(nsDisplayListBuilder* aBuilder,
                                    nsRegion* aVisibleRegion);
  








  virtual PRBool TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem) {
    return PR_FALSE;
  }
  
  



  virtual nsDisplayList* GetList() { return nsnull; }
  
#ifdef NS_DEBUG
  


  virtual const char* Name() = 0;
#endif

  nsDisplayItem* GetAbove() { return mAbove; }
  
protected:
  friend class nsDisplayList;
  
  nsDisplayItem() {
    mAbove = nsnull;
  }
  
  nsIFrame* mFrame;
};
















class nsDisplayList {
public:
  


  nsDisplayList() { mTop = &mSentinel; mSentinel.mAbove = nsnull; }
  ~nsDisplayList() {
    if (mSentinel.mAbove) {
      NS_WARNING("Nonempty list left over?");
    }
    DeleteAll();
  }

  



  void AppendToTop(nsDisplayItem* aItem) {
    NS_ASSERTION(aItem, "No item to append!");
    NS_ASSERTION(!aItem->mAbove, "Already in a list!");
    mTop->mAbove = aItem;
    mTop = aItem;
  }
  
  



  nsresult AppendNewToTop(nsDisplayItem* aItem) {
    if (!aItem)
      return NS_ERROR_OUT_OF_MEMORY;
    AppendToTop(aItem);
    return NS_OK;
  }
  
  



  nsresult AppendNewToBottom(nsDisplayItem* aItem) {
    if (!aItem)
      return NS_ERROR_OUT_OF_MEMORY;
    AppendToBottom(aItem);
    return NS_OK;
  }
  
  



  void AppendToBottom(nsDisplayItem* aItem) {
    NS_ASSERTION(aItem, "No item to append!");
    NS_ASSERTION(!aItem->mAbove, "Already in a list!");
    aItem->mAbove = mSentinel.mAbove;
    mSentinel.mAbove = aItem;
    if (mTop == &mSentinel) {
      mTop = aItem;
    }
  }
  
  


  void AppendToTop(nsDisplayList* aList) {
    if (aList->mSentinel.mAbove) {
      mTop->mAbove = aList->mSentinel.mAbove;
      mTop = aList->mTop;
      aList->mTop = &aList->mSentinel;
      aList->mSentinel.mAbove = nsnull;
    }
  }
  
  


  void AppendToBottom(nsDisplayList* aList) {
    if (aList->mSentinel.mAbove) {
      aList->mTop->mAbove = mSentinel.mAbove;
      mTop = aList->mTop;
      mSentinel.mAbove = aList->mSentinel.mAbove;
           
      aList->mTop = &aList->mSentinel;
      aList->mSentinel.mAbove = nsnull;
    }
  }
  
  


  nsDisplayItem* RemoveBottom();
  
  


  void DeleteBottom();
  


  void DeleteAll();
  
  


  nsDisplayItem* GetTop() const {
    return mTop != &mSentinel ? static_cast<nsDisplayItem*>(mTop) : nsnull;
  }
  


  nsDisplayItem* GetBottom() const { return mSentinel.mAbove; }
  PRBool IsEmpty() const { return mTop == &mSentinel; }
  
  



  PRUint32 Count() const;
  








  void SortByZOrder(nsDisplayListBuilder* aBuilder, nsIContent* aCommonAncestor);
  







  void SortByContentOrder(nsDisplayListBuilder* aBuilder, nsIContent* aCommonAncestor);

  





  typedef PRBool (* SortLEQ)(nsDisplayItem* aItem1, nsDisplayItem* aItem2,
                             void* aClosure);
  void Sort(nsDisplayListBuilder* aBuilder, SortLEQ aCmp, void* aClosure);

  







  void OptimizeVisibility(nsDisplayListBuilder* aBuilder, nsRegion* aVisibleRegion);
  






  void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
             const nsRect& aDirtyRect) const;
  


  nsRect GetBounds(nsDisplayListBuilder* aBuilder) const;
  



  nsIFrame* HitTest(nsDisplayListBuilder* aBuilder, nsPoint aPt,
                    nsDisplayItem::HitTestState* aState) const;

private:
  
  
  void* operator new(size_t sz) CPP_THROW_NEW;
  
  
  void FlattenTo(nsTArray<nsDisplayItem*>* aElements);
  
  
  void ExplodeAnonymousChildLists(nsDisplayListBuilder* aBuilder);
  
  nsDisplayItemLink  mSentinel;
  nsDisplayItemLink* mTop;
};








class nsDisplayListSet {
public:
  



  nsDisplayList* BorderBackground() const { return mBorderBackground; }
  



  nsDisplayList* BlockBorderBackgrounds() const { return mBlockBorderBackgrounds; }
  



  nsDisplayList* Floats() const { return mFloats; }
  




  nsDisplayList* PositionedDescendants() const { return mPositioned; }
  



  nsDisplayList* Outlines() const { return mOutlines; }
  


  nsDisplayList* Content() const { return mContent; }
  
  nsDisplayListSet(nsDisplayList* aBorderBackground,
                   nsDisplayList* aBlockBorderBackgrounds,
                   nsDisplayList* aFloats,
                   nsDisplayList* aContent,
                   nsDisplayList* aPositionedDescendants,
                   nsDisplayList* aOutlines) :
     mBorderBackground(aBorderBackground),
     mBlockBorderBackgrounds(aBlockBorderBackgrounds),
     mFloats(aFloats),
     mContent(aContent),
     mPositioned(aPositionedDescendants),
     mOutlines(aOutlines) {
  }

  


  
  nsDisplayListSet(const nsDisplayListSet& aLists,
                   nsDisplayList* aBorderBackground) :
     mBorderBackground(aBorderBackground),
     mBlockBorderBackgrounds(aLists.BlockBorderBackgrounds()),
     mFloats(aLists.Floats()),
     mContent(aLists.Content()),
     mPositioned(aLists.PositionedDescendants()),
     mOutlines(aLists.Outlines()) {
  }
  
  



  void MoveTo(const nsDisplayListSet& aDestination) const;

private:
  
  
  void* operator new(size_t sz) CPP_THROW_NEW;

protected:
  nsDisplayList* mBorderBackground;
  nsDisplayList* mBlockBorderBackgrounds;
  nsDisplayList* mFloats;
  nsDisplayList* mContent;
  nsDisplayList* mPositioned;
  nsDisplayList* mOutlines;
};





struct nsDisplayListCollection : public nsDisplayListSet {
  nsDisplayListCollection() :
    nsDisplayListSet(&mLists[0], &mLists[1], &mLists[2], &mLists[3], &mLists[4],
                     &mLists[5]) {}
  nsDisplayListCollection(nsDisplayList* aBorderBackground) :
    nsDisplayListSet(aBorderBackground, &mLists[1], &mLists[2], &mLists[3], &mLists[4],
                     &mLists[5]) {}

  

                     
  void SortAllByContentOrder(nsDisplayListBuilder* aBuilder, nsIContent* aCommonAncestor) {
    for (PRInt32 i = 0; i < 6; ++i) {
      mLists[i].SortByContentOrder(aBuilder, aCommonAncestor);
    }
  }

private:
  
  
  void* operator new(size_t sz) CPP_THROW_NEW;

  nsDisplayList mLists[6];
};











class nsDisplayGeneric : public nsDisplayItem {
public:
  typedef void (* PaintCallback)(nsIFrame* aFrame, nsIRenderingContext* aCtx,
                                 const nsRect& aDirtyRect, nsPoint aFramePt);

  nsDisplayGeneric(nsIFrame* aFrame, PaintCallback aPaint, const char* aName)
    : nsDisplayItem(aFrame), mPaint(aPaint)
#ifdef DEBUG
      , mName(aName)
#endif
  {
    MOZ_COUNT_CTOR(nsDisplayGeneric);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayGeneric() {
    MOZ_COUNT_DTOR(nsDisplayGeneric);
  }
#endif
  
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect) {
    mPaint(mFrame, aCtx, aDirtyRect, aBuilder->ToReferenceFrame(mFrame));
  }
  NS_DISPLAY_DECL_NAME(mName)
protected:
  PaintCallback mPaint;
#ifdef DEBUG
  const char*   mName;
#endif
};

#if defined(MOZ_REFLOW_PERF_DSP) && defined(MOZ_REFLOW_PERF)












class nsDisplayReflowCount : public nsDisplayItem {
public:
  nsDisplayReflowCount(nsIFrame* aFrame, const char* aFrameName,
                       PRUint32 aColor = 0)
    : nsDisplayItem(aFrame),
      mFrameName(aFrameName),
      mColor(aColor)
  {
    MOZ_COUNT_CTOR(nsDisplayReflowCount);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayReflowCount() {
    MOZ_COUNT_DTOR(nsDisplayReflowCount);
  }
#endif
  
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect) {
    nsPoint pt = aBuilder->ToReferenceFrame(mFrame);
    nsIRenderingContext::AutoPushTranslation translate(aCtx, pt.x, pt.y);
    mFrame->PresContext()->PresShell()->PaintCount(mFrameName, aCtx,
                                                      mFrame->PresContext(),
                                                      mFrame, mColor);
  }
  NS_DISPLAY_DECL_NAME("nsDisplayReflowCount")
protected:
  const char* mFrameName;
  nscolor mColor;
};

#define DO_GLOBAL_REFLOW_COUNT_DSP(_name)                                     \
  PR_BEGIN_MACRO                                                              \
    if (!aBuilder->IsBackgroundOnly() && !aBuilder->IsForEventDelivery() &&   \
        PresContext()->PresShell()->IsPaintingFrameCounts()) {                \
      nsresult _rv =                                                          \
        aLists.Outlines()->AppendNewToTop(new (aBuilder)                      \
                                          nsDisplayReflowCount(this, _name)); \
      NS_ENSURE_SUCCESS(_rv, _rv);                                            \
    }                                                                         \
  PR_END_MACRO

#define DO_GLOBAL_REFLOW_COUNT_DSP_COLOR(_name, _color)                       \
  PR_BEGIN_MACRO                                                              \
    if (!aBuilder->IsBackgroundOnly() && !aBuilder->IsForEventDelivery() &&   \
        PresContext()->PresShell()->IsPaintingFrameCounts()) {                \
      nsresult _rv =                                                          \
        aLists.Outlines()->AppendNewToTop(new (aBuilder)                      \
                                          nsDisplayReflowCount(this, _name,   \
                                                               _color));      \
      NS_ENSURE_SUCCESS(_rv, _rv);                                            \
    }                                                                         \
  PR_END_MACRO




#define DECL_DO_GLOBAL_REFLOW_COUNT_DSP(_class, _super)                   \
  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,           \
                              const nsRect&           aDirtyRect,         \
                              const nsDisplayListSet& aLists) {           \
    DO_GLOBAL_REFLOW_COUNT_DSP(#_class);                                  \
    return _super::BuildDisplayList(aBuilder, aDirtyRect, aLists);        \
  }

#else 

#define DO_GLOBAL_REFLOW_COUNT_DSP(_name)
#define DO_GLOBAL_REFLOW_COUNT_DSP_COLOR(_name, _color)
#define DECL_DO_GLOBAL_REFLOW_COUNT_DSP(_class, _super)

#endif 

class nsDisplayCaret : public nsDisplayItem {
public:
  nsDisplayCaret(nsIFrame* aCaretFrame, nsCaret *aCaret)
    : nsDisplayItem(aCaretFrame), mCaret(aCaret) {
    MOZ_COUNT_CTOR(nsDisplayCaret);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayCaret() {
    MOZ_COUNT_DTOR(nsDisplayCaret);
  }
#endif

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder) {
    
    return mCaret->GetCaretRect() + aBuilder->ToReferenceFrame(mFrame);
  }
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
      const nsRect& aDirtyRect);
  NS_DISPLAY_DECL_NAME("Caret")
protected:
  nsRefPtr<nsCaret> mCaret;
};




class nsDisplayBorder : public nsDisplayItem {
public:
  nsDisplayBorder(nsIFrame* aFrame) : nsDisplayItem(aFrame) {
    MOZ_COUNT_CTOR(nsDisplayBorder);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayBorder() {
    MOZ_COUNT_DTOR(nsDisplayBorder);
  }
#endif

  virtual Type GetType() { return TYPE_BORDER; }
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect);
  virtual PRBool OptimizeVisibility(nsDisplayListBuilder* aBuilder, nsRegion* aVisibleRegion);
  NS_DISPLAY_DECL_NAME("Border")
};












class nsDisplaySolidColor : public nsDisplayItem {
public:
  nsDisplaySolidColor(nsIFrame* aFrame, const nsRect& aBounds, nscolor aColor)
    : nsDisplayItem(aFrame), mBounds(aBounds), mColor(aColor) {
    MOZ_COUNT_CTOR(nsDisplaySolidColor);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplaySolidColor() {
    MOZ_COUNT_DTOR(nsDisplaySolidColor);
  }
#endif

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder) { return mBounds; }

  virtual PRBool IsOpaque(nsDisplayListBuilder* aBuilder) {
    return (NS_GET_A(mColor) == 255);
  }

  virtual PRBool IsUniform(nsDisplayListBuilder* aBuilder) { return PR_TRUE; }

  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect);

  NS_DISPLAY_DECL_NAME("SolidColor")
private:
  nsRect  mBounds;
  nscolor mColor;
};




class nsDisplayBackground : public nsDisplayItem {
public:
  nsDisplayBackground(nsIFrame* aFrame) : nsDisplayItem(aFrame) {
    mIsThemed = mFrame->IsThemed();
    MOZ_COUNT_CTOR(nsDisplayBackground);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayBackground() {
    MOZ_COUNT_DTOR(nsDisplayBackground);
  }
#endif

  virtual nsIFrame* HitTest(nsDisplayListBuilder* aBuilder, nsPoint aPt,
                            HitTestState* aState) { return mFrame; }
  virtual PRBool IsOpaque(nsDisplayListBuilder* aBuilder);
  virtual PRBool IsVaryingRelativeToMovingFrame(nsDisplayListBuilder* aBuilder);
  virtual PRBool IsUniform(nsDisplayListBuilder* aBuilder);
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder);
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect);
  NS_DISPLAY_DECL_NAME("Background")
private:
    
    PRPackedBool mIsThemed;
};




class nsDisplayBoxShadowOuter : public nsDisplayItem {
public:
  nsDisplayBoxShadowOuter(nsIFrame* aFrame) : nsDisplayItem(aFrame) {
    MOZ_COUNT_CTOR(nsDisplayBoxShadowOuter);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayBoxShadowOuter() {
    MOZ_COUNT_DTOR(nsDisplayBoxShadowOuter);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect);
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder);
  virtual PRBool OptimizeVisibility(nsDisplayListBuilder* aBuilder, nsRegion* aVisibleRegion);
  NS_DISPLAY_DECL_NAME("BoxShadowOuter")
};




class nsDisplayBoxShadowInner : public nsDisplayItem {
public:
  nsDisplayBoxShadowInner(nsIFrame* aFrame) : nsDisplayItem(aFrame) {
    MOZ_COUNT_CTOR(nsDisplayBoxShadowInner);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayBoxShadowInner() {
    MOZ_COUNT_DTOR(nsDisplayBoxShadowInner);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect);
  NS_DISPLAY_DECL_NAME("BoxShadowInner")
};




class nsDisplayOutline : public nsDisplayItem {
public:
  nsDisplayOutline(nsIFrame* aFrame) : nsDisplayItem(aFrame) {
    MOZ_COUNT_CTOR(nsDisplayOutline);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayOutline() {
    MOZ_COUNT_DTOR(nsDisplayOutline);
  }
#endif

  virtual Type GetType() { return TYPE_OUTLINE; }
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder);
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect);
  virtual PRBool OptimizeVisibility(nsDisplayListBuilder* aBuilder, nsRegion* aVisibleRegion);
  NS_DISPLAY_DECL_NAME("Outline")
};




class nsDisplayEventReceiver : public nsDisplayItem {
public:
  nsDisplayEventReceiver(nsIFrame* aFrame) : nsDisplayItem(aFrame) {
    MOZ_COUNT_CTOR(nsDisplayEventReceiver);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayEventReceiver() {
    MOZ_COUNT_DTOR(nsDisplayEventReceiver);
  }
#endif

  virtual nsIFrame* HitTest(nsDisplayListBuilder* aBuilder, nsPoint aPt,
                            HitTestState* aState) { return mFrame; }
  NS_DISPLAY_DECL_NAME("EventReceiver")
};















class nsDisplayWrapList : public nsDisplayItem {
  
  

public:
  


  nsDisplayWrapList(nsIFrame* aFrame, nsDisplayList* aList);
  nsDisplayWrapList(nsIFrame* aFrame, nsDisplayItem* aItem);
  virtual ~nsDisplayWrapList();
  virtual Type GetType() { return TYPE_WRAPLIST; }
  virtual nsIFrame* HitTest(nsDisplayListBuilder* aBuilder, nsPoint aPt,
                            HitTestState* aState);
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder);
  virtual PRBool IsOpaque(nsDisplayListBuilder* aBuilder);
  virtual PRBool IsUniform(nsDisplayListBuilder* aBuilder);
  virtual PRBool IsVaryingRelativeToMovingFrame(nsDisplayListBuilder* aBuilder);
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect);
  virtual PRBool OptimizeVisibility(nsDisplayListBuilder* aBuilder,
                                    nsRegion* aVisibleRegion);
  virtual PRBool TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem) {
    NS_WARNING("This list should already have been flattened!!!");
    return PR_FALSE;
  }
  NS_DISPLAY_DECL_NAME("WrapList")
                                    
  virtual nsDisplayList* GetList() { return &mList; }
  
  





  virtual nsDisplayWrapList* WrapWithClone(nsDisplayListBuilder* aBuilder,
                                           nsDisplayItem* aItem) {
    NS_NOTREACHED("We never returned nsnull for GetUnderlyingFrame!");
    return nsnull;
  }

protected:
  nsDisplayWrapList() {}
  
  nsDisplayList mList;
};








class nsDisplayWrapper {
public:
  
  

  virtual PRBool WrapBorderBackground() { return PR_TRUE; }
  virtual nsDisplayItem* WrapList(nsDisplayListBuilder* aBuilder,
                                  nsIFrame* aFrame, nsDisplayList* aList) = 0;
  virtual nsDisplayItem* WrapItem(nsDisplayListBuilder* aBuilder,
                                  nsDisplayItem* aItem) = 0;

  nsresult WrapLists(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                     const nsDisplayListSet& aIn, const nsDisplayListSet& aOut);
  nsresult WrapListsInPlace(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                            const nsDisplayListSet& aLists);
protected:
  nsDisplayWrapper() {}
};
                              




class nsDisplayOpacity : public nsDisplayWrapList {
public:
  nsDisplayOpacity(nsIFrame* aFrame, nsDisplayList* aList);
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayOpacity();
#endif
  
  virtual Type GetType() { return TYPE_OPACITY; }
  virtual PRBool IsOpaque(nsDisplayListBuilder* aBuilder);
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect);
  virtual PRBool OptimizeVisibility(nsDisplayListBuilder* aBuilder,
                                    nsRegion* aVisibleRegion);  
  virtual PRBool TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem);
  NS_DISPLAY_DECL_NAME("Opacity")

private:
  




  PRPackedBool mNeedAlpha;
};






class nsDisplayClip : public nsDisplayWrapList {
public:
  




  nsDisplayClip(nsIFrame* aFrame, nsIFrame* aClippingFrame, 
                nsDisplayItem* aItem, const nsRect& aRect);
  nsDisplayClip(nsIFrame* aFrame, nsIFrame* aClippingFrame,
                nsDisplayList* aList, const nsRect& aRect);
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayClip();
#endif
  
  virtual Type GetType() { return TYPE_CLIP; }
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder);
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect);
  virtual PRBool OptimizeVisibility(nsDisplayListBuilder* aBuilder,
                                    nsRegion* aVisibleRegion);
  virtual PRBool TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem);
  NS_DISPLAY_DECL_NAME("Clip")
  
  nsRect GetClipRect() { return mClip; }
  void SetClipRect(const nsRect& aRect) { mClip = aRect; }
  nsIFrame* GetClippingFrame() { return mClippingFrame; }

  virtual nsDisplayWrapList* WrapWithClone(nsDisplayListBuilder* aBuilder,
                                           nsDisplayItem* aItem);

private:
  
  
  
  
  nsIFrame* mClippingFrame;
  nsRect    mClip;
};

#ifdef MOZ_SVG




class nsDisplaySVGEffects : public nsDisplayWrapList {
public:
  nsDisplaySVGEffects(nsIFrame* aFrame, nsDisplayList* aList);
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplaySVGEffects();
#endif
  
  virtual Type GetType() { return TYPE_SVG_EFFECTS; }
  virtual PRBool IsOpaque(nsDisplayListBuilder* aBuilder);
  virtual nsIFrame* HitTest(nsDisplayListBuilder* aBuilder, nsPoint aPt,
                            HitTestState* aState);
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder) {
    return mBounds + aBuilder->ToReferenceFrame(mEffectsFrame);
  }
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect);
  virtual PRBool OptimizeVisibility(nsDisplayListBuilder* aBuilder,
                                    nsRegion* aVisibleRegion);  
  virtual PRBool TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem);
  NS_DISPLAY_DECL_NAME("SVGEffects")

  nsIFrame* GetEffectsFrame() { return mEffectsFrame; }

private:
  nsIFrame* mEffectsFrame;
  
  nsRect    mBounds;
};
#endif






 
class nsDisplayTransform: public nsDisplayItem
{
public:
  


  nsDisplayTransform(nsIFrame *aFrame, nsDisplayList *aList) :
    nsDisplayItem(aFrame), mStoredList(aFrame, aList)
  {
    MOZ_COUNT_CTOR(nsDisplayTransform);
  }

#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayTransform()
  {
    MOZ_COUNT_DTOR(nsDisplayTransform);
  }
#endif

  NS_DISPLAY_DECL_NAME("nsDisplayTransform");

  virtual Type GetType() 
  {
    return TYPE_TRANSFORM;
  }

#ifdef NS_DEBUG
  nsDisplayWrapList* GetStoredList() { return &mStoredList; }
#endif

  virtual nsIFrame* HitTest(nsDisplayListBuilder *aBuilder, nsPoint aPt,
                            HitTestState *aState);
  virtual nsRect GetBounds(nsDisplayListBuilder *aBuilder);
  virtual PRBool IsOpaque(nsDisplayListBuilder *aBuilder);
  virtual PRBool IsUniform(nsDisplayListBuilder *aBuilder);
  virtual void   Paint(nsDisplayListBuilder *aBuilder,
                       nsIRenderingContext *aCtx,
                       const nsRect &aDirtyRect);
  virtual PRBool OptimizeVisibility(nsDisplayListBuilder *aBuilder,
                                    nsRegion *aVisibleRegion);
  virtual PRBool TryMerge(nsDisplayListBuilder *aBuilder, nsDisplayItem *aItem);

  

















  static nsRect TransformRect(const nsRect &aUntransformedBounds, 
                              const nsIFrame* aFrame,
                              const nsPoint &aOrigin,
                              const nsRect* aBoundsOverride = nsnull);

  


  static nsRect UntransformRect(const nsRect &aUntransformedBounds, 
                                const nsIFrame* aFrame,
                                const nsPoint &aOrigin);

  












  static nsRect GetFrameBoundsForTransform(const nsIFrame* aFrame);

  












  static gfxMatrix GetResultingTransformMatrix(const nsIFrame* aFrame,
                                               const nsPoint& aOrigin,
                                               float aFactor,
                                               const nsRect* aBoundsOverride = nsnull);

private:
  nsDisplayWrapList mStoredList;
};

#endif

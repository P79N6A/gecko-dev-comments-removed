






































#ifdef IBMBIDI

#ifndef nsBidiPresUtils_h___
#define nsBidiPresUtils_h___

#include "nsTArray.h"
#include "nsIFrame.h"
#include "nsBidi.h"
#include "nsBidiUtils.h"
#include "nsCOMPtr.h"
#include "nsDataHashtable.h"
#include "nsBlockFrame.h"
#include "nsTHashtable.h"

#ifdef DrawText
#undef DrawText
#endif






struct nsFrameContinuationState : public nsVoidPtrHashKey
{
  nsFrameContinuationState(const void *aFrame) : nsVoidPtrHashKey(aFrame) {}

  



  nsIFrame* mFirstVisualFrame;

  



  PRUint32 mFrameCount;

  



  PRPackedBool mHasContOnPrevLines;

  



  PRPackedBool mHasContOnNextLines;
};




typedef nsTHashtable<nsFrameContinuationState> nsContinuationStates;





struct nsBidiPositionResolve
{
  
  PRInt32 logicalIndex;
  
  
  PRInt32 visualIndex;
  
  
  
  PRInt32 visualLeftTwips;
  
  
  PRInt32 visualWidth;
};

class nsBidiPresUtils {
public:
  nsBidiPresUtils();
  ~nsBidiPresUtils();
  PRBool IsSuccessful(void) const;
  
  




  class BidiProcessor {
  public:
    virtual ~BidiProcessor() { }

    











    virtual void SetText(const PRUnichar*   aText,
                         PRInt32            aLength,
                         nsBidiDirection    aDirection) = 0;

    





    virtual nscoord GetWidth() = 0;

    








    virtual void DrawText(nscoord   aXOffset,
                          nscoord   aWidth) = 0;
  };

  







  nsresult Resolve(nsBlockFrame* aBlockFrame);

  





  void ReorderFrames(nsIFrame*            aFirstFrameOnLine,
                     PRInt32              aNumFramesOnLine);

  






  nsresult FormatUnicodeText(nsPresContext* aPresContext,
                             PRUnichar*      aText,
                             PRInt32&        aTextLength,
                             nsCharType      aCharType,
                             PRBool          aIsOddLevel);

  
















  nsresult RenderText(const PRUnichar*       aText,
                      PRInt32                aLength,
                      nsBidiDirection        aBaseDirection,
                      nsPresContext*         aPresContext,
                      nsRenderingContext&    aRenderingContext,
                      nsRenderingContext&    aTextRunConstructionContext,
                      nscoord                aX,
                      nscoord                aY,
                      nsBidiPositionResolve* aPosResolve = nsnull,
                      PRInt32                aPosResolveCount = 0)
  {
    return ProcessTextForRenderingContext(aText, aLength, aBaseDirection, aPresContext, aRenderingContext,
                                          aTextRunConstructionContext, MODE_DRAW, aX, aY, aPosResolve, aPosResolveCount, nsnull);
  }
  
  nscoord MeasureTextWidth(const PRUnichar*     aText,
                           PRInt32              aLength,
                           nsBidiDirection      aBaseDirection,
                           nsPresContext*       aPresContext,
                           nsRenderingContext&  aRenderingContext)
  {
    nscoord length;
    nsresult rv = ProcessTextForRenderingContext(aText, aLength, aBaseDirection, aPresContext,
                                                 aRenderingContext, aRenderingContext,
                                                 MODE_MEASURE, 0, 0, nsnull, 0, &length);
    return NS_SUCCEEDED(rv) ? length : 0;
  }

  







  PRBool CheckLineOrder(nsIFrame*  aFirstFrameOnLine,
                        PRInt32    aNumFramesOnLine,
                        nsIFrame** aLeftmost,
                        nsIFrame** aRightmost);

  






  nsIFrame* GetFrameToRightOf(const nsIFrame*  aFrame,
                              nsIFrame*        aFirstFrameOnLine,
                              PRInt32          aNumFramesOnLine);
    
  






  nsIFrame* GetFrameToLeftOf(const nsIFrame*  aFrame,
                             nsIFrame*        aFirstFrameOnLine,
                             PRInt32          aNumFramesOnLine);
    
  


  static nsBidiLevel GetFrameEmbeddingLevel(nsIFrame* aFrame);

  


  static nsBidiLevel GetFrameBaseLevel(nsIFrame* aFrame);

  enum Mode { MODE_DRAW, MODE_MEASURE };

  



















  nsresult ProcessText(const PRUnichar*       aText,
                       PRInt32                aLength,
                       nsBidiDirection        aBaseDirection,
                       nsPresContext*         aPresContext,
                       BidiProcessor&         aprocessor,
                       Mode                   aMode,
                       nsBidiPositionResolve* aPosResolve,
                       PRInt32                aPosResolveCount,
                       nscoord*               aWidth);

  












  void CopyLogicalToVisual(const nsAString& aSource,
                           nsAString& aDest,
                           nsBidiLevel aBaseDirection,
                           PRBool aOverride);

  



  PRUint32 EstimateMemoryUsed();

  void Traverse(nsCycleCollectionTraversalCallback &cb) const;
  void Unlink();

private:
  nsresult ProcessTextForRenderingContext(const PRUnichar*       aText,
                                          PRInt32                aLength,
                                          nsBidiDirection        aBaseDirection,
                                          nsPresContext*         aPresContext,
                                          nsRenderingContext&    aRenderingContext,
                                          nsRenderingContext&    aTextRunConstructionContext,
                                          Mode                   aMode,
                                          nscoord                aX, 
                                          nscoord                aY, 
                                          nsBidiPositionResolve* aPosResolve,  
                                          PRInt32                aPosResolveCount,
                                          nscoord*               aWidth );

  




  void InitLogicalArray(nsIFrame* aCurrentFrame);

  



  void InitLogicalArrayFromLine(nsIFrame* aFirstFrameOnLine,
                                PRInt32   aNumFramesOnLine);

  







  nsresult Reorder(PRBool& aReordered, PRBool& aHasRTLFrames);
  
  












  void RepositionFrame(nsIFrame*              aFrame,
                       PRBool                 aIsOddLevel,
                       nscoord&               aLeft,
                       nsContinuationStates*  aContinuationStates) const;

  







  void InitContinuationStates(nsIFrame*              aFrame,
                              nsContinuationStates*  aContinuationStates) const;

  














   void IsLeftOrRightMost(nsIFrame*              aFrame,
                          nsContinuationStates*  aContinuationStates,
                          PRBool&                aIsLeftMost ,
                          PRBool&                aIsRightMost ) const;

  






  void RepositionInlineFrames(nsIFrame* aFirstChild) const;
  
  














  inline
  void EnsureBidiContinuation(nsIFrame*       aFrame,
                              nsIFrame**      aNewFrame,
                              PRInt32&        aFrameIndex,
                              PRInt32         aStart,
                              PRInt32         aEnd);

  















  void RemoveBidiContinuation(nsIFrame*       aFrame,
                              PRInt32         aFirstIndex,
                              PRInt32         aLastIndex,
                              PRInt32&        aOffset) const;
  void CalculateCharType(PRInt32& aOffset,
                         PRInt32  aCharTypeLimit,
                         PRInt32& aRunLimit,
                         PRInt32& aRunLength,
                         PRInt32& aRunCount,
                         PRUint8& aCharType,
                         PRUint8& aPrevCharType) const;
  
  void StripBidiControlCharacters(PRUnichar* aText,
                                  PRInt32&   aTextLength) const;

  static PRBool WriteLogicalToVisual(const PRUnichar* aSrc,
                                     PRUint32 aSrcLength,
                                     PRUnichar* aDest,
                                     nsBidiLevel aBaseDirection,
                                     nsBidi* aBidiEngine);

 static void WriteReverse(const PRUnichar* aSrc,
                          PRUint32 aSrcLength,
                          PRUnichar* aDest);

  nsAutoString    mBuffer;
  nsTArray<nsIFrame*> mLogicalFrames;
  nsTArray<nsIFrame*> mVisualFrames;
  nsDataHashtable<nsISupportsHashKey, PRInt32> mContentToFrameIndex;
  PRInt32         mArraySize;
  PRInt32*        mIndexMap;
  PRUint8*        mLevels;
  nsresult        mSuccess;
  nsIContent*     mPrevContent;

  nsBidi*         mBidiEngine;
};

#endif 

#endif 

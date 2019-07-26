





#ifdef IBMBIDI

#ifndef nsBidiPresUtils_h___
#define nsBidiPresUtils_h___

#include "nsTArray.h"
#include "nsBidi.h"
#include "nsBidiUtils.h"
#include "nsCOMPtr.h"
#include "nsDataHashtable.h"
#include "nsBlockFrame.h"
#include "nsTHashtable.h"

#ifdef DrawText
#undef DrawText
#endif

struct BidiParagraphData;
struct BidiLineData;
class nsIFrame;






struct nsFrameContinuationState : public nsVoidPtrHashKey
{
  nsFrameContinuationState(const void *aFrame) : nsVoidPtrHashKey(aFrame) {}

  



  nsIFrame* mFirstVisualFrame;

  



  PRUint32 mFrameCount;

  



  bool mHasContOnPrevLines;

  



  bool mHasContOnNextLines;
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

  







  static nsresult Resolve(nsBlockFrame* aBlockFrame);
  static nsresult ResolveParagraph(nsBlockFrame* aBlockFrame,
                                   BidiParagraphData* aBpd);
  static void ResolveParagraphWithinBlock(nsBlockFrame* aBlockFrame,
                                          BidiParagraphData* aBpd);

  





  static void ReorderFrames(nsIFrame*            aFirstFrameOnLine,
                            PRInt32              aNumFramesOnLine);

  






  static nsresult FormatUnicodeText(nsPresContext* aPresContext,
                                    PRUnichar*      aText,
                                    PRInt32&        aTextLength,
                                    nsCharType      aCharType,
                                    bool            aIsOddLevel);

  
















  static nsresult RenderText(const PRUnichar*       aText,
                             PRInt32                aLength,
                             nsBidiDirection        aBaseDirection,
                             nsPresContext*         aPresContext,
                             nsRenderingContext&    aRenderingContext,
                             nsRenderingContext&    aTextRunConstructionContext,
                             nscoord                aX,
                             nscoord                aY,
                             nsBidiPositionResolve* aPosResolve = nullptr,
                             PRInt32                aPosResolveCount = 0)
  {
    return ProcessTextForRenderingContext(aText, aLength, aBaseDirection, aPresContext, aRenderingContext,
                                          aTextRunConstructionContext, MODE_DRAW, aX, aY, aPosResolve, aPosResolveCount, nullptr);
  }
  
  static nscoord MeasureTextWidth(const PRUnichar*     aText,
                                  PRInt32              aLength,
                                  nsBidiDirection      aBaseDirection,
                                  nsPresContext*       aPresContext,
                                  nsRenderingContext&  aRenderingContext)
  {
    nscoord length;
    nsresult rv = ProcessTextForRenderingContext(aText, aLength, aBaseDirection, aPresContext,
                                                 aRenderingContext, aRenderingContext,
                                                 MODE_MEASURE, 0, 0, nullptr, 0, &length);
    return NS_SUCCEEDED(rv) ? length : 0;
  }

  







  static bool CheckLineOrder(nsIFrame*  aFirstFrameOnLine,
                               PRInt32    aNumFramesOnLine,
                               nsIFrame** aLeftmost,
                               nsIFrame** aRightmost);

  






  static nsIFrame* GetFrameToRightOf(const nsIFrame*  aFrame,
                                     nsIFrame*        aFirstFrameOnLine,
                                     PRInt32          aNumFramesOnLine);
    
  






  static nsIFrame* GetFrameToLeftOf(const nsIFrame*  aFrame,
                                    nsIFrame*        aFirstFrameOnLine,
                                    PRInt32          aNumFramesOnLine);

  static nsIFrame* GetFirstLeaf(nsIFrame* aFrame);
    
  


  static nsBidiLevel GetFrameEmbeddingLevel(nsIFrame* aFrame);
    
  


  static PRUint8 GetParagraphDepth(nsIFrame* aFrame);

  


  static nsBidiLevel GetFrameBaseLevel(nsIFrame* aFrame);

  enum Mode { MODE_DRAW, MODE_MEASURE };

  



















  static nsresult ProcessText(const PRUnichar*       aText,
                              PRInt32                aLength,
                              nsBidiDirection        aBaseDirection,
                              nsPresContext*         aPresContext,
                              BidiProcessor&         aprocessor,
                              Mode                   aMode,
                              nsBidiPositionResolve* aPosResolve,
                              PRInt32                aPosResolveCount,
                              nscoord*               aWidth,
                              nsBidi*                aBidiEngine);

  












  static void CopyLogicalToVisual(const nsAString& aSource,
                                  nsAString& aDest,
                                  nsBidiLevel aBaseDirection,
                                  bool aOverride);

private:
  static nsresult
  ProcessTextForRenderingContext(const PRUnichar*       aText,
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

  







  static void TraverseFrames(nsBlockFrame*              aBlockFrame,
                             nsBlockInFlowLineIterator* aLineIter,
                             nsIFrame*                  aCurrentFrame,
                             BidiParagraphData*         aBpd);
  
  












  static void RepositionFrame(nsIFrame*              aFrame,
                              bool                   aIsOddLevel,
                              nscoord&               aLeft,
                              nsContinuationStates*  aContinuationStates);

  







  static void InitContinuationStates(nsIFrame*              aFrame,
                                     nsContinuationStates*  aContinuationStates);

  














   static void IsLeftOrRightMost(nsIFrame*              aFrame,
                                 nsContinuationStates*  aContinuationStates,
                                 bool&                aIsLeftMost ,
                                 bool&                aIsRightMost );

  






  static void RepositionInlineFrames(BidiLineData* aBld,
                                     nsIFrame* aFirstChild);
  
  














  static inline
  nsresult EnsureBidiContinuation(nsIFrame*       aFrame,
                                  nsIFrame**      aNewFrame,
                                  PRInt32&        aFrameIndex,
                                  PRInt32         aStart,
                                  PRInt32         aEnd);

  















  static void RemoveBidiContinuation(BidiParagraphData* aBpd,
                                     nsIFrame*          aFrame,
                                     PRInt32            aFirstIndex,
                                     PRInt32            aLastIndex,
                                     PRInt32&           aOffset);
  static void CalculateCharType(nsBidi*          aBidiEngine,
                                const PRUnichar* aText,
                                PRInt32&         aOffset,
                                PRInt32          aCharTypeLimit,
                                PRInt32&         aRunLimit,
                                PRInt32&         aRunLength,
                                PRInt32&         aRunCount,
                                PRUint8&         aCharType,
                                PRUint8&         aPrevCharType);
  
  static void StripBidiControlCharacters(PRUnichar* aText,
                                         PRInt32&   aTextLength);

  static bool WriteLogicalToVisual(const PRUnichar* aSrc,
                                     PRUint32 aSrcLength,
                                     PRUnichar* aDest,
                                     nsBidiLevel aBaseDirection,
                                     nsBidi* aBidiEngine);

  static void WriteReverse(const PRUnichar* aSrc,
                           PRUint32 aSrcLength,
                           PRUnichar* aDest);
};

#endif 

#endif 

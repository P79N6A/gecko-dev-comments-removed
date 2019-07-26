





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

  



  uint32_t mFrameCount;

  



  bool mHasContOnPrevLines;

  



  bool mHasContOnNextLines;
};




typedef nsTHashtable<nsFrameContinuationState> nsContinuationStates;





struct nsBidiPositionResolve
{
  
  int32_t logicalIndex;
  
  
  int32_t visualIndex;
  
  
  
  int32_t visualLeftTwips;
  
  
  int32_t visualWidth;
};

class nsBidiPresUtils {
public:
  nsBidiPresUtils();
  ~nsBidiPresUtils();
  
  




  class BidiProcessor {
  public:
    virtual ~BidiProcessor() { }

    











    virtual void SetText(const PRUnichar*   aText,
                         int32_t            aLength,
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
                            int32_t              aNumFramesOnLine);

  






  static nsresult FormatUnicodeText(nsPresContext* aPresContext,
                                    PRUnichar*      aText,
                                    int32_t&        aTextLength,
                                    nsCharType      aCharType,
                                    bool            aIsOddLevel);

  
















  static nsresult RenderText(const PRUnichar*       aText,
                             int32_t                aLength,
                             nsBidiDirection        aBaseDirection,
                             nsPresContext*         aPresContext,
                             nsRenderingContext&    aRenderingContext,
                             nsRenderingContext&    aTextRunConstructionContext,
                             nscoord                aX,
                             nscoord                aY,
                             nsBidiPositionResolve* aPosResolve = nullptr,
                             int32_t                aPosResolveCount = 0)
  {
    return ProcessTextForRenderingContext(aText, aLength, aBaseDirection, aPresContext, aRenderingContext,
                                          aTextRunConstructionContext, MODE_DRAW, aX, aY, aPosResolve, aPosResolveCount, nullptr);
  }
  
  static nscoord MeasureTextWidth(const PRUnichar*     aText,
                                  int32_t              aLength,
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
                               int32_t    aNumFramesOnLine,
                               nsIFrame** aLeftmost,
                               nsIFrame** aRightmost);

  






  static nsIFrame* GetFrameToRightOf(const nsIFrame*  aFrame,
                                     nsIFrame*        aFirstFrameOnLine,
                                     int32_t          aNumFramesOnLine);
    
  






  static nsIFrame* GetFrameToLeftOf(const nsIFrame*  aFrame,
                                    nsIFrame*        aFirstFrameOnLine,
                                    int32_t          aNumFramesOnLine);

  static nsIFrame* GetFirstLeaf(nsIFrame* aFrame);
    
  


  static nsBidiLevel GetFrameEmbeddingLevel(nsIFrame* aFrame);
    
  


  static uint8_t GetParagraphDepth(nsIFrame* aFrame);

  


  static nsBidiLevel GetFrameBaseLevel(nsIFrame* aFrame);

  enum Mode { MODE_DRAW, MODE_MEASURE };

  



















  static nsresult ProcessText(const PRUnichar*       aText,
                              int32_t                aLength,
                              nsBidiDirection        aBaseDirection,
                              nsPresContext*         aPresContext,
                              BidiProcessor&         aprocessor,
                              Mode                   aMode,
                              nsBidiPositionResolve* aPosResolve,
                              int32_t                aPosResolveCount,
                              nscoord*               aWidth,
                              nsBidi*                aBidiEngine);

  












  static void CopyLogicalToVisual(const nsAString& aSource,
                                  nsAString& aDest,
                                  nsBidiLevel aBaseDirection,
                                  bool aOverride);

private:
  static nsresult
  ProcessTextForRenderingContext(const PRUnichar*       aText,
                                 int32_t                aLength,
                                 nsBidiDirection        aBaseDirection,
                                 nsPresContext*         aPresContext,
                                 nsRenderingContext&    aRenderingContext,
                                 nsRenderingContext&    aTextRunConstructionContext,
                                 Mode                   aMode,
                                 nscoord                aX, 
                                 nscoord                aY, 
                                 nsBidiPositionResolve* aPosResolve,  
                                 int32_t                aPosResolveCount,
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
                                  int32_t&        aFrameIndex,
                                  int32_t         aStart,
                                  int32_t         aEnd);

  















  static void RemoveBidiContinuation(BidiParagraphData* aBpd,
                                     nsIFrame*          aFrame,
                                     int32_t            aFirstIndex,
                                     int32_t            aLastIndex,
                                     int32_t&           aOffset);
  static void CalculateCharType(nsBidi*          aBidiEngine,
                                const PRUnichar* aText,
                                int32_t&         aOffset,
                                int32_t          aCharTypeLimit,
                                int32_t&         aRunLimit,
                                int32_t&         aRunLength,
                                int32_t&         aRunCount,
                                uint8_t&         aCharType,
                                uint8_t&         aPrevCharType);
  
  static void StripBidiControlCharacters(PRUnichar* aText,
                                         int32_t&   aTextLength);

  static bool WriteLogicalToVisual(const PRUnichar* aSrc,
                                     uint32_t aSrcLength,
                                     PRUnichar* aDest,
                                     nsBidiLevel aBaseDirection,
                                     nsBidi* aBidiEngine);

  static void WriteReverse(const PRUnichar* aSrc,
                           uint32_t aSrcLength,
                           PRUnichar* aDest);
};

#endif 

#endif 







#ifndef nsBidiPresUtils_h___
#define nsBidiPresUtils_h___

#ifdef IBMBIDI

#include "nsBidi.h"
#include "nsBidiUtils.h"
#include "nsHashKeys.h"
#include "nsCoord.h"

#ifdef DrawText
#undef DrawText
#endif

struct BidiParagraphData;
struct BidiLineData;
class nsIFrame;
class nsBlockFrame;
class nsPresContext;
class nsRenderingContext;
class nsBlockInFlowLineIterator;
class nsStyleContext;
template<class T> class nsTHashtable;
namespace mozilla { class WritingMode; }






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

    











    virtual void SetText(const char16_t*   aText,
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
                            int32_t              aNumFramesOnLine,
                            mozilla::WritingMode aLineWM,
                            nscoord&             aLineWidth);

  






  static nsresult FormatUnicodeText(nsPresContext* aPresContext,
                                    char16_t*      aText,
                                    int32_t&        aTextLength,
                                    nsCharType      aCharType,
                                    bool            aIsOddLevel);

  























  static nsresult RenderText(const char16_t*       aText,
                             int32_t                aLength,
                             nsBidiLevel            aBaseLevel,
                             nsPresContext*         aPresContext,
                             nsRenderingContext&    aRenderingContext,
                             nsRenderingContext&    aTextRunConstructionContext,
                             nscoord                aX,
                             nscoord                aY,
                             nsBidiPositionResolve* aPosResolve = nullptr,
                             int32_t                aPosResolveCount = 0)
  {
    return ProcessTextForRenderingContext(aText, aLength, aBaseLevel, aPresContext, aRenderingContext,
                                          aTextRunConstructionContext, MODE_DRAW, aX, aY, aPosResolve, aPosResolveCount, nullptr);
  }
  
  static nscoord MeasureTextWidth(const char16_t*     aText,
                                  int32_t              aLength,
                                  nsBidiLevel          aBaseLevel,
                                  nsPresContext*       aPresContext,
                                  nsRenderingContext&  aRenderingContext)
  {
    nscoord length;
    nsresult rv = ProcessTextForRenderingContext(aText, aLength, aBaseLevel, aPresContext,
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

  


























  static nsresult ProcessText(const char16_t*       aText,
                              int32_t                aLength,
                              nsBidiLevel            aBaseLevel,
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

  











  static nsBidiLevel BidiLevelFromStyle(nsStyleContext* aStyleContext);

private:
  static nsresult
  ProcessTextForRenderingContext(const char16_t*       aText,
                                 int32_t                aLength,
                                 nsBidiLevel            aBaseLevel,
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
                              bool                   aIsEvenLevel,
                              nscoord&               aStart,
                              nsContinuationStates*  aContinuationStates,
                              mozilla::WritingMode   aLineWM,
                              nscoord&               aLineWidth);

  







  static void InitContinuationStates(nsIFrame*              aFrame,
                                     nsContinuationStates*  aContinuationStates);

  














   static void IsFirstOrLast(nsIFrame*              aFrame,
                             nsContinuationStates*  aContinuationStates,
                             bool&                  aIsFirst ,
                             bool&                  aIsLast );

  






  static void RepositionInlineFrames(BidiLineData* aBld,
                                     nsIFrame* aFirstChild,
                                     mozilla::WritingMode aLineWM,
                                     nscoord& aLineWidth);
  
  














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
                                const char16_t* aText,
                                int32_t&         aOffset,
                                int32_t          aCharTypeLimit,
                                int32_t&         aRunLimit,
                                int32_t&         aRunLength,
                                int32_t&         aRunCount,
                                uint8_t&         aCharType,
                                uint8_t&         aPrevCharType);
  
  static void StripBidiControlCharacters(char16_t* aText,
                                         int32_t&   aTextLength);

  static bool WriteLogicalToVisual(const char16_t* aSrc,
                                     uint32_t aSrcLength,
                                     char16_t* aDest,
                                     nsBidiLevel aBaseDirection,
                                     nsBidi* aBidiEngine);

  static void WriteReverse(const char16_t* aSrc,
                           uint32_t aSrcLength,
                           char16_t* aDest);
};

#endif 

#endif 

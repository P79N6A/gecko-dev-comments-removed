




#ifndef nsIMathMLFrame_h___
#define nsIMathMLFrame_h___

#include "nsQueryFrame.h"
#include "nsMathMLOperators.h"

struct nsPresentationData;
struct nsEmbellishData;
struct nsHTMLReflowMetrics;
class nsRenderingContext;
class nsIFrame;



enum eMathMLFrameType {
  eMathMLFrameType_UNKNOWN = -1,
  eMathMLFrameType_Ordinary,
  eMathMLFrameType_OperatorOrdinary,
  eMathMLFrameType_OperatorInvisible,
  eMathMLFrameType_OperatorUserDefined,
  eMathMLFrameType_Inner,
  eMathMLFrameType_ItalicIdentifier,
  eMathMLFrameType_UprightIdentifier,
  eMathMLFrameType_COUNT
};


class nsIMathMLFrame
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsIMathMLFrame)

  
  virtual bool IsSpaceLike() = 0;

 
 
 
 







  NS_IMETHOD
  GetBoundingMetrics(nsBoundingMetrics& aBoundingMetrics) = 0;

  NS_IMETHOD
  SetBoundingMetrics(const nsBoundingMetrics& aBoundingMetrics) = 0;

  NS_IMETHOD
  SetReference(const nsPoint& aReference) = 0;

  virtual eMathMLFrameType GetMathMLFrameType() = 0;

 
 

 
















  NS_IMETHOD 
  Stretch(nsRenderingContext& aRenderingContext,
          nsStretchDirection   aStretchDirection,
          nsBoundingMetrics&   aContainerSize,
          nsHTMLReflowMetrics& aDesiredStretchSize) = 0;

 
 
  NS_IMETHOD
  GetEmbellishData(nsEmbellishData& aEmbellishData) = 0;


 
 

 

  NS_IMETHOD
  GetPresentationData(nsPresentationData& aPresentationData) = 0;

  




































  NS_IMETHOD
  InheritAutomaticData(nsIFrame* aParent) = 0;

  NS_IMETHOD
  TransmitAutomaticData() = 0;

 



















  NS_IMETHOD
  UpdatePresentationData(uint32_t        aFlagsValues,
                         uint32_t        aWhichFlags) = 0;

 




















  NS_IMETHOD
  UpdatePresentationDataFromChildAt(int32_t         aFirstIndex,
                                    int32_t         aLastIndex,
                                    uint32_t        aFlagsValues,
                                    uint32_t        aWhichFlags) = 0;

  
  
  
  
  virtual uint8_t
  ScriptIncrement(nsIFrame* aFrame) = 0;

  
  
  
  
  virtual bool
  IsMrowLike() = 0;
};






struct nsEmbellishData {
  
  uint32_t flags;

  
  nsIFrame* coreFrame;

  
  nsStretchDirection direction;

  
  
  
  
  nscoord leadingSpace;
  nscoord trailingSpace;

  nsEmbellishData() {
    flags = 0;
    coreFrame = nullptr;
    direction = NS_STRETCH_DIRECTION_UNSUPPORTED;
    leadingSpace = 0;
    trailingSpace = 0;
  }
};









struct nsPresentationData {
  
  uint32_t flags;

  
  
  
  nsIFrame* baseFrame;

  nsPresentationData() {
    flags = 0;
    baseFrame = nullptr;
  }
};







#define NS_MATHML_COMPRESSED                          0x00000002U





#define NS_MATHML_STRETCH_ALL_CHILDREN_VERTICALLY     0x00000004U





#define NS_MATHML_STRETCH_ALL_CHILDREN_HORIZONTALLY   0x00000008U


#define NS_MATHML_SPACE_LIKE                          0x00000040U




#define NS_MATHML_ERROR                               0x80000000U


#define NS_MATHML_STRETCH_DONE                        0x20000000U





#define NS_MATHML_SHOW_BOUNDING_METRICS               0x10000000U



#define NS_MATHML_IS_COMPRESSED(_flags) \
  (NS_MATHML_COMPRESSED == ((_flags) & NS_MATHML_COMPRESSED))

#define NS_MATHML_WILL_STRETCH_ALL_CHILDREN_VERTICALLY(_flags) \
  (NS_MATHML_STRETCH_ALL_CHILDREN_VERTICALLY == ((_flags) & NS_MATHML_STRETCH_ALL_CHILDREN_VERTICALLY))

#define NS_MATHML_WILL_STRETCH_ALL_CHILDREN_HORIZONTALLY(_flags) \
  (NS_MATHML_STRETCH_ALL_CHILDREN_HORIZONTALLY == ((_flags) & NS_MATHML_STRETCH_ALL_CHILDREN_HORIZONTALLY))

#define NS_MATHML_IS_SPACE_LIKE(_flags) \
  (NS_MATHML_SPACE_LIKE == ((_flags) & NS_MATHML_SPACE_LIKE))

#define NS_MATHML_HAS_ERROR(_flags) \
  (NS_MATHML_ERROR == ((_flags) & NS_MATHML_ERROR))

#define NS_MATHML_STRETCH_WAS_DONE(_flags) \
  (NS_MATHML_STRETCH_DONE == ((_flags) & NS_MATHML_STRETCH_DONE))

#define NS_MATHML_PAINT_BOUNDING_METRICS(_flags) \
  (NS_MATHML_SHOW_BOUNDING_METRICS == ((_flags) & NS_MATHML_SHOW_BOUNDING_METRICS))






#define NS_MATHML_EMBELLISH_OPERATOR                0x00000001



#define NS_MATHML_EMBELLISH_MOVABLELIMITS           0x00000002



#define NS_MATHML_EMBELLISH_ACCENT                  0x00000004



#define NS_MATHML_EMBELLISH_ACCENTOVER              0x00000008



#define NS_MATHML_EMBELLISH_ACCENTUNDER             0x00000010



#define NS_MATHML_IS_EMBELLISH_OPERATOR(_flags) \
  (NS_MATHML_EMBELLISH_OPERATOR == ((_flags) & NS_MATHML_EMBELLISH_OPERATOR))

#define NS_MATHML_EMBELLISH_IS_MOVABLELIMITS(_flags) \
  (NS_MATHML_EMBELLISH_MOVABLELIMITS == ((_flags) & NS_MATHML_EMBELLISH_MOVABLELIMITS))

#define NS_MATHML_EMBELLISH_IS_ACCENT(_flags) \
  (NS_MATHML_EMBELLISH_ACCENT == ((_flags) & NS_MATHML_EMBELLISH_ACCENT))

#define NS_MATHML_EMBELLISH_IS_ACCENTOVER(_flags) \
  (NS_MATHML_EMBELLISH_ACCENTOVER == ((_flags) & NS_MATHML_EMBELLISH_ACCENTOVER))

#define NS_MATHML_EMBELLISH_IS_ACCENTUNDER(_flags) \
  (NS_MATHML_EMBELLISH_ACCENTUNDER == ((_flags) & NS_MATHML_EMBELLISH_ACCENTUNDER))

#endif 

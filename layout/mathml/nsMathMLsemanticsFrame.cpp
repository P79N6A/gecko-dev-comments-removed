





#include "nsMathMLsemanticsFrame.h"
#include "nsMimeTypes.h"





nsIFrame*
NS_NewMathMLsemanticsFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLsemanticsFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLsemanticsFrame)

nsMathMLsemanticsFrame::~nsMathMLsemanticsFrame()
{
}

nsIFrame* 
nsMathMLsemanticsFrame::GetSelectedFrame()
{
  
  nsIFrame* childFrame = mFrames.FirstChild(); 
  mSelectedFrame = childFrame;

  
  if (!childFrame) {
    mInvalidMarkup = true;
    return mSelectedFrame;
  }
  mInvalidMarkup = false;

  
  
  bool firstChildIsAnnotation = false;
  nsIContent* childContent = childFrame->GetContent();
  if (childContent->GetNameSpaceID() == kNameSpaceID_MathML &&
      (childContent->Tag() == nsGkAtoms::annotation_ ||
       childContent->Tag() == nsGkAtoms::annotation_xml_)) {
    firstChildIsAnnotation = true;
  }

  
  
  if (!firstChildIsAnnotation &&
      childFrame->IsFrameOfType(nsIFrame::eMathML)) {
    nsIMathMLFrame* mathMLFrame = do_QueryFrame(childFrame);
    if (mathMLFrame) {
      TransmitAutomaticData();
      return mSelectedFrame;
    }
    
    childFrame = childFrame->GetNextSibling();
  }

  
  
  
  for ( ; childFrame; childFrame = childFrame->GetNextSibling()) {
    nsIContent* childContent = childFrame->GetContent();

    if (childContent->GetNameSpaceID() != kNameSpaceID_MathML) continue;

    if (childContent->Tag() == nsGkAtoms::annotation_) {

      
      
      
      if (childContent->HasAttr(kNameSpaceID_None, nsGkAtoms::src)) continue;

      
      
      mSelectedFrame = childFrame;
      break;
    }

    if (childContent->Tag() == nsGkAtoms::annotation_xml_) {

      
      if (childContent->HasAttr(kNameSpaceID_None, nsGkAtoms::src)) continue;

      
      
      
      
      
      
      
      
      
      
      
      
      
      
      nsAutoString value;
      childContent->GetAttr(kNameSpaceID_None, nsGkAtoms::encoding, value);
      if (value.EqualsLiteral("application/mathml-presentation+xml") ||
          value.EqualsLiteral("MathML-Presentation") ||
          value.EqualsLiteral(IMAGE_SVG_XML) ||
          value.EqualsLiteral("SVG1.1") ||
          value.EqualsLiteral(APPLICATION_XHTML_XML) ||
          value.EqualsLiteral(TEXT_HTML)) {
        mSelectedFrame = childFrame;
        break;
      }
    }
  }

  TransmitAutomaticData();
  return mSelectedFrame;
}

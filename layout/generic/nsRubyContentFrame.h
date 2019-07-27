







#ifndef nsRubyContentFrame_h___
#define nsRubyContentFrame_h___

#include "nsInlineFrame.h"

typedef nsInlineFrame nsRubyContentFrameSuper;

class nsRubyContentFrame : public nsRubyContentFrameSuper
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  
  virtual bool IsFrameOfType(uint32_t aFlags) const override;

  
  
  
  
  
  bool IsIntraLevelWhitespace() const;

protected:
  explicit nsRubyContentFrame(nsStyleContext* aContext)
    : nsRubyContentFrameSuper(aContext) {}
};

#endif 

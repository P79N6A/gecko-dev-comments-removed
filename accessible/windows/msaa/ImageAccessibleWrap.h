






#ifndef mozilla_a11y_ImageAccessibleWrap_h__
#define mozilla_a11y_ImageAccessibleWrap_h__

#include "ImageAccessible.h"
#include "ia2AccessibleImage.h"

namespace mozilla {
namespace a11y {

class ImageAccessibleWrap : public ImageAccessible,
                            public ia2AccessibleImage
{
public:
  ImageAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc) :
    ImageAccessible(aContent, aDoc) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};

} 
} 

#endif


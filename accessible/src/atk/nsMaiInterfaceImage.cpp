





#include "InterfaceInitFuncs.h"

#include "AccessibleWrap.h"
#include "ImageAccessible.h"
#include "nsMai.h"

using namespace mozilla;
using namespace mozilla::a11y;

extern "C" {
const gchar* getDescriptionCB(AtkObject* aAtkObj);

static void
getImagePositionCB(AtkImage* aImage, gint* aAccX, gint* aAccY,
                   AtkCoordType aCoordType)
{
  AccessibleWrap* accWrap = GetAccessibleWrap(ATK_OBJECT(aImage));
  if (!accWrap || !accWrap->IsImage())
    return;

  ImageAccessible* image = accWrap->AsImage();
  uint32_t geckoCoordType = (aCoordType == ATK_XY_WINDOW) ?
    nsIAccessibleCoordinateType::COORDTYPE_WINDOW_RELATIVE :
    nsIAccessibleCoordinateType::COORDTYPE_SCREEN_RELATIVE;
  
  image->GetImagePosition(geckoCoordType, aAccX, aAccY);
}

static const gchar*
getImageDescriptionCB(AtkImage* aImage)
{
  return getDescriptionCB(ATK_OBJECT(aImage));
}

static void
getImageSizeCB(AtkImage* aImage, gint* aAccWidth, gint* aAccHeight)
{
  AccessibleWrap* accWrap = GetAccessibleWrap(ATK_OBJECT(aImage));
  if (!accWrap || !accWrap->IsImage())
    return;

  accWrap->AsImage()->GetImageSize(aAccWidth, aAccHeight);
}
}

void
imageInterfaceInitCB(AtkImageIface* aIface)
{
  NS_ASSERTION(aIface, "no interface!");
  if (NS_UNLIKELY(!aIface))
    return;

  aIface->get_image_position = getImagePositionCB;
  aIface->get_image_description = getImageDescriptionCB;
  aIface->get_image_size = getImageSizeCB;
}







#include "InterfaceInitFuncs.h"

#include "AccessibleWrap.h"
#include "nsHTMLImageAccessible.h"
#include "nsMai.h"

extern "C" {
const gchar* getDescriptionCB(AtkObject* aAtkObj);

static void
getImagePositionCB(AtkImage* aImage, gint* aAccX, gint* aAccY,
                   AtkCoordType aCoordType)
{
  AccessibleWrap* accWrap = GetAccessibleWrap(ATK_OBJECT(aImage));
  if (!accWrap || !accWrap->IsImage())
    return;

  nsHTMLImageAccessible* image = accWrap->AsImage();
  PRUint32 geckoCoordType = (aCoordType == ATK_XY_WINDOW) ?
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

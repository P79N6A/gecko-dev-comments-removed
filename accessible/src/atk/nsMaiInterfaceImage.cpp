






































#include "nsAccessibleWrap.h"
#include "nsMaiInterfaceImage.h"

extern "C" const gchar* getDescriptionCB(AtkObject* aAtkObj);

void
imageInterfaceInitCB(AtkImageIface *aIface)
{
    g_return_if_fail(aIface != NULL);

    aIface->get_image_position = getImagePositionCB;
    aIface->get_image_description = getImageDescriptionCB;
    aIface->get_image_size = getImageSizeCB;

}

void
getImagePositionCB(AtkImage *aImage, gint *aAccX, gint *aAccY,
                   AtkCoordType aCoordType)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aImage));
    if (!accWrap) 
      return;

    nsCOMPtr<nsIAccessibleImage> image;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleImage),
                            getter_AddRefs(image));
    if (!image)
      return;

    PRUint32 geckoCoordType = (aCoordType == ATK_XY_WINDOW) ?
      nsIAccessibleCoordinateType::COORDTYPE_WINDOW_RELATIVE :
      nsIAccessibleCoordinateType::COORDTYPE_SCREEN_RELATIVE;

    
    image->GetImagePosition(geckoCoordType, aAccX, aAccY);
}

const gchar *
getImageDescriptionCB(AtkImage *aImage)
{
   return getDescriptionCB(ATK_OBJECT(aImage));
}

void
getImageSizeCB(AtkImage *aImage, gint *aAccWidth, gint *aAccHeight)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aImage));
    if (!accWrap) 
      return;

    nsCOMPtr<nsIAccessibleImage> image;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleImage),
                            getter_AddRefs(image));
    if (!image)
      return;

    image->GetImageSize(aAccWidth, aAccHeight);
}

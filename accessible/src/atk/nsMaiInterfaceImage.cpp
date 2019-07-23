







































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

    PRInt32 width, height; 
    image->GetImageBounds(aAccX, aAccY, &width, &height);
    
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

    PRInt32 x,y; 
    image->GetImageBounds(&x, &y, aAccWidth, aAccHeight);
}

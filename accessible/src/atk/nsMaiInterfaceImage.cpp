







































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
    
    nsresult rv = image->GetImageBounds(aAccX, aAccY, &width, &height);
    if (NS_FAILED(rv))
      return;
    
    if (aCoordType == ATK_XY_WINDOW) {
        nsCOMPtr<nsIDOMNode> domNode;
        accWrap->GetDOMNode(getter_AddRefs(domNode));
        nsIntPoint winCoords = nsAccUtils::GetScreenCoordsForWindow(domNode);
        *aAccX -= winCoords.x;
        *aAccY -= winCoords.y;
    }
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

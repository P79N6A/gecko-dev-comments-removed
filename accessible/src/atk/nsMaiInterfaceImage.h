







































#ifndef __MAI_INTERFACE_IMAGE_H__
#define __MAI_INTERFACE_IMAGE_H__

#include "nsMai.h"
#include "nsIAccessibleImage.h"

G_BEGIN_DECLS

void imageInterfaceInitCB(AtkImageIface *aIface);


void getImagePositionCB(AtkImage *aImage,
                        gint *aAccX,
                        gint *aAccY,
                        AtkCoordType aCoordType);
const gchar* getImageDescriptionCB(AtkImage *aImage);
void getImageSizeCB(AtkImage *aImage,
                    gint *aAccWidth,
                    gint *aAccHeight);

G_END_DECLS

#endif 

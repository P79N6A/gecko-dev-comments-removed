







#include "GrTextureUnitObj.h"
#include "GrTextureObj.h"

void GrTextureUnitObj::setTexture(GrTextureObj *texture)  {

    if (fTexture) {
        GrAlwaysAssert(fTexture->getBound(this));
        fTexture->resetBound(this);

        GrAlwaysAssert(!fTexture->getDeleted());
        fTexture->unref();
    }

    fTexture = texture;

    if (fTexture) {
        GrAlwaysAssert(!fTexture->getDeleted());
        fTexture->ref();

        GrAlwaysAssert(!fTexture->getBound(this));
        fTexture->setBound(this);
    }
}









#include "SkBitmap.h"
#include "SkRegion.h"

bool SkBitmap::scrollRect(const SkIRect* subset, int dx, int dy,
                          SkRegion* inval) const
{
    if (NULL != subset) {
        SkBitmap tmp;

        return  this->extractSubset(&tmp, *subset) &&
                
                tmp.scrollRect(NULL, dx, dy, inval);
    }

    int shift;

    switch (this->config()) {
    case kIndex8_Config:
    case kA8_Config:
        shift = 0;
        break;
    case kARGB_4444_Config:
    case kRGB_565_Config:
        shift = 1;
        break;
    case kARGB_8888_Config:
        shift = 2;
        break;
    default:
        
        return false;
    }

    int width = this->width();
    int height = this->height();    
    
    
    if ((dx | dy) == 0 || width <= 0 || height <= 0) {
        if (NULL != inval) {
            inval->setEmpty();
        }
        return true;
    }

    
    if (NULL != inval) {
        SkIRect r;
        
        r.set(0, 0, width, height);
        
        inval->setRect(r);
        
        r.offset(dx, dy);
        
        
        if (!SkIRect::Intersects(r, inval->getBounds())) {
            
            return true;
        }
        
        
        inval->op(r, SkRegion::kDifference_Op);
    }
    
    SkAutoLockPixels    alp(*this);
    
    
    if (this->getPixels() == NULL) {
        return true;
    }

    char*       dst = (char*)this->getPixels();
    const char* src = dst;
    int         rowBytes = this->rowBytes();    

    if (dy <= 0) {
        src -= dy * rowBytes;
        height += dy;
    } else {
        dst += dy * rowBytes;
        height -= dy;
        
        src += (height - 1) * rowBytes;
        dst += (height - 1) * rowBytes;
        
        rowBytes = -rowBytes;
    }

    if (dx <= 0) {
        src -= dx << shift;
        width += dx;
    } else {
        dst += dx << shift;
        width -= dx;
    }

    
    
    if (width <= 0) {
        return true;
    }

    width <<= shift;    
    while (--height >= 0) {
        memmove(dst, src, width);
        dst += rowBytes;
        src += rowBytes;
    }
    return true;
}

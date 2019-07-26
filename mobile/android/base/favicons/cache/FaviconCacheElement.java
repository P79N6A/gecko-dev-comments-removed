



package org.mozilla.gecko.favicons.cache;

import android.graphics.Bitmap;





public class FaviconCacheElement implements Comparable<FaviconCacheElement> {
    
    final boolean isPrimary;

    
    Bitmap faviconPayload;

    
    
    
    
    
    
    
    volatile boolean invalidated;

    final int imageSize;

    
    final FaviconsForURL backpointer;

    public FaviconCacheElement(Bitmap payload, boolean primary, int size, FaviconsForURL backpointer) {
        this.faviconPayload = payload;
        this.isPrimary = primary;
        this.imageSize = size;
        this.backpointer = backpointer;
    }

    public FaviconCacheElement(Bitmap faviconPayload, boolean isPrimary, FaviconsForURL backpointer) {
        this.faviconPayload = faviconPayload;
        this.isPrimary = isPrimary;
        this.backpointer = backpointer;

        if (faviconPayload != null) {
            imageSize = faviconPayload.getWidth();
        } else {
            imageSize = 0;
        }
    }

    public int sizeOf() {
        if (invalidated) {
            return 0;
        }
        return faviconPayload.getRowBytes() * faviconPayload.getHeight();
    }

    








    @Override
    public int compareTo(FaviconCacheElement another) {
        if (invalidated && !another.invalidated) {
            return -1;
        }

        if (!invalidated && another.invalidated) {
            return 1;
        }

        if (invalidated) {
            return 0;
        }

        final int w1 = imageSize;
        final int w2 = another.imageSize;
        if (w1 > w2) {
            return 1;
        } else if (w2 > w1) {
            return -1;
        }
        return 0;
    }

    




    public void onEvictedFromCache() {
        if (isPrimary) {
            
            
            
            
            
            invalidated = true;
            faviconPayload = null;
        } else {
            
            if (backpointer == null) {
                return;
            }
            backpointer.favicons.remove(this);
        }
    }
}

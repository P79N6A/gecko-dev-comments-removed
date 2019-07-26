



package org.mozilla.gecko.favicons.cache;

import android.graphics.Bitmap;





public class FaviconCacheElement implements Comparable<FaviconCacheElement> {
    
    final boolean mIsPrimary;

    
    Bitmap mFaviconPayload;

    
    
    
    
    
    
    
    volatile boolean mInvalidated;

    final int mImageSize;

    
    final FaviconsForURL mBackpointer;

    public FaviconCacheElement(Bitmap payload, boolean isPrimary, int imageSize, FaviconsForURL backpointer) {
        mFaviconPayload = payload;
        mIsPrimary = isPrimary;
        mImageSize = imageSize;
        mBackpointer = backpointer;
    }

    public FaviconCacheElement(Bitmap payload, boolean isPrimary, FaviconsForURL backpointer) {
        mFaviconPayload = payload;
        mIsPrimary = isPrimary;
        mBackpointer = backpointer;

        if (payload != null) {
            mImageSize = payload.getWidth();
        } else {
            mImageSize = 0;
        }
    }

    public int sizeOf() {
        if (mInvalidated) {
            return 0;
        }
        return mFaviconPayload.getRowBytes() * mFaviconPayload.getHeight();
    }

    








    @Override
    public int compareTo(FaviconCacheElement another) {
        if (mInvalidated && !another.mInvalidated) {
            return -1;
        }

        if (!mInvalidated && another.mInvalidated) {
            return 1;
        }

        if (mInvalidated) {
            return 0;
        }

        final int w1 = mImageSize;
        final int w2 = another.mImageSize;
        if (w1 > w2) {
            return 1;
        } else if (w2 > w1) {
            return -1;
        }
        return 0;
    }

    




    public void onEvictedFromCache() {
        if (mIsPrimary) {
            
            
            
            
            
            mInvalidated = true;
            mFaviconPayload = null;
        } else {
            
            if (mBackpointer == null) {
                return;
            }
            mBackpointer.mFavicons.remove(this);
        }
    }
}





package org.mozilla.gecko.favicons.cache;

import android.graphics.Bitmap;
import android.util.Log;
import org.mozilla.gecko.gfx.BitmapUtils;

import java.util.ArrayList;
import java.util.Collections;

public class FaviconsForURL {
    private static final String LOGTAG = "FaviconForURL";

    private volatile int mDominantColor = -1;

    final long mDownloadTimestamp;
    final ArrayList<FaviconCacheElement> mFavicons;

    public final boolean mHasFailed;

    public FaviconsForURL(int size) {
        this(size, false);
    }

    public FaviconsForURL(int size, boolean hasFailed) {
        mHasFailed = hasFailed;
        mDownloadTimestamp = System.currentTimeMillis();
        mFavicons = new ArrayList<FaviconCacheElement>(size);
    }

    public FaviconCacheElement addSecondary(Bitmap favicon, int imageSize) {
        return addInternal(favicon, false, imageSize);
    }

    public FaviconCacheElement addPrimary(Bitmap favicon) {
        return addInternal(favicon, true, favicon.getWidth());
    }

    private FaviconCacheElement addInternal(Bitmap favicon, boolean isPrimary, int imageSize) {
        FaviconCacheElement c = new FaviconCacheElement(favicon, isPrimary, imageSize, this);

        int index = Collections.binarySearch(mFavicons, c);
        if (index < 0) {
            index = 0;
        }
        mFavicons.add(index, c);

        return c;
    }

    






    public int getNextHighestIndex(int targetSize) {
        
        FaviconCacheElement dummy = new FaviconCacheElement(null, false, targetSize, null);

        int index = Collections.binarySearch(mFavicons, dummy);

        
        
        
        if (index < 0) {
            index++;
            index = -index;
        }

        

        
        
        
        
        if (index == mFavicons.size()) {
            index = -1;
        }

        return index;
    }

    











    public FaviconCacheElement getNextPrimary(final int fromIndex) {
        final int numIcons = mFavicons.size();

        int searchIndex = fromIndex;
        while (searchIndex < numIcons) {
            FaviconCacheElement element = mFavicons.get(searchIndex);

            if (element.mIsPrimary) {
                if (element.mInvalidated) {
                    
                    break;
                }
                return element;
            }
            searchIndex++;
        }

        
        searchIndex = fromIndex - 1;
        while (searchIndex >= 0) {
            FaviconCacheElement element = mFavicons.get(searchIndex);

            if (element.mIsPrimary) {
                if (element.mInvalidated) {
                    return null;
                }
                return element;
            }
            searchIndex--;
        }

        Log.e(LOGTAG, "No primaries found in Favicon cache structure. This is madness!");

        return null;
    }

    


    public int ensureDominantColor() {
        if (mDominantColor == -1) {
            
            for (FaviconCacheElement element : mFavicons) {
                if (!element.mInvalidated) {
                    mDominantColor = BitmapUtils.getDominantColor(element.mFaviconPayload);
                    return mDominantColor;
                }
            }
            mDominantColor = 0xFFFFFF;
        }

        return mDominantColor;
    }
}

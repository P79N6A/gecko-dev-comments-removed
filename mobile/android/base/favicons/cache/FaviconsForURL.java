



package org.mozilla.gecko.favicons.cache;

import android.graphics.Bitmap;
import android.util.Log;
import org.mozilla.gecko.gfx.BitmapUtils;

import java.util.ArrayList;
import java.util.Collections;

public class FaviconsForURL {
    private static final String LOGTAG = "FaviconForURL";

    private volatile int dominantColor = -1;

    final long downloadTimestamp;
    final ArrayList<FaviconCacheElement> favicons;

    public final boolean hasFailed;

    public FaviconsForURL(int size) {
        this(size, false);
    }

    public FaviconsForURL(int size, boolean failed) {
        hasFailed = failed;
        downloadTimestamp = System.currentTimeMillis();
        favicons = new ArrayList<FaviconCacheElement>(size);
    }

    public FaviconCacheElement addSecondary(Bitmap favicon, int imageSize) {
        return addInternal(favicon, false, imageSize);
    }

    public FaviconCacheElement addPrimary(Bitmap favicon) {
        return addInternal(favicon, true, favicon.getWidth());
    }

    private FaviconCacheElement addInternal(Bitmap favicon, boolean isPrimary, int imageSize) {
        FaviconCacheElement c = new FaviconCacheElement(favicon, isPrimary, imageSize, this);

        int index = Collections.binarySearch(favicons, c);

        
        
        if (index >= 0) {
            return favicons.get(index);
        }

        
        
        index++;
        index = -index;
        favicons.add(index, c);

        return c;
    }

    






    public int getNextHighestIndex(int targetSize) {
        
        FaviconCacheElement dummy = new FaviconCacheElement(null, false, targetSize, null);

        int index = Collections.binarySearch(favicons, dummy);

        
        
        
        if (index < 0) {
            index++;
            index = -index;
        }

        

        
        
        
        
        if (index == favicons.size()) {
            index = -1;
        }

        return index;
    }

    











    public FaviconCacheElement getNextPrimary(final int fromIndex) {
        final int numIcons = favicons.size();

        int searchIndex = fromIndex;
        while (searchIndex < numIcons) {
            FaviconCacheElement element = favicons.get(searchIndex);

            if (element.isPrimary) {
                if (element.invalidated) {
                    
                    
                    
                    
                    return null;
                }
                return element;
            }
            searchIndex++;
        }

        
        searchIndex = fromIndex - 1;
        while (searchIndex >= 0) {
            FaviconCacheElement element = favicons.get(searchIndex);

            if (element.isPrimary) {
                if (element.invalidated) {
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
        if (dominantColor == -1) {
            
            for (FaviconCacheElement element : favicons) {
                if (!element.invalidated) {
                    dominantColor = BitmapUtils.getDominantColor(element.faviconPayload);
                    return dominantColor;
                }
            }
            dominantColor = 0xFFFFFF;
        }

        return dominantColor;
    }
}

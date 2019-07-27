



package org.mozilla.gecko.favicons.cache;

import android.graphics.Bitmap;
import android.util.Log;
import org.mozilla.gecko.favicons.Favicons;

import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.locks.ReentrantReadWriteLock;














































































public class FaviconCache {
    private static final String LOGTAG = "FaviconCache";

    
    private static final int NUM_FAVICON_SIZES = 4;

    
    public final int maxCachedWidth;

    
    public static final long FAILURE_RETRY_MILLISECONDS = 1000 * 60 * 60 * 4;

    
    
    
    
    private final HashMap<String, FaviconsForURL> backingMap = new HashMap<String, FaviconsForURL>();

    
    private final HashMap<String, FaviconsForURL> permanentBackingMap = new HashMap<String, FaviconsForURL>();

    
    
    
    
    private final LinkedList<FaviconCacheElement> ordering = new LinkedList<FaviconCacheElement>();

    
    
    
    

    
    private final AtomicInteger currentSize = new AtomicInteger(0);

    
    private final int maxSizeBytes;

    
    
    private final Object reorderingLock = new Object();

    
    
    private final ReentrantReadWriteLock backingMapsLock = new ReentrantReadWriteLock(false);

    


    private void startRead() {
        backingMapsLock.readLock().lock();
    }

    


    private void finishRead() {
        backingMapsLock.readLock().unlock();
    }

    


    private void startWrite() {
        backingMapsLock.writeLock().lock();
    }

    


    private void finishWrite() {
        backingMapsLock.writeLock().unlock();
    }

    public FaviconCache(int maxSize, int maxWidthToCache) {
        maxSizeBytes = maxSize;
        maxCachedWidth = maxWidthToCache;
    }

    






    public boolean isFailedFavicon(String faviconURL) {
        if (faviconURL == null) {
            return true;
        }

        startRead();

        try {
            
            
            
            if (!backingMap.containsKey(faviconURL)) {
                return false;
            }

            FaviconsForURL container = backingMap.get(faviconURL);

            
            if (!container.hasFailed) {
                return false;
            }

            final long failureTimestamp = container.downloadTimestamp;

            
            final long failureDiff = System.currentTimeMillis() - failureTimestamp;

            
            if (failureDiff < FAILURE_RETRY_MILLISECONDS) {
                return true;
            }
        } catch (Exception unhandled) {
            Log.e(LOGTAG, "FaviconCache exception!", unhandled);
            return true;
        }  finally {
            finishRead();
        }

        startWrite();

        
        try {
            recordRemoved(backingMap.remove(faviconURL));
            return false;
        } finally {
            finishWrite();
        }
    }

    




    public void putFailed(String faviconURL) {
        startWrite();

        try {
            FaviconsForURL container = new FaviconsForURL(0, true);
            recordRemoved(backingMap.put(faviconURL, container));
        } finally {
            finishWrite();
        }
    }

    










    public Bitmap getFaviconForDimensions(String faviconURL, int targetSize) {
        if (faviconURL == null) {
            Log.e(LOGTAG, "You passed a null faviconURL to getFaviconForDimensions. Don't.");
            return null;
        }

        boolean shouldComputeColour = false;
        boolean wasPermanent = false;
        FaviconsForURL container;
        final Bitmap newBitmap;

        startRead();

        try {
            container = permanentBackingMap.get(faviconURL);
            if (container == null) {
                container = backingMap.get(faviconURL);
                if (container == null) {
                    
                    return null;
                }
            } else {
                wasPermanent = true;
            }

            FaviconCacheElement cacheElement;

            
            int cacheElementIndex = (targetSize == -1) ? -1 : container.getNextHighestIndex(targetSize);

            
            
            if (cacheElementIndex != -1) {
                
                cacheElement = container.favicons.get(cacheElementIndex);

                if (cacheElement.invalidated) {
                    return null;
                }

                
                if (cacheElement.imageSize == targetSize) {
                    setMostRecentlyUsedWithinRead(cacheElement);
                    return cacheElement.faviconPayload;
                }
            } else {
                
                
                cacheElementIndex = container.favicons.size();
            }

            
            
            

            
            cacheElement = container.getNextPrimary(cacheElementIndex);

            if (cacheElement == null) {
                
                return null;
            }

            if (targetSize == -1) {
                
                return cacheElement.faviconPayload;
            }

            
            Bitmap largestElementBitmap = cacheElement.faviconPayload;
            int largestSize = cacheElement.imageSize;

            if (largestSize >= targetSize) {
                
                newBitmap = Bitmap.createScaledBitmap(largestElementBitmap, targetSize, targetSize, true);
            } else {
                
                
                largestSize *= 2;

                if (largestSize >= targetSize) {
                    
                    newBitmap = Bitmap.createScaledBitmap(largestElementBitmap, targetSize, targetSize, true);
                } else {
                    
                    newBitmap = Bitmap.createScaledBitmap(largestElementBitmap, largestSize, largestSize, true);

                    shouldComputeColour = true;
                }
            }
        } catch (Exception unhandled) {
            

            
            Log.e(LOGTAG, "FaviconCache exception!", unhandled);
            return null;
        } finally {
            finishRead();
        }

        startWrite();
        try {
            if (shouldComputeColour) {
                
                container.ensureDominantColor();
            }

            
            
            
            
            FaviconCacheElement newElement = container.addSecondary(newBitmap, targetSize);

            if (!wasPermanent) {
                if (setMostRecentlyUsedWithinWrite(newElement)) {
                    currentSize.addAndGet(newElement.sizeOf());
                }
            }
        } finally {
            finishWrite();
        }

        return newBitmap;
    }

    





    public int getDominantColor(String key) {
        startRead();

        try {
            FaviconsForURL element = permanentBackingMap.get(key);
            if (element == null) {
                element = backingMap.get(key);
            }

            if (element == null) {
                Log.w(LOGTAG, "Cannot compute dominant color of non-cached favicon. Cache fullness " +
                              currentSize.get() + '/' + maxSizeBytes);
                return 0xFFFFFF;
            }

            return element.ensureDominantColor();
        } finally {
            finishRead();
        }
    }

    





    private void recordRemoved(FaviconsForURL wasRemoved) {
        
        if (wasRemoved == null) {
            return;
        }

        int sizeRemoved = 0;

        for (FaviconCacheElement e : wasRemoved.favicons) {
            sizeRemoved += e.sizeOf();
            ordering.remove(e);
        }

        currentSize.addAndGet(-sizeRemoved);
    }

    private Bitmap produceCacheableBitmap(Bitmap favicon) {
        
        if (favicon == Favicons.defaultFavicon || favicon == null) {
            return null;
        }

        
        
        
        if (favicon.getWidth() > maxCachedWidth) {
            return Bitmap.createScaledBitmap(favicon, maxCachedWidth, maxCachedWidth, true);
        }

        return favicon;
    }

    






    private boolean setMostRecentlyUsedWithinRead(FaviconCacheElement element) {
        synchronized(reorderingLock) {
            boolean contained = ordering.remove(element);
            ordering.offer(element);
            return contained;
        }
    }

    






    private boolean setMostRecentlyUsedWithinWrite(FaviconCacheElement element) {
        boolean contained = ordering.remove(element);
        ordering.offer(element);
        return contained;
    }

    






    public void putSingleFavicon(String faviconURL, Bitmap aFavicon) {
        Bitmap favicon = produceCacheableBitmap(aFavicon);
        if (favicon == null) {
            return;
        }

        
        
        
        
        FaviconsForURL toInsert = new FaviconsForURL(NUM_FAVICON_SIZES);

        
        FaviconCacheElement newElement = toInsert.addPrimary(favicon);

        startWrite();
        try {
            
            setMostRecentlyUsedWithinWrite(newElement);

            currentSize.addAndGet(newElement.sizeOf());

            
            FaviconsForURL wasRemoved;
            wasRemoved = backingMap.put(faviconURL, toInsert);

            recordRemoved(wasRemoved);
        } finally {
            finishWrite();
        }

        cullIfRequired();
    }

    






    public void putFavicons(String faviconURL, Iterator<Bitmap> favicons, boolean permanently) {
        
        FaviconsForURL toInsert = new FaviconsForURL(5 * NUM_FAVICON_SIZES);
        int sizeGained = 0;

        while (favicons.hasNext()) {
            Bitmap favicon = produceCacheableBitmap(favicons.next());
            if (favicon == null) {
                continue;
            }

            FaviconCacheElement newElement = toInsert.addPrimary(favicon);
            sizeGained += newElement.sizeOf();
        }

        startWrite();
        try {
            if (permanently) {
                permanentBackingMap.put(faviconURL, toInsert);
                return;
            }

            for (FaviconCacheElement newElement : toInsert.favicons) {
                setMostRecentlyUsedWithinWrite(newElement);
            }

            
            
            currentSize.addAndGet(sizeGained);

            
            recordRemoved(backingMap.put(faviconURL, toInsert));
        } finally {
            finishWrite();
        }

        cullIfRequired();
    }

    



    private void cullIfRequired() {
        Log.d(LOGTAG, "Favicon cache fullness: " + currentSize.get() + '/' + maxSizeBytes);

        if (currentSize.get() <= maxSizeBytes) {
            return;
        }

        startWrite();
        try {
            while (currentSize.get() > maxSizeBytes) {
                

                FaviconCacheElement victim;
                victim = ordering.poll();

                currentSize.addAndGet(-victim.sizeOf());
                victim.onEvictedFromCache();

                Log.d(LOGTAG, "After cull: " + currentSize.get() + '/' + maxSizeBytes);
            }
        } finally {
            finishWrite();
        }
    }

    


    public void evictAll() {
        startWrite();

        
        try {
            currentSize.set(0);
            backingMap.clear();
            ordering.clear();

        } finally {
            finishWrite();
        }
    }
}

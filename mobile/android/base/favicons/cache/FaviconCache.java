



package org.mozilla.gecko.favicons.cache;

import android.graphics.Bitmap;
import android.util.Log;
import org.mozilla.gecko.favicons.Favicons;

import java.util.Iterator;
import java.util.LinkedList;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.Semaphore;
import java.util.concurrent.atomic.AtomicInteger;














































































public class FaviconCache {
    private static final String LOGTAG = "FaviconCache";

    
    private static final int NUM_FAVICON_SIZES = 4;

    
    public final int mMaxCachedWidth;

    
    public static final long FAILURE_RETRY_MILLISECONDS = 1000 * 60 * 20;

    
    
    
    
    private final ConcurrentHashMap<String, FaviconsForURL> mBackingMap = new ConcurrentHashMap<String, FaviconsForURL>();

    
    private final ConcurrentHashMap<String, FaviconsForURL> mPermanentBackingMap = new ConcurrentHashMap<String, FaviconsForURL>();

    
    
    
    
    private final LinkedList<FaviconCacheElement> mOrdering = new LinkedList<FaviconCacheElement>();

    
    
    
    

    
    private final AtomicInteger mCurrentSize = new AtomicInteger(0);

    
    private final int mMaxSizeBytes;

    
    
    private final AtomicInteger mOngoingReads = new AtomicInteger(0);

    
    
    private final Semaphore mTurnSemaphore = new Semaphore(1);

    
    
    
    private final Semaphore mReorderingSemaphore = new Semaphore(1);

    
    private final Semaphore mWriteLock = new Semaphore(1);

    



    private void startRead() {
        mTurnSemaphore.acquireUninterruptibly();
        mTurnSemaphore.release();

        if (mOngoingReads.incrementAndGet() == 1) {
            
            mWriteLock.acquireUninterruptibly();
        }
    }

    



    private void finishRead() {
        if (mOngoingReads.decrementAndGet() == 0) {
            mWriteLock.release();
        }
    }

    



    private void startWrite() {
        mTurnSemaphore.acquireUninterruptibly();
        mWriteLock.acquireUninterruptibly();
    }

    


    private void finishWrite() {
        mTurnSemaphore.release();
        mWriteLock.release();
    }

    public FaviconCache(int maxSize, int maxWidthToCache) {
        mMaxSizeBytes = maxSize;
        mMaxCachedWidth = maxWidthToCache;
    }

    






    public boolean isFailedFavicon(String faviconURL) {
        if (faviconURL == null) {
            return true;
        }

        startRead();

        try {
            
            
            
            if (!mBackingMap.containsKey(faviconURL)) {
                return false;
            }

            FaviconsForURL container = mBackingMap.get(faviconURL);

            
            if (!container.mHasFailed) {
                return false;
            }

            final long failureTimestamp = container.mDownloadTimestamp;

            
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
            recordRemoved(mBackingMap.remove(faviconURL));
            return false;
        } finally {
            finishWrite();
        }
    }

    




    public void putFailed(String faviconURL) {
        startWrite();

        try {
            FaviconsForURL container = new FaviconsForURL(0, true);
            recordRemoved(mBackingMap.put(faviconURL, container));
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
            container = mPermanentBackingMap.get(faviconURL);
            if (container == null) {
                container = mBackingMap.get(faviconURL);
                if (container == null) {
                    
                    return null;
                }
            } else {
                wasPermanent = true;
            }

            FaviconCacheElement cacheElement;

            
            int cacheElementIndex = (targetSize == -1) ? -1 : container.getNextHighestIndex(targetSize);

            
            
            if (cacheElementIndex != -1) {
                
                cacheElement = container.mFavicons.get(cacheElementIndex);

                if (cacheElement.mInvalidated) {
                    return null;
                }

                
                if (cacheElement.mImageSize == targetSize) {
                    setMostRecentlyUsedWithinRead(cacheElement);
                    return cacheElement.mFaviconPayload;
                }
            } else {
                
                
                cacheElementIndex = container.mFavicons.size();
            }

            
            
            

            
            cacheElement = container.getNextPrimary(cacheElementIndex);

            if (cacheElement == null) {
                
                return null;
            }

            if (targetSize == -1) {
                
                return cacheElement.mFaviconPayload;
            }

            
            Bitmap largestElementBitmap = cacheElement.mFaviconPayload;
            int largestSize = cacheElement.mImageSize;

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
                    mCurrentSize.addAndGet(newElement.sizeOf());
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
            FaviconsForURL element = mPermanentBackingMap.get(key);
            if (element == null) {
                element = mBackingMap.get(key);
            }

            if (element == null) {
                Log.w(LOGTAG, "Cannot compute dominant color of non-cached favicon. Cache fullness " +
                              mCurrentSize.get() + '/' + mMaxSizeBytes);
                finishRead();
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

        for (FaviconCacheElement e : wasRemoved.mFavicons) {
            sizeRemoved += e.sizeOf();
            mOrdering.remove(e);
        }

        mCurrentSize.addAndGet(-sizeRemoved);
    }

    private Bitmap produceCacheableBitmap(Bitmap favicon) {
        
        if (favicon == Favicons.sDefaultFavicon || favicon == null) {
            return null;
        }

        
        
        
        if (favicon.getWidth() > mMaxCachedWidth) {
            return Bitmap.createScaledBitmap(favicon, mMaxCachedWidth, mMaxCachedWidth, true);
        }

        return favicon;
    }

    






    private boolean setMostRecentlyUsedWithinRead(FaviconCacheElement element) {
        mReorderingSemaphore.acquireUninterruptibly();
        try {
            boolean contained = mOrdering.remove(element);
            mOrdering.offer(element);
            return contained;
        } finally {
            mReorderingSemaphore.release();
        }
    }

    






    private boolean setMostRecentlyUsedWithinWrite(FaviconCacheElement element) {
        boolean contained = mOrdering.remove(element);
        mOrdering.offer(element);
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

            mCurrentSize.addAndGet(newElement.sizeOf());

            
            FaviconsForURL wasRemoved;
            wasRemoved = mBackingMap.put(faviconURL, toInsert);

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
                mPermanentBackingMap.put(faviconURL, toInsert);
                return;
            }

            for (FaviconCacheElement newElement : toInsert.mFavicons) {
                setMostRecentlyUsedWithinWrite(newElement);
            }

            
            
            mCurrentSize.addAndGet(sizeGained);

            
            recordRemoved(mBackingMap.put(faviconURL, toInsert));
        } finally {
            finishWrite();
        }

        cullIfRequired();
    }

    



    private void cullIfRequired() {
        Log.d(LOGTAG, "Favicon cache fullness: " + mCurrentSize.get() + '/' + mMaxSizeBytes);

        if (mCurrentSize.get() <= mMaxSizeBytes) {
            return;
        }

        startWrite();
        try {
            while (mCurrentSize.get() > mMaxSizeBytes) {
                

                FaviconCacheElement victim;
                victim = mOrdering.poll();

                mCurrentSize.addAndGet(-victim.sizeOf());
                victim.onEvictedFromCache();

                Log.d(LOGTAG, "After cull: " + mCurrentSize.get() + '/' + mMaxSizeBytes);
            }
        } finally {
            finishWrite();
        }
    }

    


    public void evictAll() {
        startWrite();

        
        try {
            mCurrentSize.set(0);
            mBackingMap.clear();
            mOrdering.clear();

        } finally {
            finishWrite();
        }
    }
}

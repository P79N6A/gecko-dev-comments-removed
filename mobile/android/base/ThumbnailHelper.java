




package org.mozilla.gecko;

import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.mozglue.DirectBufferAllocator;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.util.Log;

import java.nio.ByteBuffer;
import java.util.LinkedList;
import java.util.concurrent.atomic.AtomicInteger;










public final class ThumbnailHelper {
    private static final String LOGTAG = "GeckoThumbnailHelper";

    public static final float THUMBNAIL_ASPECT_RATIO = 0.714f;  

    

    private static ThumbnailHelper sInstance;

    public static synchronized ThumbnailHelper getInstance() {
        if (sInstance == null) {
            sInstance = new ThumbnailHelper();
        }
        return sInstance;
    }

    

    private final LinkedList<Tab> mPendingThumbnails;    
    private AtomicInteger mPendingWidth;
    private int mWidth;
    private int mHeight;
    private ByteBuffer mBuffer;

    private ThumbnailHelper() {
        mPendingThumbnails = new LinkedList<Tab>();
        mPendingWidth = new AtomicInteger((int)GeckoApp.mAppContext.getResources().getDimension(R.dimen.tab_thumbnail_width));
        mWidth = -1;
        mHeight = -1;
    }

    public void getAndProcessThumbnailFor(Tab tab) {
        if ("about:home".equals(tab.getURL())) {
            tab.updateThumbnail(null);
            return;
        }

        if (tab.getState() == Tab.STATE_DELAYED) {
            String url = tab.getURL();
            if (url != null) {
                byte[] thumbnail = BrowserDB.getThumbnailForUrl(GeckoApp.mAppContext.getContentResolver(), url);
                if (thumbnail != null) {
                    setTabThumbnail(tab, null, thumbnail);
                }
            }
            return;
        }

        synchronized (mPendingThumbnails) {
            if (mPendingThumbnails.lastIndexOf(tab) > 0) {
                
                
                
                
                
                return;
            }

            mPendingThumbnails.add(tab);
            if (mPendingThumbnails.size() > 1) {
                
                
                return;
            }
        }
        requestThumbnailFor(tab);
    }

    public void setThumbnailWidth(int width) {
        mPendingWidth.set(IntSize.nextPowerOfTwo(width));
    }

    private void updateThumbnailSize() {
        
        mWidth = mPendingWidth.get();

        mWidth &= ~0x1; 
        mHeight = Math.round(mWidth * THUMBNAIL_ASPECT_RATIO);

        int capacity = mWidth * mHeight * 2; 
        if (mBuffer == null || mBuffer.capacity() != capacity) {
            if (mBuffer != null) {
                mBuffer = DirectBufferAllocator.free(mBuffer);
            }
            try {
                mBuffer = DirectBufferAllocator.allocate(capacity);
            } catch (IllegalArgumentException iae) {
                Log.w(LOGTAG, iae.toString());
            } catch (OutOfMemoryError oom) {
                Log.w(LOGTAG, "Unable to allocate thumbnail buffer of capacity " + capacity);
            }
            
        }
    }

    private void requestThumbnailFor(Tab tab) {
        updateThumbnailSize();

        if (mBuffer == null) {
            
            
            
            
            
            
            synchronized (mPendingThumbnails) {
                mPendingThumbnails.clear();
            }
            return;
        }

        GeckoEvent e = GeckoEvent.createThumbnailEvent(tab.getId(), mWidth, mHeight, mBuffer);
        GeckoAppShell.sendEventToGecko(e);
    }

    
    public static void notifyThumbnail(ByteBuffer data, int tabId, boolean success) {
        Tab tab = Tabs.getInstance().getTab(tabId);
        ThumbnailHelper helper = ThumbnailHelper.getInstance();
        if (success && tab != null) {
            helper.handleThumbnailData(tab, data);
        }
        helper.processNextThumbnail(tab);
    }

    private void processNextThumbnail(Tab tab) {
        Tab nextTab = null;
        synchronized (mPendingThumbnails) {
            if (tab != null && tab != mPendingThumbnails.peek()) {
                Log.e(LOGTAG, "handleThumbnailData called with unexpected tab's data!");
                
                
            } else {
                mPendingThumbnails.remove();
            }
            nextTab = mPendingThumbnails.peek();
        }
        if (nextTab != null) {
            requestThumbnailFor(nextTab);
        }
    }

    private void handleThumbnailData(Tab tab, ByteBuffer data) {
        if (data != mBuffer) {
            
            Log.e(LOGTAG, "handleThumbnailData called with an unexpected ByteBuffer!");
        }

        if (shouldUpdateThumbnail(tab)) {
            processThumbnailData(tab, data);
        }
    }

    private void processThumbnailData(Tab tab, ByteBuffer data) {
        Bitmap b = tab.getThumbnailBitmap(mWidth, mHeight);
        data.position(0);
        b.copyPixelsFromBuffer(data);
        setTabThumbnail(tab, b, null);
    }

    private void setTabThumbnail(Tab tab, Bitmap bitmap, byte[] compressed) {
        try {
            if (bitmap == null) {
                if (compressed == null) {
                    Log.w(LOGTAG, "setTabThumbnail: one of bitmap or compressed must be non-null!");
                    return;
                }
                bitmap = BitmapFactory.decodeByteArray(compressed, 0, compressed.length);
            }
            tab.updateThumbnail(bitmap);
        } catch (OutOfMemoryError ome) {
            Log.w(LOGTAG, "setTabThumbnail: decoding byte array of length " + compressed.length + " ran out of memory");
        }
    }

    private boolean shouldUpdateThumbnail(Tab tab) {
        return (Tabs.getInstance().isSelectedTab(tab) || GeckoApp.mAppContext.areTabsShown());
    }
}

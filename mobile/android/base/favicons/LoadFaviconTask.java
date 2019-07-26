



package org.mozilla.gecko.favicons;


import android.content.ContentResolver;
import android.graphics.Bitmap;
import android.net.http.AndroidHttpClient;
import android.os.Handler;
import android.text.TextUtils;
import android.util.Log;
import org.apache.http.Header;
import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.entity.BufferedHttpEntity;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.util.GeckoJarReader;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.util.UiAsyncTask;
import static org.mozilla.gecko.favicons.Favicons.sContext;

import java.io.IOException;
import java.io.InputStream;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.HashMap;
import java.util.HashSet;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;







public class LoadFaviconTask extends UiAsyncTask<Void, Void, Bitmap> {
    private static final String LOGTAG = "LoadFaviconTask";

    
    
    private static final HashMap<String, LoadFaviconTask> loadsInFlight = new HashMap<String, LoadFaviconTask>();

    public static final int FLAG_PERSIST = 1;
    public static final int FLAG_SCALE = 2;
    private static final int MAX_REDIRECTS_TO_FOLLOW = 5;

    private static AtomicInteger mNextFaviconLoadId = new AtomicInteger(0);
    private int mId;
    private String mPageUrl;
    private String mFaviconUrl;
    private OnFaviconLoadedListener mListener;
    private int mFlags;

    private final boolean mOnlyFromLocal;

    
    protected int mTargetWidth;
    private final AtomicReference<LoadFaviconTask> mChainee = new AtomicReference<LoadFaviconTask>(null);
    private boolean mIsChaining;

    static AndroidHttpClient sHttpClient = AndroidHttpClient.newInstance(GeckoAppShell.getGeckoInterface().getDefaultUAString());

    public LoadFaviconTask(Handler backgroundThreadHandler,
                           String pageUrl, String faviconUrl, int flags,
                           OnFaviconLoadedListener listener) {
        this(backgroundThreadHandler, pageUrl, faviconUrl, flags, listener, -1, false);
    }
    public LoadFaviconTask(Handler backgroundThreadHandler,
                           String pageUrl, String faviconUrl, int flags,
                           OnFaviconLoadedListener aListener, int targetSize, boolean fromLocal) {
        super(backgroundThreadHandler);

        mId = mNextFaviconLoadId.incrementAndGet();

        mPageUrl = pageUrl;
        mFaviconUrl = faviconUrl;
        mListener = aListener;
        mFlags = flags;
        mTargetWidth = targetSize;
        mOnlyFromLocal = fromLocal;
    }

    
    private Bitmap loadFaviconFromDb() {
        ContentResolver resolver = sContext.getContentResolver();
        return BrowserDB.getFaviconForFaviconUrl(resolver, mFaviconUrl);
    }

    
    private void saveFaviconToDb(final Bitmap favicon) {
        if ((mFlags & FLAG_PERSIST) == 0) {
            return;
        }

        ContentResolver resolver = sContext.getContentResolver();
        BrowserDB.updateFaviconForUrl(resolver, mPageUrl, favicon, mFaviconUrl);
    }

    




    private HttpResponse tryDownload(URI faviconURI) throws URISyntaxException, IOException {
        HashSet<String> visitedLinkSet = new HashSet<String>();
        visitedLinkSet.add(faviconURI.toString());
        return tryDownloadRecurse(faviconURI, visitedLinkSet);
    }
    private HttpResponse tryDownloadRecurse(URI faviconURI, HashSet<String> visited) throws URISyntaxException, IOException {
        if (visited.size() == MAX_REDIRECTS_TO_FOLLOW) {
            return null;
        }

        HttpGet request = new HttpGet(faviconURI);
        HttpResponse response = sHttpClient.execute(request);
        if (response == null) {
            return null;
        }

        if (response.getStatusLine() != null) {

            
            int status = response.getStatusLine().getStatusCode();

            
            if (status >= 300 && status < 400) {
                Header header = response.getFirstHeader("Location");

                
                if (header == null) {
                    return null;
                }

                String newURI = header.getValue();
                if (newURI == null || newURI.equals(faviconURI.toString())) {
                    return null;
                }

                if (visited.contains(newURI)) {
                    
                    return null;
                }

                visited.add(newURI);
                return tryDownloadRecurse(new URI(newURI), visited);
            }

            if (status >= 400) {
                return null;
            }
        }
        return response;
    }

    
    private Bitmap downloadFavicon(URI targetFaviconURI) {
        if (mFaviconUrl.startsWith("jar:jar:")) {
            return GeckoJarReader.getBitmap(sContext.getResources(), mFaviconUrl);
        }

        
        String scheme = targetFaviconURI.getScheme();
        if (!"http".equals(scheme) && !"https".equals(scheme)) {
            return null;
        }

        
        
        Bitmap image = null;
        try {
            
            HttpResponse response = tryDownload(targetFaviconURI);
            if (response == null) {
                return null;
            }

            HttpEntity entity = response.getEntity();
            if (entity == null) {
                return null;
            }

            BufferedHttpEntity bufferedEntity = new BufferedHttpEntity(entity);
            InputStream contentStream = null;
            try {
                contentStream = bufferedEntity.getContent();
                image = BitmapUtils.decodeStream(contentStream);
                contentStream.close();
            } finally {
                if (contentStream != null) {
                    contentStream.close();
                }
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "Error reading favicon", e);
        }

        return image;
    }

    @Override
    protected Bitmap doInBackground(Void... unused) {
        if (isCancelled()) {
            return null;
        }

        String storedFaviconUrl;
        boolean isUsingDefaultURL = false;

        
        
        if (TextUtils.isEmpty(mFaviconUrl)) {
            
            storedFaviconUrl = Favicons.getFaviconURLForPageURLFromCache(mPageUrl);

            
            if (storedFaviconUrl == null) {
                storedFaviconUrl = Favicons.getFaviconUrlForPageUrl(mPageUrl);
                if (storedFaviconUrl != null) {
                    
                    Favicons.putFaviconURLForPageURLInCache(mPageUrl, storedFaviconUrl);
                }
            }

            
            if (storedFaviconUrl != null) {
                mFaviconUrl = storedFaviconUrl;
            } else {
                
                mFaviconUrl = Favicons.guessDefaultFaviconURL(mPageUrl);
                isUsingDefaultURL = true;
            }
        }

        
        
        if (Favicons.isFailedFavicon(mFaviconUrl)) {
            return null;
        }

        if (isCancelled()) {
            return null;
        }

        Bitmap image;
        
        
        synchronized(loadsInFlight) {
            
            LoadFaviconTask existingTask = loadsInFlight.get(mFaviconUrl);
            if (existingTask != null && !existingTask.isCancelled()) {
                existingTask.chainTasks(this);
                mIsChaining = true;
            }

            
            
            
            loadsInFlight.put(mFaviconUrl, this);
        }

        if (mIsChaining) {
            
            return null;
        }

        if (isCancelled()) {
            return null;
        }

        image = loadFaviconFromDb();
        if (image != null && image.getWidth() > 0 && image.getHeight() > 0) {
            return image;
        }

        if (mOnlyFromLocal || isCancelled()) {
            return null;
        }

        try {
            image = downloadFavicon(new URI(mFaviconUrl));
        } catch (URISyntaxException e) {
            Log.e(LOGTAG, "The provided favicon URL is not valid");
            return null;
        }

        
        if (image == null && !isUsingDefaultURL) {
            try {
                image = downloadFavicon(new URI(Favicons.guessDefaultFaviconURL(mPageUrl)));
            } catch (URISyntaxException e){
                
            }
        }

        if (image != null && image.getWidth() > 0 && image.getHeight() > 0) {
            saveFaviconToDb(image);
        } else {
            Favicons.putFaviconInFailedCache(mFaviconUrl);
        }

        return image;
    }

    @Override
    protected void onPostExecute(Bitmap image) {
        if (mIsChaining) {
            return;
        }

        
        Favicons.putFaviconInMemCache(mFaviconUrl, image);

        
        processResult(image);
    }

    private void processResult(Bitmap image) {
        Favicons.removeLoadTask(mId);

        Bitmap scaled = image;

        
        if (mTargetWidth != -1 && image != null &&  image.getWidth() != mTargetWidth) {
            scaled = Favicons.getSizedFaviconFromCache(mFaviconUrl, mTargetWidth);
        }

        Favicons.dispatchResult(mPageUrl, mFaviconUrl, scaled, mListener);

        
        final LoadFaviconTask chainee = mChainee.get();

        if (chainee != null) {
            
            
            
            chainee.processResult(image);
            mChainee.set(null);
            return;
        }

        
        
        
        synchronized(loadsInFlight) {
            if (mChainee.get() == null) {
                loadsInFlight.remove(mFaviconUrl);
                return;
            }
        }

        
        mChainee.get().processResult(image);
    }

    @Override
    protected void onCancelled() {
        Favicons.removeLoadTask(mId);

        synchronized(loadsInFlight) {
            if (mChainee.get() == null) {
                loadsInFlight.remove(mFaviconUrl);
            }
        }

        
        
    }

    






    private void chainTasks(LoadFaviconTask aChainee) {
        
        if (mChainee.compareAndSet(null, aChainee)) {
            return;
        }

        
        
        mChainee.get().chainTasks(aChainee);
    }

    int getId() {
        return mId;
    }

    static void closeHTTPClient() {
        
        
        
        if (ThreadUtils.isOnBackgroundThread()) {
            if (sHttpClient != null) {
                sHttpClient.close();
            }
            return;
        }

        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                LoadFaviconTask.closeHTTPClient();
            }
        });
    }
}

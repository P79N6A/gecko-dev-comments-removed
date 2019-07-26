



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
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.favicons.decoders.FaviconDecoder;
import org.mozilla.gecko.favicons.decoders.LoadFaviconResult;
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
import java.util.LinkedList;
import java.util.concurrent.atomic.AtomicInteger;







public class LoadFaviconTask extends UiAsyncTask<Void, Void, Bitmap> {
    private static final String LOGTAG = "LoadFaviconTask";

    
    
    private static final HashMap<String, LoadFaviconTask> loadsInFlight = new HashMap<String, LoadFaviconTask>();

    public static final int FLAG_PERSIST = 1;
    public static final int FLAG_SCALE = 2;
    private static final int MAX_REDIRECTS_TO_FOLLOW = 5;
    
    
    private static final int DEFAULT_FAVICON_BUFFER_SIZE = 25000;

    private static AtomicInteger mNextFaviconLoadId = new AtomicInteger(0);
    private int mId;
    private String mPageUrl;
    private String mFaviconUrl;
    private OnFaviconLoadedListener mListener;
    private int mFlags;

    private final boolean mOnlyFromLocal;

    
    protected int mTargetWidth;
    private LinkedList<LoadFaviconTask> mChainees;
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

    
    private LoadFaviconResult loadFaviconFromDb() {
        ContentResolver resolver = sContext.getContentResolver();
        return BrowserDB.getFaviconForFaviconUrl(resolver, mFaviconUrl);
    }

    
    private void saveFaviconToDb(final byte[] encodedFavicon) {
        if ((mFlags & FLAG_PERSIST) == 0) {
            return;
        }

        ContentResolver resolver = sContext.getContentResolver();
        BrowserDB.updateFaviconForUrl(resolver, mPageUrl, encodedFavicon, mFaviconUrl);
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

    



    private static Bitmap fetchJARFavicon(String uri) {
        if (uri == null) {
            return null;
        }
        if (uri.startsWith("jar:jar:")) {
            Log.d(LOGTAG, "Fetching favicon from JAR.");
            try {
                return GeckoJarReader.getBitmap(sContext.getResources(), uri);
            } catch (Exception e) {
                
                Log.w(LOGTAG, "Error fetching favicon from JAR.", e);
                return null;
            }
        }
        return null;
    }

    
    
    private LoadFaviconResult downloadFavicon(URI targetFaviconURI) {
        if (targetFaviconURI == null) {
            return null;
        }

        
        String scheme = targetFaviconURI.getScheme();
        if (!"http".equals(scheme) && !"https".equals(scheme)) {
            return null;
        }

        LoadFaviconResult result = null;

        try {
            result = downloadAndDecodeImage(targetFaviconURI);
        } catch (Exception e) {
            Log.e(LOGTAG, "Error reading favicon", e);
        }

        return result;
    }

    










    private LoadFaviconResult downloadAndDecodeImage(URI targetFaviconURL) throws IOException, URISyntaxException {
        
        HttpResponse response = tryDownload(targetFaviconURL);
        if (response == null) {
            return null;
        }

        HttpEntity entity = response.getEntity();
        if (entity == null) {
            return null;
        }

        
        final long entityReportedLength = entity.getContentLength();
        int bufferSize;
        if (entityReportedLength > 0) {
            
            
            bufferSize = (int) entityReportedLength + 1;
        } else {
            
            bufferSize = DEFAULT_FAVICON_BUFFER_SIZE;
        }

        
        byte[] buffer = new byte[bufferSize];

        
        int bPointer = 0;

        
        int lastRead = 0;
        InputStream contentStream = entity.getContent();
        try {
            
            
            while (lastRead != -1) {
                
                lastRead = contentStream.read(buffer, bPointer, buffer.length - bPointer);
                bPointer += lastRead;

                
                if (bPointer == buffer.length) {
                    bufferSize *= 2;
                    byte[] newBuffer = new byte[bufferSize];

                    
                    System.arraycopy(buffer, 0, newBuffer, 0, buffer.length);
                    buffer = newBuffer;
                }
            }
        } finally {
            contentStream.close();
        }

        
        return FaviconDecoder.decodeFavicon(buffer, 0, bPointer + 1);
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
                storedFaviconUrl = Favicons.getFaviconURLForPageURL(mPageUrl);
                if (storedFaviconUrl != null) {
                    
                    Favicons.putFaviconURLForPageURLInCache(mPageUrl, storedFaviconUrl);
                }
            }

            
            if (storedFaviconUrl != null) {
                mFaviconUrl = storedFaviconUrl;
            } else {
                
                mFaviconUrl = Favicons.guessDefaultFaviconURL(mPageUrl);

                if (TextUtils.isEmpty(mFaviconUrl)) {
                    return null;
                }
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

                
                
                return null;
            }

            
            
            loadsInFlight.put(mFaviconUrl, this);
        }

        if (isCancelled()) {
            return null;
        }

        
        LoadFaviconResult loadedBitmaps = loadFaviconFromDb();
        if (loadedBitmaps != null) {
            return pushToCacheAndGetResult(loadedBitmaps);
        }

        if (mOnlyFromLocal || isCancelled()) {
            return null;
        }

        
        image = fetchJARFavicon(mFaviconUrl);
        if (imageIsValid(image)) {
            
            Favicons.putFaviconInMemCache(mFaviconUrl, image);
            return image;
        }

        try {
            loadedBitmaps = downloadFavicon(new URI(mFaviconUrl));
        } catch (URISyntaxException e) {
            Log.e(LOGTAG, "The provided favicon URL is not valid");
            return null;
        } catch (Exception e) {
            Log.e(LOGTAG, "Couldn't download favicon.", e);
        }

        if (loadedBitmaps != null) {
            saveFaviconToDb(loadedBitmaps.getBytesForDatabaseStorage());
            return pushToCacheAndGetResult(loadedBitmaps);
        }

        if (isUsingDefaultURL) {
            Favicons.putFaviconInFailedCache(mFaviconUrl);
            return null;
        }

        if (isCancelled()) {
            return null;
        }

        
        final String guessed = Favicons.guessDefaultFaviconURL(mPageUrl);
        if (guessed == null) {
            Favicons.putFaviconInFailedCache(mFaviconUrl);
            return null;
        }

        image = fetchJARFavicon(guessed);
        if (imageIsValid(image)) {
            
            Favicons.putFaviconInMemCache(mFaviconUrl, image);
            return image;
        }

        try {
            loadedBitmaps = downloadFavicon(new URI(guessed));
        } catch (Exception e) {
            
            return null;
        }

        if (loadedBitmaps != null) {
            saveFaviconToDb(loadedBitmaps.getBytesForDatabaseStorage());
            return pushToCacheAndGetResult(loadedBitmaps);
        }

        return null;
    }

    








    private Bitmap pushToCacheAndGetResult(LoadFaviconResult loadedBitmaps) {
        Favicons.putFaviconsInMemCache(mFaviconUrl, loadedBitmaps.getBitmaps());
        Bitmap result = Favicons.getSizedFaviconFromCache(mFaviconUrl, mTargetWidth);
        return result;
    }

    private static boolean imageIsValid(final Bitmap image) {
        return image != null &&
               image.getWidth() > 0 &&
               image.getHeight() > 0;
    }

    @Override
    protected void onPostExecute(Bitmap image) {
        if (mIsChaining) {
            return;
        }

        
        processResult(image);

        synchronized (loadsInFlight) {
            
            loadsInFlight.remove(mFaviconUrl);
        }

        
        
        
        

        
        
        

        
        if (mChainees != null) {
            for (LoadFaviconTask t : mChainees) {
                
                
                t.processResult(image);
            }
        }
    }

    private void processResult(Bitmap image) {
        Favicons.removeLoadTask(mId);
        Bitmap scaled = image;

        
        if (mTargetWidth != -1 && image != null &&  image.getWidth() != mTargetWidth) {
            scaled = Favicons.getSizedFaviconFromCache(mFaviconUrl, mTargetWidth);
        }

        Favicons.dispatchResult(mPageUrl, mFaviconUrl, scaled, mListener);
    }

    @Override
    protected void onCancelled() {
        Favicons.removeLoadTask(mId);

        synchronized(loadsInFlight) {
            
            
            final LoadFaviconTask primary = loadsInFlight.get(mFaviconUrl);
            if (primary == this) {
                loadsInFlight.remove(mFaviconUrl);
                return;
            }
            if (primary == null) {
                
                return;
            }
            if (primary.mChainees != null) {
              primary.mChainees.remove(this);
            }
        }

        
        
    }

    







    private void chainTasks(LoadFaviconTask aChainee) {
        if (mChainees == null) {
            mChainees = new LinkedList<LoadFaviconTask>();
        }

        mChainees.add(aChainee);
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

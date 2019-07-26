



package org.mozilla.gecko.favicons;


import android.content.ContentResolver;
import android.graphics.Bitmap;
import android.net.http.AndroidHttpClient;
import android.os.Handler;
import android.util.Log;
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

import java.io.InputStream;
import java.net.MalformedURLException;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;
import java.util.concurrent.atomic.AtomicInteger;







public class LoadFaviconTask extends UiAsyncTask<Void, Void, Bitmap> {
    private static final String LOGTAG = "LoadFaviconTask";

    public static final int FLAG_PERSIST = 1;
    public static final int FLAG_SCALE = 2;

    private static AtomicInteger mNextFaviconLoadId = new AtomicInteger(0);
    private int mId;
    private String mPageUrl;
    private String mFaviconUrl;
    private OnFaviconLoadedListener mListener;
    private int mFlags;

    static AndroidHttpClient sHttpClient = AndroidHttpClient.newInstance(GeckoAppShell.getGeckoInterface().getDefaultUAString());

    public LoadFaviconTask(Handler backgroundThreadHandler,
                           String aPageUrl, String aFaviconUrl, int aFlags,
                           OnFaviconLoadedListener aListener) {
        super(backgroundThreadHandler);

        mId = mNextFaviconLoadId.incrementAndGet();

        mPageUrl = aPageUrl;
        mFaviconUrl = aFaviconUrl;
        mListener = aListener;
        mFlags = aFlags;
    }

    
    private Bitmap loadFaviconFromDb() {
        ContentResolver resolver = sContext.getContentResolver();
        return BrowserDB.getFaviconForUrl(resolver, mPageUrl);
    }

    
    private void saveFaviconToDb(final Bitmap favicon) {
        if ((mFlags & FLAG_PERSIST) == 0) {
            return;
        }

        ContentResolver resolver = sContext.getContentResolver();
        BrowserDB.updateFaviconForUrl(resolver, mPageUrl, favicon, mFaviconUrl);
    }

    
    private Bitmap downloadFavicon(URL targetFaviconURL) {
        if (mFaviconUrl.startsWith("jar:jar:")) {
            return GeckoJarReader.getBitmap(sContext.getResources(), mFaviconUrl);
        }

        URI uri;
        try {
            uri = targetFaviconURL.toURI();
        } catch (URISyntaxException e) {
            Log.d(LOGTAG, "Could not get URI for favicon");
            return null;
        }

        
        String scheme = uri.getScheme();
        if (!"http".equals(scheme) && !"https".equals(scheme))
            return null;

        
        
        Bitmap image = null;
        try {
            HttpGet request = new HttpGet(targetFaviconURL.toURI());
            HttpResponse response = sHttpClient.execute(request);
            if (response == null)
                return null;
            if (response.getStatusLine() != null) {
                
                int status = response.getStatusLine().getStatusCode();
                if (status >= 400) {
                    Favicons.putFaviconInFailedCache(mPageUrl, Favicons.FAILED_EXPIRY_NEVER);
                    return null;
                }
            }

            HttpEntity entity = response.getEntity();
            if (entity == null)
                return null;
            if (entity.getContentType() != null) {
                
                String contentType = entity.getContentType().getValue();
                if (contentType.indexOf("image") == -1)
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
        Bitmap image;

        if (isCancelled())
            return null;

        URL faviconURLToDownload;

        
        try {
            
            if (mFaviconUrl == null || mFaviconUrl.length() == 0) {
                
                URL targetPageURL = new URL(mPageUrl);

                faviconURLToDownload = new URL(targetPageURL.getProtocol(), targetPageURL.getAuthority(), "/favicon.ico");
                mFaviconUrl = faviconURLToDownload.toString();
            } else {
                faviconURLToDownload = new URL(mFaviconUrl);
            }
        } catch (MalformedURLException e) {
            Log.d(LOGTAG, "The provided favicon URL is not valid");
            return null;
        }

        if (isCancelled())
            return null;

        String storedFaviconUrl = Favicons.getFaviconUrlForPageUrl(mPageUrl);
        if (storedFaviconUrl != null && storedFaviconUrl.equals(mFaviconUrl)) {
            image = loadFaviconFromDb();
            if (image != null && image.getWidth() > 0 && image.getHeight() > 0)
                return ((mFlags & FLAG_SCALE) != 0) ? Favicons.scaleImage(image) : image;
        }

        if (isCancelled())
            return null;

        image = downloadFavicon(faviconURLToDownload);

        if (image != null && image.getWidth() > 0 && image.getHeight() > 0) {
            saveFaviconToDb(image);
            image = ((mFlags & FLAG_SCALE) != 0) ? Favicons.scaleImage(image) : image;
        } else {
            image = null;
        }

        return image;
    }

    @Override
    protected void onPostExecute(final Bitmap image) {
        Favicons.removeLoadTask(mId);
        Favicons.dispatchResult(mPageUrl, image, mListener);
    }

    @Override
    protected void onCancelled() {
        Favicons.removeLoadTask(mId);

        
        
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

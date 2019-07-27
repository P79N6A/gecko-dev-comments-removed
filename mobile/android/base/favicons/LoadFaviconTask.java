



package org.mozilla.gecko.favicons;


import android.content.ContentResolver;
import android.content.Context;
import android.graphics.Bitmap;
import android.net.http.AndroidHttpClient;
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

import java.io.IOException;
import java.io.InputStream;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.atomic.AtomicInteger;







public class LoadFaviconTask {
    private static final String LOGTAG = "LoadFaviconTask";

    
    
    private static final HashMap<String, LoadFaviconTask> loadsInFlight = new HashMap<>();

    public static final int FLAG_PERSIST = 1;
    public static final int FLAG_SCALE = 2;
    private static final int MAX_REDIRECTS_TO_FOLLOW = 5;
    
    
    private static final int DEFAULT_FAVICON_BUFFER_SIZE = 25000;

    private static final AtomicInteger nextFaviconLoadId = new AtomicInteger(0);
    private final Context context;
    private final int id;
    private final String pageUrl;
    private String faviconURL;
    private final OnFaviconLoadedListener listener;
    private final int flags;

    private final boolean onlyFromLocal;
    volatile boolean mCancelled;

    
    protected int targetWidth;
    private LinkedList<LoadFaviconTask> chainees;
    private boolean isChaining;

    static AndroidHttpClient httpClient = AndroidHttpClient.newInstance(GeckoAppShell.getGeckoInterface().getDefaultUAString());

    public LoadFaviconTask(Context context, String pageURL, String faviconURL, int flags, OnFaviconLoadedListener listener) {
        this(context, pageURL, faviconURL, flags, listener, -1, false);
    }

    public LoadFaviconTask(Context context, String pageURL, String faviconURL, int flags, OnFaviconLoadedListener listener,
                           int targetWidth, boolean onlyFromLocal) {
        id = nextFaviconLoadId.incrementAndGet();

        this.context = context;
        this.pageUrl = pageURL;
        this.faviconURL = faviconURL;
        this.listener = listener;
        this.flags = flags;
        this.targetWidth = targetWidth;
        this.onlyFromLocal = onlyFromLocal;
    }

    
    private LoadFaviconResult loadFaviconFromDb() {
        ContentResolver resolver = context.getContentResolver();
        return BrowserDB.getFaviconForFaviconUrl(resolver, faviconURL);
    }

    
    private void saveFaviconToDb(final byte[] encodedFavicon) {
        if (encodedFavicon == null) {
            return;
        }

        if ((flags & FLAG_PERSIST) == 0) {
            return;
        }

        ContentResolver resolver = context.getContentResolver();
        BrowserDB.updateFaviconForUrl(resolver, pageUrl, encodedFavicon, faviconURL);
    }

    




    private HttpResponse tryDownload(URI faviconURI) throws URISyntaxException, IOException {
        HashSet<String> visitedLinkSet = new HashSet<>();
        visitedLinkSet.add(faviconURI.toString());
        return tryDownloadRecurse(faviconURI, visitedLinkSet);
    }
    private HttpResponse tryDownloadRecurse(URI faviconURI, HashSet<String> visited) throws URISyntaxException, IOException {
        if (visited.size() == MAX_REDIRECTS_TO_FOLLOW) {
            return null;
        }

        HttpGet request = new HttpGet(faviconURI);
        HttpResponse response = httpClient.execute(request);
        if (response == null) {
            return null;
        }

        if (response.getStatusLine() != null) {

            
            int status = response.getStatusLine().getStatusCode();

            
            if (status >= 300 && status < 400) {
                Header header = response.getFirstHeader("Location");

                
                final String newURI;
                try {
                    if (header == null) {
                        return null;
                    }

                    newURI = header.getValue();
                    if (newURI == null || newURI.equals(faviconURI.toString())) {
                        return null;
                    }

                    if (visited.contains(newURI)) {
                        
                        return null;
                    }

                    visited.add(newURI);
                } finally {
                    
                    try {
                        response.getEntity().consumeContent();
                    } catch (Exception e) {
                        
                    }
                }

                return tryDownloadRecurse(new URI(newURI), visited);
            }

            if (status >= 400) {
                
                try {
                    response.getEntity().consumeContent();
                } catch (Exception e) {
                    
                }
                return null;
            }
        }
        return response;
    }

    



    private Bitmap fetchJARFavicon(String uri) {
        if (uri == null) {
            return null;
        }
        if (uri.startsWith("jar:jar:")) {
            Log.d(LOGTAG, "Fetching favicon from JAR.");
            try {
                return GeckoJarReader.getBitmap(context.getResources(), uri);
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
        } catch (OutOfMemoryError e) {
            Log.e(LOGTAG, "Insufficient memory to process favicon");
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

        
        try {
            return decodeImageFromResponse(entity);
        } finally {
            
            entity.consumeContent();
        }
    }

    









    private LoadFaviconResult decodeImageFromResponse(HttpEntity entity) throws IOException {
        
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

    
    
    public final void execute() {
        ThreadUtils.assertOnUiThread();

        try {
            Favicons.longRunningExecutor.execute(new Runnable() {
                @Override
                public void run() {
                    final Bitmap result = doInBackground();

                    ThreadUtils.getUiHandler().post(new Runnable() {
                       @Override
                        public void run() {
                            if (mCancelled) {
                                onCancelled();
                            } else {
                                onPostExecute(result);
                            }
                        }
                    });
                }
            });
          } catch (RejectedExecutionException e) {
              
              onCancelled();
          }
    }

    public final boolean cancel() {
        mCancelled = true;
        return true;
    }

    public final boolean isCancelled() {
        return mCancelled;
    }

    Bitmap doInBackground() {
        if (isCancelled()) {
            return null;
        }

        String storedFaviconUrl;
        boolean isUsingDefaultURL = false;

        
        
        if (TextUtils.isEmpty(faviconURL)) {
            
            storedFaviconUrl = Favicons.getFaviconURLForPageURLFromCache(pageUrl);

            
            if (storedFaviconUrl == null) {
                storedFaviconUrl = Favicons.getFaviconURLForPageURL(context, pageUrl);
                if (storedFaviconUrl != null) {
                    
                    Favicons.putFaviconURLForPageURLInCache(pageUrl, storedFaviconUrl);
                }
            }

            
            if (storedFaviconUrl != null) {
                faviconURL = storedFaviconUrl;
            } else {
                
                faviconURL = Favicons.guessDefaultFaviconURL(pageUrl);

                if (TextUtils.isEmpty(faviconURL)) {
                    return null;
                }
                isUsingDefaultURL = true;
            }
        }

        
        
        if (Favicons.isFailedFavicon(faviconURL)) {
            return null;
        }

        if (isCancelled()) {
            return null;
        }

        Bitmap image;
        
        
        synchronized(loadsInFlight) {
            
            LoadFaviconTask existingTask = loadsInFlight.get(faviconURL);
            if (existingTask != null && !existingTask.isCancelled()) {
                existingTask.chainTasks(this);
                isChaining = true;

                
                
                return null;
            }

            
            
            loadsInFlight.put(faviconURL, this);
        }

        if (isCancelled()) {
            return null;
        }

        
        LoadFaviconResult loadedBitmaps = loadFaviconFromDb();
        if (loadedBitmaps != null) {
            return pushToCacheAndGetResult(loadedBitmaps);
        }

        if (onlyFromLocal || isCancelled()) {
            return null;
        }

        
        image = fetchJARFavicon(faviconURL);
        if (imageIsValid(image)) {
            
            Favicons.putFaviconInMemCache(faviconURL, image);
            return image;
        }

        try {
            loadedBitmaps = downloadFavicon(new URI(faviconURL));
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
            Favicons.putFaviconInFailedCache(faviconURL);
            return null;
        }

        if (isCancelled()) {
            return null;
        }

        
        final String guessed = Favicons.guessDefaultFaviconURL(pageUrl);
        if (guessed == null) {
            Favicons.putFaviconInFailedCache(faviconURL);
            return null;
        }

        image = fetchJARFavicon(guessed);
        if (imageIsValid(image)) {
            
            Favicons.putFaviconInMemCache(faviconURL, image);
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
        Favicons.putFaviconsInMemCache(faviconURL, loadedBitmaps.getBitmaps());
        Bitmap result = Favicons.getSizedFaviconFromCache(faviconURL, targetWidth);
        return result;
    }

    private static boolean imageIsValid(final Bitmap image) {
        return image != null &&
               image.getWidth() > 0 &&
               image.getHeight() > 0;
    }

    void onPostExecute(Bitmap image) {
        if (isChaining) {
            return;
        }

        
        processResult(image);

        synchronized (loadsInFlight) {
            
            loadsInFlight.remove(faviconURL);
        }

        
        
        
        

        
        
        

        
        if (chainees != null) {
            for (LoadFaviconTask t : chainees) {
                
                
                t.processResult(image);
            }
        }
    }

    private void processResult(Bitmap image) {
        Favicons.removeLoadTask(id);
        Bitmap scaled = image;

        
        if (targetWidth != -1 && image != null &&  image.getWidth() != targetWidth) {
            scaled = Favicons.getSizedFaviconFromCache(faviconURL, targetWidth);
        }

        Favicons.dispatchResult(pageUrl, faviconURL, scaled, listener);
    }

    void onCancelled() {
        Favicons.removeLoadTask(id);

        synchronized(loadsInFlight) {
            
            
            final LoadFaviconTask primary = loadsInFlight.get(faviconURL);
            if (primary == this) {
                loadsInFlight.remove(faviconURL);
                return;
            }
            if (primary == null) {
                
                return;
            }
            if (primary.chainees != null) {
              primary.chainees.remove(this);
            }
        }

        
        
    }

    







    private void chainTasks(LoadFaviconTask aChainee) {
        if (chainees == null) {
            chainees = new LinkedList<>();
        }

        chainees.add(aChainee);
    }

    int getId() {
        return id;
    }

    static void closeHTTPClient() {
        
        
        
        if (ThreadUtils.isOnBackgroundThread()) {
            if (httpClient != null) {
                httpClient.close();
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

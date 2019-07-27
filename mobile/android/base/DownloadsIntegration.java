




package org.mozilla.gecko;

import org.mozilla.gecko.AppConstants.Versions;
import org.mozilla.gecko.util.NativeEventListener;
import org.mozilla.gecko.util.NativeJSObject;
import org.mozilla.gecko.util.EventCallback;
import org.mozilla.gecko.mozglue.generatorannotations.WrapElementForJNI;

import java.io.File;
import java.lang.IllegalArgumentException;
import java.util.ArrayList;
import java.util.List;

import android.app.DownloadManager;
import android.content.Context;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.media.MediaScannerConnection;
import android.media.MediaScannerConnection.MediaScannerConnectionClient;
import android.net.Uri;
import android.text.TextUtils;
import android.util.Log;

public class DownloadsIntegration implements NativeEventListener
{
    private static final String LOGTAG = "GeckoDownloadsIntegration";

    @SuppressWarnings("serial")
    private static final List<String> UNKNOWN_MIME_TYPES = new ArrayList<String>(3) {{
        add("unknown/unknown"); 
        add("application/unknown");
        add("application/octet-stream"); 
    }};

    private static final String DOWNLOAD_REMOVE = "Download:Remove";

    private DownloadsIntegration() {
        EventDispatcher.getInstance().registerGeckoThreadListener((NativeEventListener)this, DOWNLOAD_REMOVE);
    }

    private static DownloadsIntegration sInstance;

    private static class Download {
        final File file;
        final long id;

        final private static int UNKNOWN_ID = -1;

        public Download(final String path) {
            this(path, UNKNOWN_ID);
        }

        public Download(final String path, final long id) {
            file = new File(path);
            this.id = id;
        }

        public static Download fromJSON(final NativeJSObject obj) {
            final String path = obj.getString("path");
            return new Download(path);
        }

        public static Download fromCursor(final Cursor c) {
            final String path = c.getString(c.getColumnIndexOrThrow(DownloadManager.COLUMN_LOCAL_FILENAME));
            final long id = c.getLong(c.getColumnIndexOrThrow(DownloadManager.COLUMN_ID));
            return new Download(path, id);
        }

        public boolean equals(final Download download) {
            return file.equals(download.file);
        }
    }

    public static void init() {
        if (sInstance == null) {
            sInstance = new DownloadsIntegration();
        }
    }

    @Override
    public void handleMessage(final String event, final NativeJSObject message,
                              final EventCallback callback) {
        if (DOWNLOAD_REMOVE.equals(event)) {
            final Download d = Download.fromJSON(message);
            removeDownload(d);
        }
    }

    private static boolean useSystemDownloadManager() {
        if (!AppConstants.ANDROID_DOWNLOADS_INTEGRATION) {
            return false;
        }

        int state = PackageManager.COMPONENT_ENABLED_STATE_DEFAULT;
        try {
            state = GeckoAppShell.getContext().getPackageManager().getApplicationEnabledSetting("com.android.providers.downloads");
        } catch (IllegalArgumentException e) {
            
            return false;
        }

        return (PackageManager.COMPONENT_ENABLED_STATE_ENABLED == state ||
                PackageManager.COMPONENT_ENABLED_STATE_DEFAULT == state);
    }

    @WrapElementForJNI
    public static void scanMedia(final String aFile, String aMimeType) {
        String mimeType = aMimeType;
        if (UNKNOWN_MIME_TYPES.contains(mimeType)) {
            
            
            mimeType = "";
        }

        
        if (TextUtils.isEmpty(mimeType)) {
            final int extPosition = aFile.lastIndexOf(".");
            if (extPosition > 0 && extPosition < aFile.length() - 1) {
                mimeType = GeckoAppShell.getMimeTypeFromExtension(aFile.substring(extPosition+1));
            }
        }

        
        
        if (TextUtils.isEmpty(mimeType)) {
            if (TextUtils.isEmpty(aMimeType)) {
                mimeType = UNKNOWN_MIME_TYPES.get(0);
            } else {
                mimeType = aMimeType;
            }
        }

        if (useSystemDownloadManager()) {
            final File f = new File(aFile);
            final DownloadManager dm = (DownloadManager) GeckoAppShell.getContext().getSystemService(Context.DOWNLOAD_SERVICE);
            dm.addCompletedDownload(f.getName(),
                    f.getName(),
                    true, 
                    mimeType,
                    f.getAbsolutePath(),
                    Math.max(1, f.length()), 
                    false); 
        } else {
            final Context context = GeckoAppShell.getContext();
            final GeckoMediaScannerClient client = new GeckoMediaScannerClient(context, aFile, mimeType);
            client.connect();
        }
    }

    public static void removeDownload(final Download download) {
        if (!useSystemDownloadManager()) {
            return;
        }

        final DownloadManager dm = (DownloadManager) GeckoAppShell.getContext().getSystemService(Context.DOWNLOAD_SERVICE);

        Cursor c = null;
        try {
            c = dm.query((new DownloadManager.Query()).setFilterByStatus(DownloadManager.STATUS_SUCCESSFUL));
            if (c == null || !c.moveToFirst()) {
                return;
            }

            do {
                final Download d = Download.fromCursor(c);
                
                if (download.equals(d)) {
                    dm.remove(d.id);
                }
            } while(c.moveToNext());
        } finally {
            if (c != null) {
                c.close();
            }
        }
    }

    private static final class GeckoMediaScannerClient implements MediaScannerConnectionClient {
        private final String mFile;
        private final String mMimeType;
        private MediaScannerConnection mScanner;

        public GeckoMediaScannerClient(Context context, String file, String mimeType) {
            mFile = file;
            mMimeType = mimeType;
            mScanner = new MediaScannerConnection(context, this);
        }

        public void connect() {
            mScanner.connect();
        }

        @Override
        public void onMediaScannerConnected() {
            mScanner.scanFile(mFile, mMimeType);
        }

        @Override
        public void onScanCompleted(String path, Uri uri) {
            if(path.equals(mFile)) {
                mScanner.disconnect();
                mScanner = null;
            }
        }
    }
}

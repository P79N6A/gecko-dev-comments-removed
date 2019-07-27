




package org.mozilla.gecko;

import org.json.JSONException;
import org.json.JSONObject;

import org.mozilla.gecko.AppConstants.Versions;
import org.mozilla.gecko.mozglue.generatorannotations.WrapElementForJNI;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import android.app.DownloadManager;
import android.content.Context;
import android.database.Cursor;
import android.media.MediaScannerConnection;
import android.media.MediaScannerConnection.MediaScannerConnectionClient;
import android.net.Uri;
import android.text.TextUtils;
import android.util.Log;

public class DownloadsIntegration
{
    private static final String LOGTAG = "GeckoDownloadsIntegration";

    @SuppressWarnings("serial")
    private static final List<String> UNKNOWN_MIME_TYPES = new ArrayList<String>(3) {{
        add("unknown/unknown"); 
        add("application/unknown");
        add("application/octet-stream"); 
    }};

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

        if (AppConstants.ANDROID_DOWNLOADS_INTEGRATION) {
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

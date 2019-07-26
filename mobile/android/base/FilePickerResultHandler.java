



package org.mozilla.gecko;

import org.mozilla.gecko.mozglue.GeckoLoader;
import org.mozilla.gecko.util.ActivityResultHandler;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.provider.MediaStore;
import android.provider.OpenableColumns;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.LoaderManager;
import android.support.v4.app.LoaderManager.LoaderCallbacks;
import android.support.v4.content.CursorLoader;
import android.support.v4.content.Loader;
import android.text.format.Time;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.IOException;
import java.util.Queue;

class FilePickerResultHandler implements ActivityResultHandler {
    private static final String LOGTAG = "GeckoFilePickerResultHandler";

    protected final Queue<String> mFilePickerResult;
    protected final ActivityHandlerHelper.FileResultHandler mHandler;

    
    
    private String mImageName = "";

    public FilePickerResultHandler(Queue<String> resultQueue) {
        mFilePickerResult = resultQueue;
        mHandler = null;
    }

    
    public FilePickerResultHandler(ActivityHandlerHelper.FileResultHandler handler) {
        mFilePickerResult = null;
        mHandler = handler;
    }

    private void sendResult(String res) {
        if (mFilePickerResult != null)
            mFilePickerResult.offer(res);

        if (mHandler != null)
            mHandler.gotFile(res);
    }

    @Override
    public void onActivityResult(int resultCode, Intent intent) {
        if (resultCode != Activity.RESULT_OK) {
            sendResult("");
            return;
        }

        
        if (intent == null) {
            if (mImageName != null) {
                File file = new File(Environment.getExternalStorageDirectory(), mImageName);
                sendResult(file.getAbsolutePath());
            } else {
                sendResult("");
            }
            return;
        }

        Uri uri = intent.getData();
        if (uri == null) {
            sendResult("");
            return;
        }

        
        if ("file".equals(uri.getScheme())) {
            String path = uri.getPath();
            sendResult(path == null ? "" : path);
            return;
        }

        final FragmentActivity fa = (FragmentActivity) GeckoAppShell.getGeckoInterface().getActivity();
        final LoaderManager lm = fa.getSupportLoaderManager();
        
        try {
            
            final ContentResolver cr = fa.getContentResolver();
            Cursor cursor = cr.query(uri, new String[] { "MediaStore.Video.Media.DATA" }, null, null, null);
            cursor.close();

            lm.initLoader(intent.hashCode(), null, new VideoLoaderCallbacks(uri));
            return;
        } catch(Exception ex) { }

        lm.initLoader(uri.hashCode(), null, new FileLoaderCallbacks(uri));
        return;
    }

    public String generateImageName() {
        Time now = new Time();
        now.setToNow();
        mImageName = now.format("%Y-%m-%d %H.%M.%S") + ".jpg";
        return mImageName;
    }

    private class VideoLoaderCallbacks implements LoaderCallbacks<Cursor> {
        final private Uri mUri;
        public VideoLoaderCallbacks(Uri uri) {
            mUri = uri;
        }

        @Override
        public Loader<Cursor> onCreateLoader(int id, Bundle args) {
            final FragmentActivity fa = (FragmentActivity) GeckoAppShell.getGeckoInterface().getActivity();
            return new CursorLoader(fa,
                                    mUri,
                                    new String[] { "MediaStore.Video.Media.DATA" },
                                    null,  
                                    null,  
                                    null); 
        }

        @Override
        public void onLoadFinished(Loader<Cursor> loader, Cursor cursor) {
            if (cursor.moveToFirst()) {
                String res = cursor.getString(cursor.getColumnIndexOrThrow(MediaStore.Video.Media.DATA));
                sendResult(res);
            } else {
                final FragmentActivity fa = (FragmentActivity) GeckoAppShell.getGeckoInterface().getActivity();
                final LoaderManager lm = fa.getSupportLoaderManager();
                lm.initLoader(cursor.hashCode(), null, new FileLoaderCallbacks(mUri));
            }
        }

        @Override
        public void onLoaderReset(Loader<Cursor> loader) { }
    }

    private class FileLoaderCallbacks implements LoaderCallbacks<Cursor> {
        final private Uri mUri;
        public FileLoaderCallbacks(Uri uri) {
            mUri = uri;
        }

        @Override
        public Loader<Cursor> onCreateLoader(int id, Bundle args) {
            final FragmentActivity fa = (FragmentActivity) GeckoAppShell.getGeckoInterface().getActivity();
            return new CursorLoader(fa,
                                    mUri,
                                    new String[] { OpenableColumns.DISPLAY_NAME },
                                    null,  
                                    null,  
                                    null); 
        }

        @Override
        public void onLoadFinished(Loader<Cursor> loader, Cursor cursor) {
            if (cursor.moveToFirst()) {
                String name = cursor.getString(0);
                
                String fileName = "tmp_";
                String fileExt = null;
                int period;

                final FragmentActivity fa = (FragmentActivity) GeckoAppShell.getGeckoInterface().getActivity();
                final ContentResolver cr = fa.getContentResolver();

                
                if (name == null || (period = name.lastIndexOf('.')) == -1) {
                    String mimeType = cr.getType(mUri);
                    fileExt = "." + GeckoAppShell.getExtensionFromMimeType(mimeType);
                } else {
                    fileExt = name.substring(period);
                    fileName += name.substring(0, period);
                }

                
                try {
                    File file = File.createTempFile(fileName, fileExt, GeckoLoader.getGREDir(GeckoAppShell.getContext()));
                    FileOutputStream fos = new FileOutputStream(file);
                    InputStream is = cr.openInputStream(mUri);
                    byte[] buf = new byte[4096];
                    int len = is.read(buf);
                    while (len != -1) {
                        fos.write(buf, 0, len);
                        len = is.read(buf);
                    }
                    fos.close();

                    String path = file.getAbsolutePath();
                    sendResult((path == null) ? "" : path);
                } catch(IOException ex) {
                    Log.i(LOGTAG, "Error writing file", ex);
                }
            } else {
                sendResult("");
            }
        }

        @Override
        public void onLoaderReset(Loader<Cursor> loader) { }
    }

}


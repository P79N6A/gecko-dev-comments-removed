




package org.mozilla.gecko.preferences;

import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.db.BrowserContract.Bookmarks;
import org.mozilla.gecko.db.LocalBrowserDB;

import android.content.ContentProviderOperation;
import android.content.ContentResolver;
import android.content.Context;
import android.content.OperationApplicationException;
import android.database.Cursor;
import android.os.RemoteException;
import android.provider.Browser;
import android.util.Log;

import java.util.ArrayList;

class AndroidImport implements Runnable {
    static final private String LOGTAG = "AndroidImport";
    private Context mContext;
    private Runnable mOnDoneRunnable;
    private ArrayList<ContentProviderOperation> mOperations;
    private ContentResolver mCr;
    private LocalBrowserDB mDB;
    private boolean mImportBookmarks;
    private boolean mImportHistory;

    public AndroidImport(Context context, Runnable onDoneRunnable,
                         boolean doBookmarks, boolean doHistory) {
        mContext = context;
        mOnDoneRunnable = onDoneRunnable;
        mOperations = new ArrayList<ContentProviderOperation>();
        mCr = mContext.getContentResolver();
        mDB = new LocalBrowserDB(GeckoProfile.get(context).getName());
        mImportBookmarks = doBookmarks;
        mImportHistory = doHistory;
    }

    public void mergeBookmarks() {
        Cursor cursor = null;
        try {
            cursor = mCr.query(Browser.BOOKMARKS_URI,
                               null,
                               Browser.BookmarkColumns.BOOKMARK + " = 1",
                               null,
                               null);

            if (cursor != null) {
                final int faviconCol = cursor.getColumnIndexOrThrow(Browser.BookmarkColumns.FAVICON);
                final int titleCol = cursor.getColumnIndexOrThrow(Browser.BookmarkColumns.TITLE);
                final int urlCol = cursor.getColumnIndexOrThrow(Browser.BookmarkColumns.URL);
                
                final int createCol = cursor.getColumnIndex(Browser.BookmarkColumns.CREATED);

                cursor.moveToFirst();
                while (!cursor.isAfterLast()) {
                    String url = cursor.getString(urlCol);
                    String title = cursor.getString(titleCol);
                    long created;
                    if (createCol >= 0) {
                        created = cursor.getLong(createCol);
                    } else {
                        created = System.currentTimeMillis();
                    }
                    
                    long modified = System.currentTimeMillis();
                    byte[] data = cursor.getBlob(faviconCol);
                    mDB.updateBookmarkInBatch(mCr, mOperations,
                                              url, title, null, -1,
                                              created, modified,
                                              BrowserContract.Bookmarks.DEFAULT_POSITION,
                                              null, Bookmarks.TYPE_BOOKMARK);
                    if (data != null) {
                        mDB.updateFaviconInBatch(mCr, mOperations, url, null, null, data);
                    }
                    cursor.moveToNext();
                }
            }
        } finally {
            if (cursor != null)
                cursor.close();
        }

        flushBatchOperations();
    }

    public void mergeHistory() {
        Cursor cursor = null;
        try {
            cursor = mCr.query(Browser.BOOKMARKS_URI,
                               null,
                               Browser.BookmarkColumns.BOOKMARK + " = 0 AND " +
                               Browser.BookmarkColumns.VISITS + " > 0",
                               null,
                               null);

            if (cursor != null) {
                final int dateCol = cursor.getColumnIndexOrThrow(Browser.BookmarkColumns.DATE);
                final int faviconCol = cursor.getColumnIndexOrThrow(Browser.BookmarkColumns.FAVICON);
                final int titleCol = cursor.getColumnIndexOrThrow(Browser.BookmarkColumns.TITLE);
                final int urlCol = cursor.getColumnIndexOrThrow(Browser.BookmarkColumns.URL);
                final int visitsCol = cursor.getColumnIndexOrThrow(Browser.BookmarkColumns.VISITS);

                cursor.moveToFirst();
                while (!cursor.isAfterLast()) {
                    String url = cursor.getString(urlCol);
                    String title = cursor.getString(titleCol);
                    long date = cursor.getLong(dateCol);
                    int visits = cursor.getInt(visitsCol);
                    byte[] data = cursor.getBlob(faviconCol);
                    mDB.updateHistoryInBatch(mCr, mOperations, url, title, date, visits);
                    if (data != null) {
                        mDB.updateFaviconInBatch(mCr, mOperations, url, null, null, data);
                    }
                    cursor.moveToNext();
                }
            }
        } finally {
            if (cursor != null)
                cursor.close();
        }

        flushBatchOperations();
    }

    protected void flushBatchOperations() {
        Log.d(LOGTAG, "Flushing " + mOperations.size() + " DB operations");
        try {
            
            mCr.applyBatch(BrowserContract.AUTHORITY, mOperations);
        } catch (RemoteException e) {
            Log.e(LOGTAG, "Remote exception while updating db: ", e);
        } catch (OperationApplicationException e) {
            
            Log.d(LOGTAG, "Error while applying database updates: ", e);
        }
        mOperations.clear();
    }

    @Override
    public void run() {
        if (mImportBookmarks) {
            mergeBookmarks();
        }
        if (mImportHistory) {
            mergeHistory();
        }

        mOnDoneRunnable.run();
    }
}

package org.mozilla.gecko.tests;

import org.mozilla.gecko.*;
import org.mozilla.gecko.db.BrowserDB;

import android.app.Activity;
import android.content.ContentResolver;
import android.database.Cursor;
import android.net.Uri;
import android.test.ActivityInstrumentationTestCase2;
import java.util.ArrayList;

class DatabaseHelper {
    protected enum BrowserDataType {BOOKMARKS, HISTORY};
    private Activity mActivity;
    private Assert mAsserter;

    public DatabaseHelper(Activity activity, Assert asserter) {
        mActivity = activity;
        mAsserter = asserter;
    }
    


    protected boolean isBookmark(String url) {
        final ContentResolver resolver = mActivity.getContentResolver();
        return BrowserDB.isBookmark(resolver, url);
    }

    protected Uri buildUri(BrowserDataType dataType) {
        Uri uri = null;
        if (dataType == BrowserDataType.BOOKMARKS || dataType == BrowserDataType.HISTORY) {
            uri = Uri.parse("content://" + TestConstants.ANDROID_PACKAGE_NAME + ".db.browser/" + dataType.toString().toLowerCase());
        } else {
           mAsserter.ok(false, "The wrong data type has been provided = " + dataType.toString(), "Please provide the correct data type");
        }
        uri = uri.buildUpon().appendQueryParameter("profile", "default")
                             .appendQueryParameter("sync", "true").build();
        return uri;
    }

    




    protected void addOrUpdateMobileBookmark(String title, String url) {
        final ContentResolver resolver = mActivity.getContentResolver();
        BrowserDB.addBookmark(resolver, title, url);
        mAsserter.ok(true, "Inserting/updating a new bookmark", "Inserting/updating the bookmark with the title = " + title + " and the url = " + url);
    }

    




    protected void updateBookmark(String url, String title, String keyword) {
        final ContentResolver resolver = mActivity.getContentResolver();
        
        Cursor c = null;
        try {
            c = BrowserDB.getBookmarkForUrl(resolver, url);
            if (!c.moveToFirst()) {
                mAsserter.ok(false, "Getting bookmark with url", "Couldn't find bookmark with url = " + url);
                return;
            }

            int id = c.getInt(c.getColumnIndexOrThrow("_id"));
            BrowserDB.updateBookmark(resolver, id, url, title, keyword);

            mAsserter.ok(true, "Updating bookmark", "Updating bookmark with url = " + url);
        } finally {
            if (c != null) {
                c.close();
            }
        }
    }

    protected void deleteBookmark(String url) {
        final ContentResolver resolver = mActivity.getContentResolver();
        BrowserDB.removeBookmarksWithURL(resolver, url);
    }

    protected void deleteHistoryItem(String url) {
        final ContentResolver resolver = mActivity.getContentResolver();
        BrowserDB.removeHistoryEntry(resolver, url);
    }

    
    protected long getFolderIdFromGuid(String guid) {
        ContentResolver resolver = mActivity.getContentResolver();
        long folderId = Long.valueOf(-1);
        Uri bookmarksUri = buildUri(BrowserDataType.BOOKMARKS);
        Cursor c = null;
        try {
            c = resolver.query(bookmarksUri,
                               new String[] { "_id" },
                               "guid = ?",
                               new String[] { guid },
                               null);
            if (c.moveToFirst()) {
                folderId = c.getLong(c.getColumnIndexOrThrow("_id"));
            }
            if (folderId == -1) {
                mAsserter.ok(false, "Trying to get the folder id" ,"We did not get the correct folder id");
            }
        } finally {
            if (c != null) {
                c.close();
            }
        }
        return folderId;
    }

     



    protected ArrayList<String> getBrowserDBUrls(BrowserDataType dataType) {
        ArrayList<String> browserData = new ArrayList<String>();
        ContentResolver resolver = mActivity.getContentResolver();
        Cursor cursor = null;
        Uri uri = buildUri(dataType);
        if (dataType == BrowserDataType.HISTORY) {
            cursor = BrowserDB.getAllVisitedHistory(resolver);
        } else if (dataType == BrowserDataType.BOOKMARKS) {
            cursor = BrowserDB.getBookmarksInFolder(resolver, getFolderIdFromGuid("mobile"));
        }
        if (cursor != null) {
            cursor.moveToFirst();
            for (int i = 0; i < cursor.getCount(); i++ ) {
                 
                if (cursor.getString(cursor.getColumnIndex("url")) != null) {
                    browserData.add(cursor.getString(cursor.getColumnIndex("url")));
                }
                if(!cursor.isLast()) {
                    cursor.moveToNext();
                }
            }
        } else {
             mAsserter.ok(false, "We could not retrieve any data from the database", "The cursor was null");
        }
        if (cursor != null) {
            cursor.close();
        }
        return browserData;
    }
}

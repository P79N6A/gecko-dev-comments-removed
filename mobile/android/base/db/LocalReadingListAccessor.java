



package org.mozilla.gecko.db;

import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.database.ContentObserver;
import android.database.Cursor;
import android.net.Uri;
import android.util.Log;
import org.mozilla.gecko.AboutPages;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.ReaderModeUtils;
import org.mozilla.gecko.db.BrowserContract.ReadingListItems;
import org.mozilla.gecko.mozglue.RobocopTarget;


@RobocopTarget
public class LocalReadingListAccessor implements ReadingListAccessor {
    private static final String LOG_TAG = "GeckoReadingListAcc";

    private static final String NOT_DELETED = ReadingListItems.IS_DELETED + " = 0";
    private static final String NEITHER_DELETED_NOR_ARCHIVED = ReadingListItems.IS_ARCHIVED + " = 0 AND " + ReadingListItems.IS_DELETED + " = 0";
    private static final String ITEMS_TO_FETCH = ReadingListItems.CONTENT_STATUS + " = " + ReadingListItems.STATUS_UNFETCHED + " AND " + NEITHER_DELETED_NOR_ARCHIVED;
    private static final String SORT_ORDER_RECENT_FIRST = "COALESCE(" + ReadingListItems.SERVER_STORED_ON + ", " + ReadingListItems.ADDED_ON + ") DESC";

    private final Uri mReadingListUriWithProfile;

    public LocalReadingListAccessor(final String profile) {
        mReadingListUriWithProfile = DBUtils.appendProfile(profile, ReadingListItems.CONTENT_URI);
    }

    
    @Override
    public int getCount(ContentResolver cr) {
        final String[] columns = new String[]{ReadingListItems._ID};
        final Cursor cursor = cr.query(mReadingListUriWithProfile, columns, NOT_DELETED, null, null);

        try {
            return cursor.getCount();
        } finally {
            cursor.close();
        }
    }

    @Override
    public Cursor getReadingList(ContentResolver cr) {
        
        
        
        
        return cr.query(mReadingListUriWithProfile,
                        ReadingListItems.DEFAULT_PROJECTION,
                        NEITHER_DELETED_NOR_ARCHIVED,
                        null,
                        SORT_ORDER_RECENT_FIRST);
    }

    @Override
    public Cursor getReadingListUnfetched(ContentResolver cr) {
        
        
        return cr.query(mReadingListUriWithProfile,
                        new String[] { ReadingListItems._ID, ReadingListItems.URL },
                        ITEMS_TO_FETCH,
                        null,
                        SORT_ORDER_RECENT_FIRST);
    }

    @Override
    public boolean isReadingListItem(final ContentResolver cr, String uri) {
        uri = stripURI(uri);

        final Cursor c = cr.query(mReadingListUriWithProfile,
                                  new String[] { ReadingListItems._ID },
                                  ReadingListItems.URL + " = ? OR " + ReadingListItems.RESOLVED_URL + " = ?",
                                  new String[] { uri, uri },
                                  null);

        if (c == null) {
            Log.e(LOG_TAG, "Null cursor in isReadingListItem");
            return false;
        }

        try {
            return c.moveToNext();
        } finally {
            c.close();
        }
    }


    @Override
    public long addReadingListItem(ContentResolver cr, ContentValues values) {
        
        for (String field: ReadingListItems.REQUIRED_FIELDS) {
            if (!values.containsKey(field)) {
                throw new IllegalArgumentException("Missing required field for reading list item: " + field);
            }
        }

        
        final String url = stripURI(values.getAsString(ReadingListItems.URL));
        values.put(ReadingListItems.URL, url);

        
        values.put(ReadingListItems.ADDED_ON, System.currentTimeMillis());
        values.put(ReadingListItems.ADDED_BY, ReadingListProvider.PLACEHOLDER_THIS_DEVICE);

        
        
        final long id = ContentUris.parseId(cr.insert(mReadingListUriWithProfile, values));

        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Reader:Added", url));

        return id;
    }

    @Override
    public long addBasicReadingListItem(ContentResolver cr, String url, String title) {
        if (url == null) {
            throw new IllegalArgumentException("URL must not be null.");
        }
        final ContentValues values = new ContentValues();
        values.put(ReadingListItems.URL, url);
        if (title != null) {
            values.put(ReadingListItems.TITLE, title);
        } else {
            values.putNull(ReadingListItems.TITLE);
        }

        return addReadingListItem(cr, values);
    }

    @Override
    public void updateReadingListItem(ContentResolver cr, ContentValues values) {
        if (!values.containsKey(ReadingListItems._ID)) {
            throw new IllegalArgumentException("Cannot update reading list item without an ID");
        }

        if (values.containsKey(ReadingListItems.URL)) {
            values.put(ReadingListItems.URL, stripURI(values.getAsString(ReadingListItems.URL)));
        }

        final int updated = cr.update(mReadingListUriWithProfile,
                                      values,
                                      ReadingListItems._ID + " = ? ",
                                      new String[] { values.getAsString(ReadingListItems._ID) });

        Log.d(LOG_TAG, "Updated " + updated + " reading list rows.");
    }

    @Override
    public void removeReadingListItemWithURL(final ContentResolver cr, String uri) {
        cr.delete(mReadingListUriWithProfile,
                  ReadingListItems.URL + " = ? OR " + ReadingListItems.RESOLVED_URL + " = ?",
                  new String[]{ uri, uri });

        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Reader:Removed", uri));
    }

    @Override
    public void deleteItem(ContentResolver cr, long itemID) {
        
        
        
        cr.delete(ContentUris.appendId(mReadingListUriWithProfile.buildUpon(), itemID).build(),
                  null, null);
    }

    @Override
    public void registerContentObserver(Context context, ContentObserver observer) {
        context.getContentResolver().registerContentObserver(mReadingListUriWithProfile, false, observer);
    }

    @Override
    public void markAsRead(ContentResolver cr, long itemID) {
        final ContentValues values = new ContentValues();
        values.put(ReadingListItems.MARKED_READ_BY, ReadingListProvider.PLACEHOLDER_THIS_DEVICE);
        values.put(ReadingListItems.MARKED_READ_ON, System.currentTimeMillis());
        values.put(ReadingListItems.IS_UNREAD, 0);

        
        cr.update(mReadingListUriWithProfile, values, ReadingListItems._ID + " = " + itemID, null);
    }

    @Override
    public void updateContent(ContentResolver cr, long itemID, String resolvedTitle, String resolvedURL, String excerpt) {
        final ContentValues values = new ContentValues();
        values.put(ReadingListItems.CONTENT_STATUS, ReadingListItems.STATUS_FETCHED_ARTICLE);
        values.put(ReadingListItems.RESOLVED_URL, resolvedURL);
        values.put(ReadingListItems.RESOLVED_TITLE, resolvedTitle);
        values.put(ReadingListItems.EXCERPT, excerpt);

        
        cr.update(mReadingListUriWithProfile, values, ReadingListItems._ID + " = " + itemID, null);
    }

    


    private String stripURI(final String uri) {
        return !AboutPages.isAboutReader(uri) ? uri : ReaderModeUtils.getUrlFromAboutReader(uri);
    }
}

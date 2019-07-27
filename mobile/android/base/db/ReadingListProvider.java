



package org.mozilla.gecko.db;

import android.content.ContentUris;
import android.content.ContentValues;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteQueryBuilder;
import android.net.Uri;
import android.text.TextUtils;
import android.util.Log;
import org.mozilla.gecko.db.DBUtils.UpdateOperation;

import static org.mozilla.gecko.db.BrowserContract.ReadingListItems.*;

public class ReadingListProvider extends SharedBrowserDatabaseProvider {
    private static final String LOGTAG = "GeckoRLProvider";

    static final String TABLE_READING_LIST = TABLE_NAME;

    static final int ITEMS = 101;
    static final int ITEMS_ID = 102;
    static final UriMatcher URI_MATCHER = new UriMatcher(UriMatcher.NO_MATCH);

    public static final String PLACEHOLDER_THIS_DEVICE = "$local";

    static {
        URI_MATCHER.addURI(BrowserContract.READING_LIST_AUTHORITY, "items", ITEMS);
        URI_MATCHER.addURI(BrowserContract.READING_LIST_AUTHORITY, "items/#", ITEMS_ID);
    }

    








    public int updateOrInsertItem(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
        if (!values.containsKey(CLIENT_LAST_MODIFIED)) {
            values.put(CLIENT_LAST_MODIFIED, System.currentTimeMillis());
        }

        if (isCallerSync(uri)) {
            int updated = updateItemsWithFlags(uri, values, null, selection, selectionArgs);
            if (updated > 0) {
                return updated;
            }
            return insertItem(uri, values) != -1 ? 1 : 0;
        }

        
        final ContentValues flags = processChangeValues(values);

        int updated = updateItemsWithFlags(uri, values, flags, selection, selectionArgs);
        if (updated <= 0) {
            
            values.put(SYNC_STATUS, SYNC_STATUS_NEW);
            values.put(SYNC_CHANGE_FLAGS, SYNC_CHANGE_NONE);
            updated = insertItem(uri, values) != -1 ? 1 : 0;
        }
        return updated;
    }

    








    private ContentValues processChangeValues(ContentValues values) {
        if (values == null || values.size() == 0) {
            return null;
        }

        final ContentValues out = new ContentValues();
        int flag = 0;
        if (values.containsKey(MARKED_READ_BY) ||
            values.containsKey(MARKED_READ_ON) ||
            values.containsKey(IS_UNREAD)) {
            flag |= SYNC_CHANGE_UNREAD_CHANGED;
        }

        if (values.containsKey(IS_FAVORITE)) {
            flag |= SYNC_CHANGE_FAVORITE_CHANGED;
        }

        if (values.containsKey(RESOLVED_URL) ||
            values.containsKey(RESOLVED_TITLE) ||
            values.containsKey(EXCERPT)) {
            flag |= SYNC_CHANGE_RESOLVED;
        }

        if (flag == 0) {
            return null;
        }

        out.put(SYNC_CHANGE_FLAGS, flag);
        return out;
    }

    




    public int updateItemsWithFlags(Uri uri, ContentValues values, ContentValues flags, String selection, String[] selectionArgs) {
        trace("Updating ReadingListItems on URI: " + uri);
        final SQLiteDatabase db = getWritableDatabase(uri);
        if (!values.containsKey(CLIENT_LAST_MODIFIED)) {
            values.put(CLIENT_LAST_MODIFIED, System.currentTimeMillis());
        }

        if (flags == null) {
            
            return db.update(TABLE_READING_LIST, values, selection, selectionArgs);
        }

        
        final ContentValues setModified = new ContentValues();
        setModified.put(SYNC_STATUS, "CASE " + SYNC_STATUS +
                                     " WHEN " + SYNC_STATUS_SYNCED +
                                     " THEN " + SYNC_STATUS_MODIFIED +
                                     " ELSE " + SYNC_STATUS +
                                     " END");

        final ContentValues[] valuesAndFlags = {values, flags, setModified};
        final UpdateOperation[] ops = {UpdateOperation.ASSIGN, UpdateOperation.BITWISE_OR, UpdateOperation.EXPRESSION};

        return DBUtils.updateArrays(db, TABLE_READING_LIST, valuesAndFlags, ops, selection, selectionArgs);
    }

    







    private long insertItem(Uri uri, ContentValues values) {
        if (!values.containsKey(CLIENT_LAST_MODIFIED)) {
            values.put(CLIENT_LAST_MODIFIED, System.currentTimeMillis());
        }

        
        if (!isCallerSync(uri)) {
            values.put(SYNC_STATUS, SYNC_STATUS_NEW);
            if (!values.containsKey(ADDED_ON)) {
                values.put(ADDED_ON, System.currentTimeMillis());
            }
            if (!values.containsKey(ADDED_BY)) {
                values.put(ADDED_BY, PLACEHOLDER_THIS_DEVICE);
            }
        }

        final String url = values.getAsString(URL);
        debug("Inserting item in database with URL: " + url);
        try {
            return getWritableDatabase(uri).insertOrThrow(TABLE_READING_LIST, null, values);
        } catch (SQLException e) {
            Log.e(LOGTAG, "Insert failed.", e);
            throw e;
        }
    }

    private static final ContentValues DELETED_VALUES;
    static {
        final ContentValues values = new ContentValues();
        values.put(IS_DELETED, 1);

        values.put(URL, "");             
        values.putNull(RESOLVED_URL);
        values.putNull(RESOLVED_TITLE);
        values.putNull(TITLE);
        values.putNull(EXCERPT);
        values.putNull(ADDED_BY);
        values.putNull(MARKED_READ_BY);

        
        values.put(SYNC_STATUS, SYNC_STATUS_DELETED);
        values.put(SYNC_CHANGE_FLAGS, SYNC_CHANGE_NONE);
        DELETED_VALUES = values;
    }

    






    int deleteItems(final Uri uri, String selection, String[] selectionArgs) {
        debug("Deleting item entry for URI: " + uri);
        final SQLiteDatabase db = getWritableDatabase(uri);

        
        if (isCallerSync(uri)) {
            debug("Directly deleting from reading list.");
            return db.delete(TABLE_READING_LIST, selection, selectionArgs);
        }

        
        
        
        int total = 0;
        final String whereNullGUID = DBUtils.concatenateWhere(selection, GUID + " IS NULL");
        final String whereNotNullGUID = DBUtils.concatenateWhere(selection, GUID + " IS NOT NULL");

        total += db.delete(TABLE_READING_LIST, whereNullGUID, selectionArgs);
        total += updateItemsWithFlags(uri, DELETED_VALUES, null, whereNotNullGUID, selectionArgs);

        return total;
    }

    int deleteItemByID(final Uri uri, long id) {
        debug("Deleting item entry for ID: " + id);
        final SQLiteDatabase db = getWritableDatabase(uri);

        
        if (isCallerSync(uri)) {
            debug("Directly deleting from reading list.");
            final String selection = _ID + " = " + id;
            return db.delete(TABLE_READING_LIST, selection, null);
        }

        
        
        final String whereNullGUID = _ID + " = " + id + " AND " + GUID + " IS NULL";
        final int raw = db.delete(TABLE_READING_LIST, whereNullGUID, null);
        if (raw > 0) {
            
            
            return raw;
        }

        
        final String whereNotNullGUID = _ID + " = " + id + " AND " + GUID + " IS NOT NULL";
        final ContentValues values = new ContentValues(DELETED_VALUES);
        values.put(CLIENT_LAST_MODIFIED, System.currentTimeMillis());
        return updateItemsWithFlags(uri, values, null, whereNotNullGUID, null);
    }

    @Override
    @SuppressWarnings("fallthrough")
    public int updateInTransaction(final Uri uri, ContentValues values, String selection, String[] selectionArgs) {
        trace("Calling update in transaction on URI: " + uri);

        int updated = 0;
        int match = URI_MATCHER.match(uri);

        switch (match) {
            case ITEMS_ID:
                debug("Update on ITEMS_ID: " + uri);
                selection = DBUtils.concatenateWhere(selection, TABLE_READING_LIST + "._id = ?");
                selectionArgs = DBUtils.appendSelectionArgs(selectionArgs,
                        new String[] { Long.toString(ContentUris.parseId(uri)) });

            case ITEMS: {
                debug("Updating ITEMS: " + uri);
                if (shouldUpdateOrInsert(uri)) {
                    
                    updated = updateOrInsertItem(uri, values, selection, selectionArgs);
                } else {
                    
                    ContentValues flags = isCallerSync(uri) ? null : processChangeValues(values);
                    updated = updateItemsWithFlags(uri, values, flags, selection, selectionArgs);
                }
                break;
            }

            default:
                throw new UnsupportedOperationException("Unknown update URI " + uri);
        }

        debug("Updated " + updated + " rows for URI: " + uri);
        return updated;
    }


    @Override
    @SuppressWarnings("fallthrough")
    public int deleteInTransaction(Uri uri, String selection, String[] selectionArgs) {
        trace("Calling delete in transaction on URI: " + uri);

        
        
        cleanUpSomeDeletedRecords(uri, TABLE_READING_LIST);

        int numDeleted = 0;
        int match = URI_MATCHER.match(uri);

        switch (match) {
            case ITEMS_ID:
                debug("Deleting on ITEMS_ID: " + uri);
                numDeleted = deleteItemByID(uri, ContentUris.parseId(uri));
                break;

            case ITEMS:
                debug("Deleting ITEMS: " + uri);
                numDeleted = deleteItems(uri, selection, selectionArgs);
                break;

            default:
                throw new UnsupportedOperationException("Unknown update URI " + uri);
        }

        debug("Deleted " + numDeleted + " rows for URI: " + uri);
        return numDeleted;
    }

    @Override
    public Uri insertInTransaction(Uri uri, ContentValues values) {
        trace("Calling insert in transaction on URI: " + uri);
        long id = -1;
        int match = URI_MATCHER.match(uri);

        switch (match) {
            case ITEMS:
                trace("Insert on ITEMS: " + uri);
                id = insertItem(uri, values);
                break;

            default:
                
                Log.e(LOGTAG, "Unknown insert URI " + uri);
                throw new UnsupportedOperationException("Unknown insert URI " + uri);
        }

        debug("Inserted ID in database: " + id);

        if (id >= 0) {
            return ContentUris.withAppendedId(uri, id);
        }

        Log.e(LOGTAG, "Got to end of insertInTransaction without returning an id!");
        return null;
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder) {
        String groupBy = null;
        SQLiteDatabase db = getReadableDatabase(uri);
        SQLiteQueryBuilder qb = new SQLiteQueryBuilder();
        String limit = uri.getQueryParameter(BrowserContract.PARAM_LIMIT);

        final int match = URI_MATCHER.match(uri);
        switch (match) {
            case ITEMS_ID:
                trace("Query on ITEMS_ID: " + uri);
                selection = DBUtils.concatenateWhere(selection, _ID + " = ?");
                selectionArgs = DBUtils.appendSelectionArgs(selectionArgs,
                        new String[] { Long.toString(ContentUris.parseId(uri)) });

            case ITEMS:
                trace("Query on ITEMS: " + uri);
                if (!shouldShowDeleted(uri)) {
                    selection = DBUtils.concatenateWhere(IS_DELETED + " = 0", selection);
                }
                break;

            default:
                throw new UnsupportedOperationException("Unknown query URI " + uri);
        }

        if (TextUtils.isEmpty(sortOrder)) {
            sortOrder = DEFAULT_SORT_ORDER;
        }

        trace("Running built query.");
        qb.setTables(TABLE_READING_LIST);
        Cursor cursor = qb.query(db, projection, selection, selectionArgs, groupBy, null, sortOrder, limit);
        cursor.setNotificationUri(getContext().getContentResolver(), uri);

        return cursor;
    }

    @Override
    public String getType(Uri uri) {
        trace("Getting URI type: " + uri);

        final int match = URI_MATCHER.match(uri);
        switch (match) {
            case ITEMS:
                trace("URI is ITEMS: " + uri);
                return CONTENT_TYPE;

            case ITEMS_ID:
                trace("URI is ITEMS_ID: " + uri);
                return CONTENT_ITEM_TYPE;
        }

        debug("URI has unrecognized type: " + uri);
        return null;
    }

    @Override
    protected String getDeletedItemSelection(long earlierThan) {
        if (earlierThan == -1L) {
            return IS_DELETED + " = 1";
        }
        return IS_DELETED + " = 1 AND " + CLIENT_LAST_MODIFIED + " <= " + earlierThan;
    }
}

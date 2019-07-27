



package org.mozilla.gecko.db;

import org.mozilla.gecko.db.BrowserContract.SearchHistory;

import android.content.ContentUris;
import android.content.ContentValues;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.net.Uri;
import android.text.TextUtils;

public class SearchHistoryProvider extends SharedBrowserDatabaseProvider {

    


    private String stripWhitespace(String query) {
        if (TextUtils.isEmpty(query)) {
            return "";
        }

        
        return query.trim().replaceAll("\\s+", " ");
    }


    @Override
    public Uri insertInTransaction(Uri uri, ContentValues cv) {
        final String query = stripWhitespace(cv.getAsString(SearchHistory.QUERY));

        
        if (TextUtils.isEmpty(query)) {
            return null;
        }

        final SQLiteDatabase db = getWritableDatabase(uri);

        


        final String sql = "UPDATE " + SearchHistory.TABLE_NAME + " SET " +
                SearchHistory.VISITS + " = " + SearchHistory.VISITS + " + 1, " +
                SearchHistory.DATE_LAST_VISITED + " = " + System.currentTimeMillis() +
                " WHERE " + SearchHistory.QUERY + " = ?";
        final Cursor c = db.rawQuery(sql, new String[] { query });

        try {
            if (c.getCount() > 1) {
                
                
                return null;
            }
            if (c.moveToFirst()) {
                return ContentUris
                    .withAppendedId(uri, c.getInt(c.getColumnIndex(SearchHistory._ID)));
            }
        } finally {
            c.close();
        }

        


        cv.put(SearchHistory.QUERY, query);
        cv.put(SearchHistory.VISITS, 1);
        cv.put(SearchHistory.DATE_LAST_VISITED, System.currentTimeMillis());

        long id = db.insert(SearchHistory.TABLE_NAME, null, cv);

        if (id < 0) {
            return null;
        }

        return ContentUris.withAppendedId(uri, id);
    }

    @Override
    public int deleteInTransaction(Uri uri, String selection, String[] selectionArgs) {
        return getWritableDatabase(uri)
                .delete(SearchHistory.TABLE_NAME, selection, selectionArgs);
    }

    



    @Override
    public int updateInTransaction(Uri uri, ContentValues values, String selection,
            String[] selectionArgs) {
        throw new UnsupportedOperationException(
                "This content provider does not support updating items");
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection,
            String[] selectionArgs, String sortOrder) {
        String groupBy = null;
        String having = null;
        return getReadableDatabase(uri)
                .query(SearchHistory.TABLE_NAME, projection, selection, selectionArgs,
                        groupBy, having, sortOrder);
    }

    @Override
    public String getType(Uri uri) {
        return SearchHistory.CONTENT_TYPE;
    }
}

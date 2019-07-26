



package org.mozilla.gecko.db;

import org.mozilla.gecko.R;
import org.mozilla.gecko.db.BrowserContract.HomeListItems;
import org.mozilla.gecko.db.PerProfileDatabases.DatabaseHelperFactory;
import org.mozilla.gecko.sqlite.SQLiteBridge;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.content.ContentValues;
import android.content.Context;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.database.MatrixCursor.RowBuilder;
import android.net.Uri;
import android.os.Build;
import android.text.TextUtils;
import android.util.Log;

import java.io.InputStream;
import java.io.IOException;

public class HomeListsProvider extends SQLiteBridgeContentProvider {
    private static final String LOGTAG = "GeckoHomeListsProvider";

    
    private static int DB_VERSION = 1;
    private static String DB_FILENAME = "home.sqlite";

    private static final String TABLE_ITEMS = "items";

    
    static final int ITEMS_FAKE = 100;
    static final int ITEMS = 101;
    static final int ITEMS_ID = 102;

    static final UriMatcher URI_MATCHER = new UriMatcher(UriMatcher.NO_MATCH);

    static {
        URI_MATCHER.addURI(BrowserContract.HOME_LISTS_AUTHORITY, "items/fake", ITEMS_FAKE);
        URI_MATCHER.addURI(BrowserContract.HOME_LISTS_AUTHORITY, "items", ITEMS);
        URI_MATCHER.addURI(BrowserContract.HOME_LISTS_AUTHORITY, "items/#", ITEMS_ID);
    }

    public HomeListsProvider() {
        super(LOGTAG);
    }

    @Override
    public String getType(Uri uri) {
        final int match = URI_MATCHER.match(uri);

        switch (match) {
            case ITEMS_FAKE: {
                return HomeListItems.CONTENT_TYPE;
            }
            case ITEMS: {
                return HomeListItems.CONTENT_TYPE;
            }
            default: {
                throw new UnsupportedOperationException("Unknown type " + uri);
            }
        }
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder) {
        final int match = URI_MATCHER.match(uri);

        
        if (match == ITEMS_FAKE) {
            return queryFakeItems(uri, projection, selection, selectionArgs, sortOrder);
        }

        
        return super.query(uri, projection, selection, selectionArgs, sortOrder);
    }

    


    private Cursor queryFakeItems(Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder) {
        JSONArray items = null;
        try {
            items = new JSONArray(getRawFakeItems());
        } catch (IOException e) {
            Log.e(LOGTAG, "Error getting fake list items", e);
            return null;
        } catch (JSONException e) {
            Log.e(LOGTAG, "Error parsing fake-list-items.json", e);
            return null;
        }

        final String[] itemsColumns = new String[] {
            HomeListItems._ID,
            HomeListItems.PROVIDER_ID,
            HomeListItems.URL,
            HomeListItems.TITLE
        };

        final MatrixCursor c = new MatrixCursor(itemsColumns);
        for (int i = 0; i < items.length(); i++) {
            try {
                final JSONObject item = items.getJSONObject(i);
                c.addRow(new Object[] {
                    item.getInt("id"),
                    item.getString("provider_id"),
                    item.getString("url"),
                    item.getString("title")
                });
            } catch (JSONException e) {
                Log.e(LOGTAG, "Error creating cursor row for fake list item", e);
            }
        }
        return c;
    }

    private String getRawFakeItems() throws IOException {
        final InputStream inputStream = getContext().getResources().openRawResource(R.raw.fake_list_items);
        final byte[] buffer = new byte[1024];
        StringBuilder s = new StringBuilder();
        int count;

        while ((count = inputStream.read(buffer)) != -1) {
            s.append(new String(buffer, 0, count));
        }
        return s.toString();
    }

    



    @Override
    protected String getDBName(){
        return DB_FILENAME;
    }

    @Override
    protected int getDBVersion(){
        return DB_VERSION;
    }

    @Override
    public String getTable(Uri uri) {
        final int match = URI_MATCHER.match(uri);
        switch (match) {
            case ITEMS: {
                return TABLE_ITEMS;
            }
            default: {
                throw new UnsupportedOperationException("Unknown table " + uri);
            }
        }
    }

    @Override
    public String getSortOrder(Uri uri, String aRequested) {
        return null;
    }

    @Override
    public void setupDefaults(Uri uri, ContentValues values) { }

    @Override
    public void initGecko() { }

    @Override
    public void onPreInsert(ContentValues values, Uri uri, SQLiteBridge db) { }

    @Override
    public void onPreUpdate(ContentValues values, Uri uri, SQLiteBridge db) { }

    @Override
    public void onPostQuery(Cursor cursor, Uri uri, SQLiteBridge db) { }


}

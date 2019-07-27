





package org.mozilla.gecko.db;

import org.mozilla.gecko.db.BrowserContract.Bookmarks;
import org.mozilla.gecko.db.BrowserContract.History;

import android.database.sqlite.SQLiteDatabase;
import android.net.Uri;


public class URLMetadataTable extends BaseTable {
    private static final String LOGTAG = "GeckoURLMetadataTable";

    private static final String TABLE = "metadata"; 
    private static final int TABLE_ID_NUMBER = 1200;

    
    static final Uri CONTENT_URI = Uri.withAppendedPath(BrowserContract.AUTHORITY_URI, "metadata");

    
    public static final String ID_COLUMN = "id";
    public static final String URL_COLUMN = "url";
    public static final String TILE_IMAGE_URL_COLUMN = "tileImage";
    public static final String TILE_COLOR_COLUMN = "tileColor";

    URLMetadataTable() { }

    @Override
    protected String getTable() {
        return TABLE;
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
        String create = "CREATE TABLE " + TABLE + " (" +
            ID_COLUMN + " INTEGER PRIMARY KEY, " +
            URL_COLUMN + " TEXT NON NULL UNIQUE, " +
            TILE_IMAGE_URL_COLUMN + " STRING, " +
            TILE_COLOR_COLUMN + " STRING);";
        db.execSQL(create);

        db.execSQL("CREATE INDEX metadata_url_idx ON " + TABLE + " (" + URL_COLUMN + ")");
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        
        if (newVersion >= 21 && oldVersion < 21) {
            onCreate(db);
        }
    }

    @Override
    public Table.ContentProviderInfo[] getContentProviderInfo() {
        return new Table.ContentProviderInfo[] {
            new Table.ContentProviderInfo(TABLE_ID_NUMBER, TABLE)
        };
    }

    public int deleteUnused(final SQLiteDatabase db) {
        final String selection = URL_COLUMN + " NOT IN " +
                                 "(SELECT " + History.URL +
                                 " FROM " + History.TABLE_NAME +
                                 " WHERE " + History.IS_DELETED + " = 0" +
                                 " UNION " +
                                 " SELECT " + Bookmarks.URL +
                                 " FROM " + Bookmarks.TABLE_NAME +
                                 " WHERE " + Bookmarks.IS_DELETED + " = 0 " +
                                 " AND " + Bookmarks.URL + " IS NOT NULL)";

        return db.delete(getTable(), selection, null);
    }
}

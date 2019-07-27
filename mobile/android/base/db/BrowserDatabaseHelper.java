




package org.mozilla.gecko.db;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.R;
import org.mozilla.gecko.db.BrowserContract.Bookmarks;
import org.mozilla.gecko.db.BrowserContract.Combined;
import org.mozilla.gecko.db.BrowserContract.Favicons;
import org.mozilla.gecko.db.BrowserContract.History;
import org.mozilla.gecko.db.BrowserContract.ReadingListItems;
import org.mozilla.gecko.db.BrowserContract.SearchHistory;
import org.mozilla.gecko.db.BrowserContract.Thumbnails;
import org.mozilla.gecko.util.FileUtils;

import static org.mozilla.gecko.db.DBUtils.qualifyColumn;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.DatabaseUtils;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteException;
import android.database.sqlite.SQLiteOpenHelper;
import android.net.Uri;
import android.os.Build;
import android.util.Log;


final class BrowserDatabaseHelper extends SQLiteOpenHelper {
    private static final String LOGTAG = "GeckoBrowserDBHelper";

    public static final int DATABASE_VERSION = 24;
    public static final String DATABASE_NAME = "browser.db";

    final protected Context mContext;

    static final String TABLE_BOOKMARKS = Bookmarks.TABLE_NAME;
    static final String TABLE_HISTORY = History.TABLE_NAME;
    static final String TABLE_FAVICONS = Favicons.TABLE_NAME;
    static final String TABLE_THUMBNAILS = Thumbnails.TABLE_NAME;
    static final String TABLE_READING_LIST = ReadingListItems.TABLE_NAME;
    static final String TABLE_TABS = TabsProvider.TABLE_TABS;
    static final String TABLE_CLIENTS = TabsProvider.TABLE_CLIENTS;

    static final String VIEW_COMBINED = Combined.VIEW_NAME;
    static final String VIEW_BOOKMARKS_WITH_FAVICONS = Bookmarks.VIEW_WITH_FAVICONS;
    static final String VIEW_HISTORY_WITH_FAVICONS = History.VIEW_WITH_FAVICONS;
    static final String VIEW_COMBINED_WITH_FAVICONS = Combined.VIEW_WITH_FAVICONS;

    static final String TABLE_BOOKMARKS_JOIN_FAVICONS = TABLE_BOOKMARKS + " LEFT OUTER JOIN " +
            TABLE_FAVICONS + " ON " + qualifyColumn(TABLE_BOOKMARKS, Bookmarks.FAVICON_ID) + " = " +
            qualifyColumn(TABLE_FAVICONS, Favicons._ID);

    static final String TABLE_HISTORY_JOIN_FAVICONS = TABLE_HISTORY + " LEFT OUTER JOIN " +
            TABLE_FAVICONS + " ON " + qualifyColumn(TABLE_HISTORY, History.FAVICON_ID) + " = " +
            qualifyColumn(TABLE_FAVICONS, Favicons._ID);

    static final String TABLE_BOOKMARKS_TMP = TABLE_BOOKMARKS + "_tmp";
    static final String TABLE_HISTORY_TMP = TABLE_HISTORY + "_tmp";

    private static final String[] mobileIdColumns = new String[] { Bookmarks._ID };
    private static final String[] mobileIdSelectionArgs = new String[] { Bookmarks.MOBILE_FOLDER_GUID };

    public BrowserDatabaseHelper(Context context, String databasePath) {
        super(context, databasePath, null, DATABASE_VERSION);
        mContext = context;
    }

    private void createBookmarksTable(SQLiteDatabase db) {
        debug("Creating " + TABLE_BOOKMARKS + " table");

        db.execSQL("CREATE TABLE " + TABLE_BOOKMARKS + "(" +
                Bookmarks._ID + " INTEGER PRIMARY KEY AUTOINCREMENT," +
                Bookmarks.TITLE + " TEXT," +
                Bookmarks.URL + " TEXT," +
                Bookmarks.TYPE + " INTEGER NOT NULL DEFAULT " + Bookmarks.TYPE_BOOKMARK + "," +
                Bookmarks.PARENT + " INTEGER," +
                Bookmarks.POSITION + " INTEGER NOT NULL," +
                Bookmarks.KEYWORD + " TEXT," +
                Bookmarks.DESCRIPTION + " TEXT," +
                Bookmarks.TAGS + " TEXT," +
                Bookmarks.FAVICON_ID + " INTEGER," +
                Bookmarks.DATE_CREATED + " INTEGER," +
                Bookmarks.DATE_MODIFIED + " INTEGER," +
                Bookmarks.GUID + " TEXT NOT NULL," +
                Bookmarks.IS_DELETED + " INTEGER NOT NULL DEFAULT 0, " +
                "FOREIGN KEY (" + Bookmarks.PARENT + ") REFERENCES " +
                TABLE_BOOKMARKS + "(" + Bookmarks._ID + ")" +
                ");");

        db.execSQL("CREATE INDEX bookmarks_url_index ON " + TABLE_BOOKMARKS + "("
                + Bookmarks.URL + ")");
        db.execSQL("CREATE INDEX bookmarks_type_deleted_index ON " + TABLE_BOOKMARKS + "("
                + Bookmarks.TYPE + ", " + Bookmarks.IS_DELETED + ")");
        db.execSQL("CREATE UNIQUE INDEX bookmarks_guid_index ON " + TABLE_BOOKMARKS + "("
                + Bookmarks.GUID + ")");
        db.execSQL("CREATE INDEX bookmarks_modified_index ON " + TABLE_BOOKMARKS + "("
                + Bookmarks.DATE_MODIFIED + ")");
    }

    private void createHistoryTable(SQLiteDatabase db) {
        debug("Creating " + TABLE_HISTORY + " table");
        db.execSQL("CREATE TABLE " + TABLE_HISTORY + "(" +
                History._ID + " INTEGER PRIMARY KEY AUTOINCREMENT," +
                History.TITLE + " TEXT," +
                History.URL + " TEXT NOT NULL," +
                History.VISITS + " INTEGER NOT NULL DEFAULT 0," +
                History.FAVICON_ID + " INTEGER," +
                History.DATE_LAST_VISITED + " INTEGER," +
                History.DATE_CREATED + " INTEGER," +
                History.DATE_MODIFIED + " INTEGER," +
                History.GUID + " TEXT NOT NULL," +
                History.IS_DELETED + " INTEGER NOT NULL DEFAULT 0" +
                ");");

        db.execSQL("CREATE INDEX history_url_index ON " + TABLE_HISTORY + '('
                + History.URL + ')');
        db.execSQL("CREATE UNIQUE INDEX history_guid_index ON " + TABLE_HISTORY + '('
                + History.GUID + ')');
        db.execSQL("CREATE INDEX history_modified_index ON " + TABLE_HISTORY + '('
                + History.DATE_MODIFIED + ')');
        db.execSQL("CREATE INDEX history_visited_index ON " + TABLE_HISTORY + '('
                + History.DATE_LAST_VISITED + ')');
    }

    private void createFaviconsTable(SQLiteDatabase db) {
        debug("Creating " + TABLE_FAVICONS + " table");
        db.execSQL("CREATE TABLE " + TABLE_FAVICONS + " (" +
                Favicons._ID + " INTEGER PRIMARY KEY AUTOINCREMENT," +
                Favicons.URL + " TEXT UNIQUE," +
                Favicons.DATA + " BLOB," +
                Favicons.DATE_CREATED + " INTEGER," +
                Favicons.DATE_MODIFIED + " INTEGER" +
                ");");

        db.execSQL("CREATE INDEX favicons_url_index ON " + TABLE_FAVICONS + "("
                + Favicons.URL + ")");
        db.execSQL("CREATE INDEX favicons_modified_index ON " + TABLE_FAVICONS + "("
                + Favicons.DATE_MODIFIED + ")");
    }

    private void createThumbnailsTable(SQLiteDatabase db) {
        debug("Creating " + TABLE_THUMBNAILS + " table");
        db.execSQL("CREATE TABLE " + TABLE_THUMBNAILS + " (" +
                Thumbnails._ID + " INTEGER PRIMARY KEY AUTOINCREMENT," +
                Thumbnails.URL + " TEXT UNIQUE," +
                Thumbnails.DATA + " BLOB" +
                ");");

        db.execSQL("CREATE INDEX thumbnails_url_index ON " + TABLE_THUMBNAILS + "("
                + Thumbnails.URL + ")");
    }

    private void createBookmarksWithFaviconsView(SQLiteDatabase db) {
        debug("Creating " + VIEW_BOOKMARKS_WITH_FAVICONS + " view");

        db.execSQL("CREATE VIEW IF NOT EXISTS " + VIEW_BOOKMARKS_WITH_FAVICONS + " AS " +
                "SELECT " + qualifyColumn(TABLE_BOOKMARKS, "*") +
                ", " + qualifyColumn(TABLE_FAVICONS, Favicons.DATA) + " AS " + Bookmarks.FAVICON +
                ", " + qualifyColumn(TABLE_FAVICONS, Favicons.URL) + " AS " + Bookmarks.FAVICON_URL +
                " FROM " + TABLE_BOOKMARKS_JOIN_FAVICONS);
    }

    private void createHistoryWithFaviconsView(SQLiteDatabase db) {
        debug("Creating " + VIEW_HISTORY_WITH_FAVICONS + " view");

        db.execSQL("CREATE VIEW IF NOT EXISTS " + VIEW_HISTORY_WITH_FAVICONS + " AS " +
                "SELECT " + qualifyColumn(TABLE_HISTORY, "*") +
                ", " + qualifyColumn(TABLE_FAVICONS, Favicons.DATA) + " AS " + History.FAVICON +
                ", " + qualifyColumn(TABLE_FAVICONS, Favicons.URL) + " AS " + History.FAVICON_URL +
                " FROM " + TABLE_HISTORY_JOIN_FAVICONS);
    }

    private void createClientsTable(SQLiteDatabase db) {
        debug("Creating " + TABLE_CLIENTS + " table");

        
        db.execSQL("CREATE TABLE " + TABLE_CLIENTS + "(" +
                BrowserContract.Clients.GUID + " TEXT PRIMARY KEY," +
                BrowserContract.Clients.NAME + " TEXT," +
                BrowserContract.Clients.LAST_MODIFIED + " INTEGER," +
                BrowserContract.Clients.DEVICE_TYPE + " TEXT" +
                ");");

        
        db.execSQL("CREATE INDEX " + TabsProvider.INDEX_CLIENTS_GUID +
                " ON " + TABLE_CLIENTS + "(" + BrowserContract.Clients.GUID + ")");
    }

    private void createTabsTable(SQLiteDatabase db) {
        debug("Creating tabs.db: " + db.getPath());
        debug("Creating " + TABLE_TABS + " table");

        
        db.execSQL("CREATE TABLE " + TABLE_TABS + "(" +
                BrowserContract.Tabs._ID + " INTEGER PRIMARY KEY AUTOINCREMENT," +
                BrowserContract.Tabs.CLIENT_GUID + " TEXT," +
                BrowserContract.Tabs.TITLE + " TEXT," +
                BrowserContract.Tabs.URL + " TEXT," +
                BrowserContract.Tabs.HISTORY + " TEXT," +
                BrowserContract.Tabs.FAVICON + " TEXT," +
                BrowserContract.Tabs.LAST_USED + " INTEGER," +
                BrowserContract.Tabs.POSITION + " INTEGER" +
                ");");

        
        db.execSQL("CREATE INDEX " + TabsProvider.INDEX_TABS_GUID +
                " ON " + TABLE_TABS + "(" + BrowserContract.Tabs.CLIENT_GUID + ")");
        db.execSQL("CREATE INDEX " + TabsProvider.INDEX_TABS_POSITION +
                " ON " + TABLE_TABS + "(" + BrowserContract.Tabs.POSITION + ")");
    }

    
    private void createLocalClient(SQLiteDatabase db) {
        debug("Inserting local Fennec client into " + TABLE_CLIENTS + " table");

        ContentValues values = new ContentValues();
        values.put(BrowserContract.Clients.LAST_MODIFIED, System.currentTimeMillis());
        db.insertOrThrow(TABLE_CLIENTS, null, values);
    }

    private void createCombinedViewOn19(SQLiteDatabase db) {
        






















        db.execSQL("CREATE VIEW IF NOT EXISTS " + VIEW_COMBINED + " AS" +

                
                " SELECT " + qualifyColumn(TABLE_BOOKMARKS, Bookmarks._ID) + " AS " + Combined.BOOKMARK_ID + "," +
                    "-1 AS " + Combined.HISTORY_ID + "," +
                    "0 AS " + Combined._ID + "," +
                    qualifyColumn(TABLE_BOOKMARKS, Bookmarks.URL) + " AS " + Combined.URL + ", " +
                    qualifyColumn(TABLE_BOOKMARKS, Bookmarks.TITLE) + " AS " + Combined.TITLE + ", " +
                    "-1 AS " + Combined.VISITS + ", " +
                    "-1 AS " + Combined.DATE_LAST_VISITED + "," +
                    qualifyColumn(TABLE_BOOKMARKS, Bookmarks.FAVICON_ID) + " AS " + Combined.FAVICON_ID +
                " FROM " + TABLE_BOOKMARKS +
                " WHERE " +
                    qualifyColumn(TABLE_BOOKMARKS, Bookmarks.TYPE)  + " = " + Bookmarks.TYPE_BOOKMARK + " AND " +
                    
                    qualifyColumn(TABLE_BOOKMARKS, Bookmarks.PARENT)  + " <> " + Bookmarks.FIXED_PINNED_LIST_ID + " AND " +
                    qualifyColumn(TABLE_BOOKMARKS, Bookmarks.IS_DELETED)  + " = 0 AND " +
                    qualifyColumn(TABLE_BOOKMARKS, Bookmarks.URL) +
                        " NOT IN (SELECT " + History.URL + " FROM " + TABLE_HISTORY + ")" +
                " UNION ALL" +

                    
                    " SELECT " +
                        "CASE " + qualifyColumn(TABLE_BOOKMARKS, Bookmarks.IS_DELETED) +

                            
                            
                            " WHEN 0 THEN " +
                                "CASE " + qualifyColumn(TABLE_BOOKMARKS, Bookmarks.PARENT) +
                                    " WHEN " + Bookmarks.FIXED_PINNED_LIST_ID + " THEN " +
                                        "NULL " +
                                    "ELSE " +
                                        qualifyColumn(TABLE_BOOKMARKS, Bookmarks._ID) +
                                " END " +
                            "ELSE " +
                                "NULL " +
                        "END AS " + Combined.BOOKMARK_ID + "," +
                        qualifyColumn(TABLE_HISTORY, History._ID) + " AS " + Combined.HISTORY_ID + "," +
                        "0 AS " + Combined._ID + "," +
                        qualifyColumn(TABLE_HISTORY, History.URL) + " AS " + Combined.URL + "," +

                        
                        
                        "COALESCE(" + qualifyColumn(TABLE_BOOKMARKS, Bookmarks.TITLE) + ", " +
                                      qualifyColumn(TABLE_HISTORY, History.TITLE) +
                                ") AS " + Combined.TITLE + "," +
                        qualifyColumn(TABLE_HISTORY, History.VISITS) + " AS " + Combined.VISITS + "," +
                        qualifyColumn(TABLE_HISTORY, History.DATE_LAST_VISITED) + " AS " + Combined.DATE_LAST_VISITED + "," +
                        qualifyColumn(TABLE_HISTORY, History.FAVICON_ID) + " AS " + Combined.FAVICON_ID +

                    
                    " FROM " + TABLE_HISTORY + " LEFT OUTER JOIN " + TABLE_BOOKMARKS +
                    " ON " + qualifyColumn(TABLE_BOOKMARKS, Bookmarks.URL) + " = " + qualifyColumn(TABLE_HISTORY, History.URL) +
                    " WHERE " +
                        qualifyColumn(TABLE_HISTORY, History.IS_DELETED) + " = 0 AND " +
                        "(" +
                            
                            qualifyColumn(TABLE_BOOKMARKS, Bookmarks.TYPE) + " IS NULL OR " +

                            
                            
                            qualifyColumn(TABLE_BOOKMARKS, Bookmarks.TYPE) + " = " + Bookmarks.TYPE_BOOKMARK +
                        ")"
        );

        debug("Creating " + VIEW_COMBINED_WITH_FAVICONS + " view");

        db.execSQL("CREATE VIEW IF NOT EXISTS " + VIEW_COMBINED_WITH_FAVICONS + " AS" +
                " SELECT " + qualifyColumn(VIEW_COMBINED, "*") + ", " +
                    qualifyColumn(TABLE_FAVICONS, Favicons.URL) + " AS " + Combined.FAVICON_URL + ", " +
                    qualifyColumn(TABLE_FAVICONS, Favicons.DATA) + " AS " + Combined.FAVICON +
                " FROM " + VIEW_COMBINED + " LEFT OUTER JOIN " + TABLE_FAVICONS +
                    " ON " + Combined.FAVICON_ID + " = " + qualifyColumn(TABLE_FAVICONS, Favicons._ID));

    }

    @Override
    public void onCreate(SQLiteDatabase db) {
        debug("Creating browser.db: " + db.getPath());

        for (Table table : BrowserProvider.sTables) {
            table.onCreate(db);
        }

        createBookmarksTable(db);
        createHistoryTable(db);
        createFaviconsTable(db);
        createThumbnailsTable(db);
        createTabsTable(db);
        createClientsTable(db);
        createLocalClient(db);

        createBookmarksWithFaviconsView(db);
        createHistoryWithFaviconsView(db);
        createCombinedViewOn19(db);

        createOrUpdateSpecialFolder(db, Bookmarks.PLACES_FOLDER_GUID,
            R.string.bookmarks_folder_places, 0);

        createOrUpdateAllSpecialFolders(db);
        createSearchHistoryTable(db);
        createReadingListTable(db, TABLE_READING_LIST);
        didCreateCurrentReadingListTable = true;      
        createReadingListIndices(db, TABLE_READING_LIST);
    }

    





    public void copyTabsDB(File tabsDBFile, SQLiteDatabase destinationDB) {
        createTabsTable(destinationDB);
        createClientsTable(destinationDB);

        SQLiteDatabase oldTabsDB = null;
        try {
            oldTabsDB = SQLiteDatabase.openDatabase(tabsDBFile.getPath(), null, SQLiteDatabase.OPEN_READONLY);

            if (!DBUtils.copyTable(oldTabsDB, TABLE_CLIENTS, destinationDB, TABLE_CLIENTS)) {
                Log.e(LOGTAG, "Failed to migrate table clients; ignoring.");
            }
            if (!DBUtils.copyTable(oldTabsDB, TABLE_TABS, destinationDB, TABLE_TABS)) {
                Log.e(LOGTAG, "Failed to migrate table tabs; ignoring.");
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "Exception occurred while trying to copy from " + tabsDBFile.getPath() +
                    " to " + destinationDB.getPath() + "; ignoring.", e);
        } finally {
            if (oldTabsDB != null) {
                oldTabsDB.close();
            }
        }
    }

    private void createSearchHistoryTable(SQLiteDatabase db) {
        debug("Creating " + SearchHistory.TABLE_NAME + " table");

        db.execSQL("CREATE TABLE " + SearchHistory.TABLE_NAME + "(" +
                    SearchHistory._ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
                    SearchHistory.QUERY + " TEXT UNIQUE NOT NULL, " +
                    SearchHistory.DATE_LAST_VISITED + " INTEGER, " +
                    SearchHistory.VISITS + " INTEGER ) ");

        db.execSQL("CREATE INDEX idx_search_history_last_visited ON " +
                SearchHistory.TABLE_NAME + "(" + SearchHistory.DATE_LAST_VISITED + ")");
    }

    private boolean didCreateCurrentReadingListTable = false;
    private void createReadingListTable(final SQLiteDatabase db, final String tableName) {
        debug("Creating " + TABLE_READING_LIST + " table");

        db.execSQL("CREATE TABLE " + tableName + "(" +
                   ReadingListItems._ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
                   ReadingListItems.GUID + " TEXT UNIQUE, " +                          

                   ReadingListItems.CONTENT_STATUS + " TINYINT NOT NULL DEFAULT " + ReadingListItems.STATUS_UNFETCHED + ", " +
                   ReadingListItems.SYNC_STATUS + " TINYINT NOT NULL DEFAULT " + ReadingListItems.SYNC_STATUS_NEW + ", " +
                   ReadingListItems.SYNC_CHANGE_FLAGS + " TINYINT NOT NULL DEFAULT " + ReadingListItems.SYNC_CHANGE_NONE + ", " +

                   ReadingListItems.CLIENT_LAST_MODIFIED + " INTEGER NOT NULL, " +     
                   ReadingListItems.SERVER_LAST_MODIFIED + " INTEGER, " +              

                   
                   ReadingListItems.SERVER_STORED_ON + " INTEGER, " +
                   ReadingListItems.ADDED_ON + " INTEGER, " +                   
                   ReadingListItems.MARKED_READ_ON + " INTEGER, " +

                   
                   ReadingListItems.IS_DELETED + " TINYINT NOT NULL DEFAULT 0, " +
                   ReadingListItems.IS_ARCHIVED + " TINYINT NOT NULL DEFAULT 0, " +
                   ReadingListItems.IS_UNREAD + " TINYINT NOT NULL DEFAULT 1, " +
                   ReadingListItems.IS_ARTICLE + " TINYINT NOT NULL DEFAULT 0, " +
                   ReadingListItems.IS_FAVORITE + " TINYINT NOT NULL DEFAULT 0, " +

                   ReadingListItems.URL + " TEXT NOT NULL, " +
                   ReadingListItems.TITLE + " TEXT, " +
                   ReadingListItems.RESOLVED_URL + " TEXT, " +
                   ReadingListItems.RESOLVED_TITLE + " TEXT, " +

                   ReadingListItems.EXCERPT + " TEXT, " +

                   ReadingListItems.ADDED_BY + " TEXT, " +
                   ReadingListItems.MARKED_READ_BY + " TEXT, " +

                   ReadingListItems.WORD_COUNT + " INTEGER DEFAULT 0, " +
                   ReadingListItems.READ_POSITION + " INTEGER DEFAULT 0 " +
                "); ");
    }

    private void createReadingListIndices(final SQLiteDatabase db, final String tableName) {
        
        db.execSQL("CREATE INDEX reading_list_url ON " + tableName + "("
                           + ReadingListItems.URL + ")");
        db.execSQL("CREATE INDEX reading_list_content_status ON " + tableName + "("
                           + ReadingListItems.CONTENT_STATUS + ")");
    }

    private void createOrUpdateAllSpecialFolders(SQLiteDatabase db) {
        createOrUpdateSpecialFolder(db, Bookmarks.MOBILE_FOLDER_GUID,
            R.string.bookmarks_folder_mobile, 0);
        createOrUpdateSpecialFolder(db, Bookmarks.TOOLBAR_FOLDER_GUID,
            R.string.bookmarks_folder_toolbar, 1);
        createOrUpdateSpecialFolder(db, Bookmarks.MENU_FOLDER_GUID,
            R.string.bookmarks_folder_menu, 2);
        createOrUpdateSpecialFolder(db, Bookmarks.TAGS_FOLDER_GUID,
            R.string.bookmarks_folder_tags, 3);
        createOrUpdateSpecialFolder(db, Bookmarks.UNFILED_FOLDER_GUID,
            R.string.bookmarks_folder_unfiled, 4);
        createOrUpdateSpecialFolder(db, Bookmarks.PINNED_FOLDER_GUID,
            R.string.bookmarks_folder_pinned, 5);
    }

    private void createOrUpdateSpecialFolder(SQLiteDatabase db,
            String guid, int titleId, int position) {
        ContentValues values = new ContentValues();
        values.put(Bookmarks.GUID, guid);
        values.put(Bookmarks.TYPE, Bookmarks.TYPE_FOLDER);
        values.put(Bookmarks.POSITION, position);

        if (guid.equals(Bookmarks.PLACES_FOLDER_GUID)) {
            values.put(Bookmarks._ID, Bookmarks.FIXED_ROOT_ID);
        } else if (guid.equals(Bookmarks.PINNED_FOLDER_GUID)) {
            values.put(Bookmarks._ID, Bookmarks.FIXED_PINNED_LIST_ID);
        }

        
        values.put(Bookmarks.PARENT, Bookmarks.FIXED_ROOT_ID);

        String title = mContext.getResources().getString(titleId);
        values.put(Bookmarks.TITLE, title);

        long now = System.currentTimeMillis();
        values.put(Bookmarks.DATE_CREATED, now);
        values.put(Bookmarks.DATE_MODIFIED, now);

        int updated = db.update(TABLE_BOOKMARKS, values,
                                Bookmarks.GUID + " = ?",
                                new String[] { guid });

        if (updated == 0) {
            db.insert(TABLE_BOOKMARKS, Bookmarks.GUID, values);
            debug("Inserted special folder: " + guid);
        } else {
            debug("Updated special folder: " + guid);
        }
    }

    private boolean isSpecialFolder(ContentValues values) {
        String guid = values.getAsString(Bookmarks.GUID);
        if (guid == null) {
            return false;
        }

        return guid.equals(Bookmarks.MOBILE_FOLDER_GUID) ||
               guid.equals(Bookmarks.MENU_FOLDER_GUID) ||
               guid.equals(Bookmarks.TOOLBAR_FOLDER_GUID) ||
               guid.equals(Bookmarks.UNFILED_FOLDER_GUID) ||
               guid.equals(Bookmarks.TAGS_FOLDER_GUID);
    }

    private void migrateBookmarkFolder(SQLiteDatabase db, int folderId,
            BookmarkMigrator migrator) {
        Cursor c = null;

        debug("Migrating bookmark folder with id = " + folderId);

        String selection = Bookmarks.PARENT + " = " + folderId;
        String[] selectionArgs = null;

        boolean isRootFolder = (folderId == Bookmarks.FIXED_ROOT_ID);

        
        
        
        
        if (isRootFolder) {
            selection = Bookmarks.GUID + " != ?" + " AND (" +
                        selection + " OR " + Bookmarks.PARENT + " = NULL)";
            selectionArgs = new String[] { Bookmarks.PLACES_FOLDER_GUID };
        }

        List<Integer> subFolders = new ArrayList<Integer>();
        List<ContentValues> invalidSpecialEntries = new ArrayList<ContentValues>();

        try {
            c = db.query(TABLE_BOOKMARKS_TMP,
                         null,
                         selection,
                         selectionArgs,
                         null, null, null);

            
            
            
            while (c.moveToNext()) {
                ContentValues values = new ContentValues();

                
                
                
                
                DatabaseUtils.cursorRowToContentValues(c, values);

                boolean isSpecialFolder = isSpecialFolder(values);

                
                
                if (values.getAsLong(Bookmarks.PARENT) == null && isSpecialFolder)
                    values.put(Bookmarks.PARENT, Bookmarks.FIXED_ROOT_ID);

                if (isRootFolder && !isSpecialFolder) {
                    invalidSpecialEntries.add(values);
                    continue;
                }

                if (migrator != null)
                    migrator.updateForNewTable(values);

                debug("Migrating bookmark: " + values.getAsString(Bookmarks.TITLE));
                db.insert(TABLE_BOOKMARKS, Bookmarks.URL, values);

                Integer type = values.getAsInteger(Bookmarks.TYPE);
                if (type != null && type == Bookmarks.TYPE_FOLDER)
                    subFolders.add(values.getAsInteger(Bookmarks._ID));
            }
        } finally {
            if (c != null)
                c.close();
        }

        
        
        
        final int nInvalidSpecialEntries = invalidSpecialEntries.size();
        if (nInvalidSpecialEntries > 0) {
            Integer mobileFolderId = getMobileFolderId(db);
            if (mobileFolderId == null) {
                Log.e(LOGTAG, "Error migrating invalid special folder entries: mobile folder id is null");
                return;
            }

            debug("Found " + nInvalidSpecialEntries + " invalid special folder entries");
            for (int i = 0; i < nInvalidSpecialEntries; i++) {
                ContentValues values = invalidSpecialEntries.get(i);
                values.put(Bookmarks.PARENT, mobileFolderId);

                db.insert(TABLE_BOOKMARKS, Bookmarks.URL, values);
            }
        }

        final int nSubFolders = subFolders.size();
        for (int i = 0; i < nSubFolders; i++) {
            int subFolderId = subFolders.get(i);
            migrateBookmarkFolder(db, subFolderId, migrator);
        }
    }

    private void migrateBookmarksTable(SQLiteDatabase db) {
        migrateBookmarksTable(db, null);
    }

    private void migrateBookmarksTable(SQLiteDatabase db, BookmarkMigrator migrator) {
        debug("Renaming bookmarks table to " + TABLE_BOOKMARKS_TMP);
        db.execSQL("ALTER TABLE " + TABLE_BOOKMARKS +
                   " RENAME TO " + TABLE_BOOKMARKS_TMP);

        debug("Dropping views and indexes related to " + TABLE_BOOKMARKS);

        db.execSQL("DROP INDEX IF EXISTS bookmarks_url_index");
        db.execSQL("DROP INDEX IF EXISTS bookmarks_type_deleted_index");
        db.execSQL("DROP INDEX IF EXISTS bookmarks_guid_index");
        db.execSQL("DROP INDEX IF EXISTS bookmarks_modified_index");

        createBookmarksTable(db);

        createOrUpdateSpecialFolder(db, Bookmarks.PLACES_FOLDER_GUID,
            R.string.bookmarks_folder_places, 0);

        migrateBookmarkFolder(db, Bookmarks.FIXED_ROOT_ID, migrator);

        
        
        createOrUpdateAllSpecialFolders(db);

        debug("Dropping bookmarks temporary table");
        db.execSQL("DROP TABLE IF EXISTS " + TABLE_BOOKMARKS_TMP);
    }

    



    private void migrateHistoryTable(SQLiteDatabase db) {
        debug("Renaming history table to " + TABLE_HISTORY_TMP);
        db.execSQL("ALTER TABLE " + TABLE_HISTORY +
                   " RENAME TO " + TABLE_HISTORY_TMP);

        debug("Dropping views and indexes related to " + TABLE_HISTORY);

        db.execSQL("DROP INDEX IF EXISTS history_url_index");
        db.execSQL("DROP INDEX IF EXISTS history_guid_index");
        db.execSQL("DROP INDEX IF EXISTS history_modified_index");
        db.execSQL("DROP INDEX IF EXISTS history_visited_index");

        createHistoryTable(db);

        db.execSQL("INSERT INTO " + TABLE_HISTORY + " SELECT * FROM " + TABLE_HISTORY_TMP);

        debug("Dropping history temporary table");
        db.execSQL("DROP TABLE IF EXISTS " + TABLE_HISTORY_TMP);
    }

    private void upgradeDatabaseFrom3to4(SQLiteDatabase db) {
        migrateBookmarksTable(db, new BookmarkMigrator3to4());
    }

    private void upgradeDatabaseFrom6to7(SQLiteDatabase db) {
        debug("Removing history visits with NULL GUIDs");
        db.execSQL("DELETE FROM " + TABLE_HISTORY + " WHERE " + History.GUID + " IS NULL");

        migrateBookmarksTable(db);
        migrateHistoryTable(db);
    }

    private void upgradeDatabaseFrom7to8(SQLiteDatabase db) {
        debug("Combining history entries with the same URL");

        final String TABLE_DUPES = "duped_urls";
        final String TOTAL = "total";
        final String LATEST = "latest";
        final String WINNER = "winner";

        db.execSQL("CREATE TEMP TABLE " + TABLE_DUPES + " AS" +
                  " SELECT " + History.URL + ", " +
                              "SUM(" + History.VISITS + ") AS " + TOTAL + ", " +
                              "MAX(" + History.DATE_MODIFIED + ") AS " + LATEST + ", " +
                              "MAX(" + History._ID + ") AS " + WINNER +
                  " FROM " + TABLE_HISTORY +
                  " GROUP BY " + History.URL +
                  " HAVING count(" + History.URL + ") > 1");

        db.execSQL("CREATE UNIQUE INDEX " + TABLE_DUPES + "_url_index ON " +
                   TABLE_DUPES + " (" + History.URL + ")");

        final String fromClause = " FROM " + TABLE_DUPES + " WHERE " +
                                  qualifyColumn(TABLE_DUPES, History.URL) + " = " +
                                  qualifyColumn(TABLE_HISTORY, History.URL);

        db.execSQL("UPDATE " + TABLE_HISTORY +
                  " SET " + History.VISITS + " = (SELECT " + TOTAL + fromClause + "), " +
                            History.DATE_MODIFIED + " = (SELECT " + LATEST + fromClause + "), " +
                            History.IS_DELETED + " = " +
                                "(" + History._ID + " <> (SELECT " + WINNER + fromClause + "))" +
                  " WHERE " + History.URL + " IN (SELECT " + History.URL + " FROM " + TABLE_DUPES + ")");

        db.execSQL("DROP TABLE " + TABLE_DUPES);
    }

    private void upgradeDatabaseFrom10to11(SQLiteDatabase db) {
        db.execSQL("CREATE INDEX bookmarks_type_deleted_index ON " + TABLE_BOOKMARKS + "("
                + Bookmarks.TYPE + ", " + Bookmarks.IS_DELETED + ")");
    }

    private void upgradeDatabaseFrom12to13(SQLiteDatabase db) {
        createFaviconsTable(db);

        
        
        
        
        try {
            db.execSQL("ALTER TABLE " + TABLE_HISTORY +
                                    " ADD COLUMN " + History.FAVICON_ID + " INTEGER");
            db.execSQL("ALTER TABLE " + TABLE_BOOKMARKS +
                               " ADD COLUMN " + Bookmarks.FAVICON_ID + " INTEGER");
        } catch (SQLException e) {
            
            debug("Exception adding favicon_id column. We're probably fine." + e);
        }

        createThumbnailsTable(db);

        db.execSQL("DROP VIEW IF EXISTS bookmarks_with_images");
        db.execSQL("DROP VIEW IF EXISTS history_with_images");
        db.execSQL("DROP VIEW IF EXISTS combined_with_images");

        createBookmarksWithFaviconsView(db);
        createHistoryWithFaviconsView(db);

        db.execSQL("DROP TABLE IF EXISTS images");
    }

    private void upgradeDatabaseFrom13to14(SQLiteDatabase db) {
        createOrUpdateSpecialFolder(db, Bookmarks.PINNED_FOLDER_GUID,
            R.string.bookmarks_folder_pinned, 6);
    }

    private void upgradeDatabaseFrom14to15(SQLiteDatabase db) {
        Cursor c = null;
        try {
            
            c = db.query(TABLE_BOOKMARKS,
                         new String[] { Bookmarks._ID, Bookmarks.URL },
                         Bookmarks.PARENT + " = ?",
                         new String[] { Integer.toString(Bookmarks.FIXED_PINNED_LIST_ID) },
                         null, null, null);

            while (c.moveToNext()) {
                
                String url = c.getString(c.getColumnIndexOrThrow(Bookmarks.URL));
                if (Uri.parse(url).getScheme() != null) {
                    continue;
                }

                
                ContentValues values = new ContentValues(1);
                String newUrl = Uri.fromParts("user-entered", url, null).toString();
                values.put(Bookmarks.URL, newUrl);
                db.update(TABLE_BOOKMARKS, values, Bookmarks._ID + " = ?",
                          new String[] { Integer.toString(c.getInt(c.getColumnIndexOrThrow(Bookmarks._ID))) });
            }
        } finally {
            if (c != null) {
                c.close();
            }
        }
    }

    private void upgradeDatabaseFrom15to16(SQLiteDatabase db) {
        
        
        
        createV19CombinedView(db);
    }

    private void upgradeDatabaseFrom16to17(SQLiteDatabase db) {
        
        try {
            db.execSQL("DELETE FROM " + TABLE_FAVICONS +
                    " WHERE length(" + Favicons.DATA + ") = 0");
            db.execSQL("DELETE FROM " + TABLE_THUMBNAILS +
                    " WHERE length(" + Thumbnails.DATA + ") = 0");
        } catch (SQLException e) {
            Log.e(LOGTAG, "Error purging invalid favicons or thumbnails", e);
        }
    }

    


    private void upgradeDatabaseFrom17to18(SQLiteDatabase db) {
        debug("Moving reading list items from 'bookmarks' table to 'reading_list' table");

        final String selection = Bookmarks.PARENT + " = ? AND " + Bookmarks.IS_DELETED + " = ? ";
        final String[] selectionArgs = { String.valueOf(Bookmarks.FIXED_READING_LIST_ID), "0" };
        final String[] projection = {   Bookmarks._ID,
                                        Bookmarks.GUID,
                                        Bookmarks.URL,
                                        Bookmarks.DATE_MODIFIED,
                                        Bookmarks.DATE_CREATED,
                                        Bookmarks.TITLE };

        try {
            db.beginTransaction();

            
            createReadingListTable(db, TABLE_READING_LIST);

            
            final Cursor cursor = db.query(TABLE_BOOKMARKS, projection, selection, selectionArgs, null, null, null);

            if (cursor == null) {
                
                db.setTransactionSuccessful();
                return;
            }

            try {
                
                while (cursor.moveToNext()) {
                    debug(DatabaseUtils.dumpCurrentRowToString(cursor));
                    final ContentValues values = new ContentValues();

                    
                    DatabaseUtils.cursorStringToContentValues(cursor, Bookmarks.URL, values, ReadingListItems.URL);
                    DatabaseUtils.cursorStringToContentValues(cursor, Bookmarks.TITLE, values, ReadingListItems.TITLE);
                    DatabaseUtils.cursorLongToContentValues(cursor, Bookmarks.DATE_CREATED, values, ReadingListItems.ADDED_ON);
                    DatabaseUtils.cursorLongToContentValues(cursor, Bookmarks.DATE_MODIFIED, values, ReadingListItems.CLIENT_LAST_MODIFIED);

                    db.insertOrThrow(TABLE_READING_LIST, null, values);
                }
            } finally {
                cursor.close();
            }

            
            db.delete(TABLE_BOOKMARKS,
                      Bookmarks.PARENT + " = ? ",
                      new String[] { String.valueOf(Bookmarks.FIXED_READING_LIST_ID) });

            
            db.delete(TABLE_BOOKMARKS,
                      Bookmarks._ID + " = ? ",
                      new String[] { String.valueOf(Bookmarks.FIXED_READING_LIST_ID) });

            
            createReadingListIndices(db, TABLE_READING_LIST);

            
            db.setTransactionSuccessful();
            didCreateCurrentReadingListTable = true;

        } catch (SQLException e) {
            Log.e(LOGTAG, "Error migrating reading list items", e);
        } finally {
            db.endTransaction();
        }
    }

    private void upgradeDatabaseFrom18to19(SQLiteDatabase db) {
        
        createV19CombinedView(db);

        
        db.execSQL("DELETE FROM " + TABLE_HISTORY + " WHERE " + History.URL + " IS NULL");

        
        db.execSQL("UPDATE " + TABLE_BOOKMARKS + " SET " +
                   Bookmarks.TYPE + " = " + Bookmarks.TYPE_BOOKMARK +
                   " WHERE " + Bookmarks.TYPE + " IS NULL");
    }

    private void upgradeDatabaseFrom19to20(SQLiteDatabase db) {
        createSearchHistoryTable(db);
    }

    private void upgradeDatabaseFrom21to22(SQLiteDatabase db) {
        if (didCreateCurrentReadingListTable) {
            debug("No need to add CONTENT_STATUS to reading list; we just created with the current schema.");
            return;
        }

        debug("Adding CONTENT_STATUS column to reading list table.");

        try {
            db.execSQL("ALTER TABLE " + TABLE_READING_LIST +
                       " ADD COLUMN " + ReadingListItems.CONTENT_STATUS +
                       " TINYINT DEFAULT " + ReadingListItems.STATUS_UNFETCHED);

            db.execSQL("CREATE INDEX reading_list_content_status ON " + TABLE_READING_LIST + "("
                    + ReadingListItems.CONTENT_STATUS + ")");
        } catch (SQLiteException e) {
            
            
            Log.e(LOGTAG, "Error upgrading database from 21 to 22", e);
        }
    }

    private void upgradeDatabaseFrom22to23(SQLiteDatabase db) {
        if (didCreateCurrentReadingListTable) {
            debug("No need to rev reading list schema; we just created with the current schema.");
            return;
        }

        debug("Rewriting reading list table.");
        createReadingListTable(db, "tmp_rl");

        
        db.execSQL("DROP INDEX IF EXISTS reading_list_url");
        db.execSQL("DROP INDEX IF EXISTS reading_list_guid");
        db.execSQL("DROP INDEX IF EXISTS reading_list_content_status");

        final String thisDevice = ReadingListProvider.PLACEHOLDER_THIS_DEVICE;
        db.execSQL("INSERT INTO tmp_rl (" +
                   
                   ReadingListItems._ID + ", " +
                   ReadingListItems.URL + ", " +
                   ReadingListItems.TITLE + ", " +
                   ReadingListItems.RESOLVED_TITLE + ", " +       
                   ReadingListItems.RESOLVED_URL + ", " +         
                   ReadingListItems.EXCERPT + ", " +
                   ReadingListItems.IS_UNREAD + ", " +            
                   ReadingListItems.IS_DELETED + ", " +           
                   ReadingListItems.GUID + ", " +                 
                   ReadingListItems.CLIENT_LAST_MODIFIED + ", " + 
                   ReadingListItems.ADDED_ON + ", " +             
                   ReadingListItems.CONTENT_STATUS + ", " +
                   ReadingListItems.MARKED_READ_BY + ", " +       
                   ReadingListItems.ADDED_BY +                    
                   ") " +
                   "SELECT " +
                   "_id, url, title, " +
                   "CASE content_status WHEN " + ReadingListItems.STATUS_FETCHED_ARTICLE + " THEN title ELSE NULL END, " +   
                   "CASE content_status WHEN " + ReadingListItems.STATUS_FETCHED_ARTICLE + " THEN url ELSE NULL END, " +     
                   "excerpt, " +
                   "CASE read WHEN 1 THEN 0 ELSE 1 END, " +            
                   "0, " +                                             
                   "NULL, modified, created, content_status, " +
                   "CASE read WHEN 1 THEN ? ELSE NULL END, " +         
                   "?" +                                               
                   " FROM " + TABLE_READING_LIST +
                   " WHERE deleted = 0",
                   new String[] {thisDevice, thisDevice});

        
        db.execSQL("DROP TABLE " + TABLE_READING_LIST);
        db.execSQL("ALTER TABLE tmp_rl RENAME TO " + TABLE_READING_LIST);

        createReadingListIndices(db, TABLE_READING_LIST);
    }

    private void upgradeDatabaseFrom23to24(SQLiteDatabase db) {
        
        
        try {
            final File oldTabsDBFile = new File(GeckoProfile.get(mContext).getDir(), "tabs.db");
            copyTabsDB(oldTabsDBFile, db);
        } catch (Exception e) {
            Log.e(LOGTAG, "Got exception copying tabs and clients data from tabs.db to browser.db; ignoring.", e);
        }

        
        for (String filename : new String[] { "tabs.db", "tabs.db-shm", "tabs.db-wal" }) {
            final File file = new File(GeckoProfile.get(mContext).getDir(), filename);
            try {
                FileUtils.delete(file);
            } catch (Exception e) {
                Log.e(LOGTAG, "Exception occurred while trying to delete " + file.getPath() + "; ignoring.", e);
            }
        }
    }

    private void createV19CombinedView(SQLiteDatabase db) {
        db.execSQL("DROP VIEW IF EXISTS " + VIEW_COMBINED);
        db.execSQL("DROP VIEW IF EXISTS " + VIEW_COMBINED_WITH_FAVICONS);

        createCombinedViewOn19(db);
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        debug("Upgrading browser.db: " + db.getPath() + " from " +
                oldVersion + " to " + newVersion);

        
        
        for (int v = oldVersion + 1; v <= newVersion; v++) {
            switch(v) {
                case 4:
                    upgradeDatabaseFrom3to4(db);
                    break;

                case 7:
                    upgradeDatabaseFrom6to7(db);
                    break;

                case 8:
                    upgradeDatabaseFrom7to8(db);
                    break;

                case 11:
                    upgradeDatabaseFrom10to11(db);
                    break;

                case 13:
                    upgradeDatabaseFrom12to13(db);
                    break;

                case 14:
                    upgradeDatabaseFrom13to14(db);
                    break;

                case 15:
                    upgradeDatabaseFrom14to15(db);
                    break;

                case 16:
                    upgradeDatabaseFrom15to16(db);
                    break;

                case 17:
                    upgradeDatabaseFrom16to17(db);
                    break;

                case 18:
                    upgradeDatabaseFrom17to18(db);
                    break;

                case 19:
                    upgradeDatabaseFrom18to19(db);
                    break;

                case 20:
                    upgradeDatabaseFrom19to20(db);
                    break;

                case 22:
                    upgradeDatabaseFrom21to22(db);
                    break;

                case 23:
                    upgradeDatabaseFrom22to23(db);
                    break;

                case 24:
                    upgradeDatabaseFrom23to24(db);
                    break;
            }
        }

        for (Table table : BrowserProvider.sTables) {
            table.onUpgrade(db, oldVersion, newVersion);
        }

        
        
        if (oldVersion < 13 && newVersion >= 13) {
            if (mContext.getDatabasePath("favicon_urls.db").exists()) {
                mContext.deleteDatabase("favicon_urls.db");
            }
        }
    }

    @Override
    public void onOpen(SQLiteDatabase db) {
        debug("Opening browser.db: " + db.getPath());

        Cursor cursor = null;
        try {
            cursor = db.rawQuery("PRAGMA foreign_keys=ON", null);
        } finally {
            if (cursor != null)
                cursor.close();
        }
        cursor = null;
        try {
            cursor = db.rawQuery("PRAGMA synchronous=NORMAL", null);
        } finally {
            if (cursor != null)
                cursor.close();
        }

        
        
        if (Build.VERSION.SDK_INT >= 11) {
            
            if (Build.VERSION.SDK_INT < 16) {
                db.enableWriteAheadLogging();

                
                db.setLockingEnabled(false);
            }
        } else {
            
            cursor = null;
            try {
                cursor = db.rawQuery("PRAGMA journal_mode=PERSIST", null);
            } finally {
                if (cursor != null)
                    cursor.close();
            }
        }
    }

    
    
    private static final boolean logDebug   = Log.isLoggable(LOGTAG, Log.DEBUG);
    private static final boolean logVerbose = Log.isLoggable(LOGTAG, Log.VERBOSE);
    protected static void trace(String message) {
        if (logVerbose) {
            Log.v(LOGTAG, message);
        }
    }

    protected static void debug(String message) {
        if (logDebug) {
            Log.d(LOGTAG, message);
        }
    }

    private Integer getMobileFolderId(SQLiteDatabase db) {
        Cursor c = null;

        try {
            c = db.query(TABLE_BOOKMARKS,
                         mobileIdColumns,
                         Bookmarks.GUID + " = ?",
                         mobileIdSelectionArgs,
                         null, null, null);

            if (c == null || !c.moveToFirst())
                return null;

            return c.getInt(c.getColumnIndex(Bookmarks._ID));
        } finally {
            if (c != null)
                c.close();
        }
    }

    private interface BookmarkMigrator {
        public void updateForNewTable(ContentValues bookmark);
    }

    private class BookmarkMigrator3to4 implements BookmarkMigrator {
        @Override
        public void updateForNewTable(ContentValues bookmark) {
            Integer isFolder = bookmark.getAsInteger("folder");
            if (isFolder == null || isFolder != 1) {
                bookmark.put(Bookmarks.TYPE, Bookmarks.TYPE_BOOKMARK);
            } else {
                bookmark.put(Bookmarks.TYPE, Bookmarks.TYPE_FOLDER);
            }

            bookmark.remove("folder");
        }
    }
}


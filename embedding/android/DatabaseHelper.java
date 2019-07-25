





































package org.mozilla.gecko;

import android.content.*;
import android.database.sqlite.*;
import android.util.*;

class DatabaseHelper extends SQLiteOpenHelper {

    private static final String CREATE_MOZ_PLACES =
      "CREATE TABLE moz_places ( " +
        "  id INTEGER PRIMARY KEY" +
        ", url LONGVARCHAR UNIQUE" +
        ", title LONGVARCHAR" +
        ", rev_host LONGVARCHAR" +
        ", visit_count INTEGER DEFAULT 0" +
        ", hidden INTEGER DEFAULT 0 NOT NULL" +
        ", typed INTEGER DEFAULT 0 NOT NULL" +
        ", favicon_id INTEGER" +
        ", frecency INTEGER DEFAULT -1 NOT NULL" +
        ", last_visit_date INTEGER " +
        ", guid TEXT" +
      ")";

    private static final String CREATE_MOZ_HISTORYVISITS  =
      "CREATE TABLE moz_historyvisits (" +
        "  id INTEGER PRIMARY KEY" +
        ", from_visit INTEGER" +
        ", place_id INTEGER" +
        ", visit_date INTEGER" +
        ", visit_type INTEGER" +
        ", session INTEGER" +
      ")";

    private static final String CREATE_MOZ_INPUTHISTORY =
      "CREATE TABLE moz_inputhistory (" +
        "  place_id INTEGER NOT NULL" +
        ", input LONGVARCHAR NOT NULL" +
        ", use_count INTEGER" +
        ", PRIMARY KEY (place_id, input)" +
      ")";

    private static final String CREATE_MOZ_ANNOS =
      "CREATE TABLE moz_annos (" +
        "  id INTEGER PRIMARY KEY" +
        ", place_id INTEGER NOT NULL" +
        ", anno_attribute_id INTEGER" +
        ", mime_type VARCHAR(32) DEFAULT NULL" +
        ", content LONGVARCHAR" +
        ", flags INTEGER DEFAULT 0" +
        ", expiration INTEGER DEFAULT 0" +
        ", type INTEGER DEFAULT 0" +
        ", dateAdded INTEGER DEFAULT 0" +
        ", lastModified INTEGER DEFAULT 0" +
      ")";

    private static final String CREATE_MOZ_ANNO_ATTRIBUTES =
      "CREATE TABLE moz_anno_attributes (" +
        "  id INTEGER PRIMARY KEY" +
        ", name VARCHAR(32) UNIQUE NOT NULL" +
      ")";

    private static final String CREATE_MOZ_ITEMS_ANNOS =
      "CREATE TABLE moz_items_annos (" +
        "  id INTEGER PRIMARY KEY" +
        ", item_id INTEGER NOT NULL" +
        ", anno_attribute_id INTEGER" +
        ", mime_type VARCHAR(32) DEFAULT NULL" +
        ", content LONGVARCHAR" +
        ", flags INTEGER DEFAULT 0" +
        ", expiration INTEGER DEFAULT 0" +
        ", type INTEGER DEFAULT 0" +
        ", dateAdded INTEGER DEFAULT 0" +
        ", lastModified INTEGER DEFAULT 0" +
      ")";

    private static final String CREATE_MOZ_FAVICONS =
      "CREATE TABLE moz_favicons (" +
        "  id INTEGER PRIMARY KEY" +
        ", url LONGVARCHAR UNIQUE" +
        ", data BLOB" +
        ", mime_type VARCHAR(32)" +
        ", expiration LONG" +
      ")";

    private static final String CREATE_MOZ_BOOKMARKS =
      "CREATE TABLE moz_bookmarks (" +
        "  id INTEGER PRIMARY KEY" +
        ", type INTEGER" +
        ", fk INTEGER DEFAULT NULL" +
        ", parent INTEGER" +
        ", position INTEGER" +
        ", title LONGVARCHAR" +
        ", keyword_id INTEGER" +
        ", folder_type TEXT" +
        ", dateAdded INTEGER" +
        ", lastModified INTEGER" +
        ", guid TEXT" +
      ")";

    private static final String CREATE_MOZ_BOOKMARKS_ROOTS =
      "CREATE TABLE moz_bookmarks_roots (" +
        "  root_name VARCHAR(16) UNIQUE" +
        ", folder_id INTEGER" +
      ")";

    private static final String CREATE_MOZ_KEYWORDS =
      "CREATE TABLE moz_keywords (" +
        "  id INTEGER PRIMARY KEY AUTOINCREMENT" +
        ", keyword TEXT UNIQUE" +
      ")";

    private static final String DATABASE_NAME = "places";
    private static final int DATABASE_VERSION = 1;

    DatabaseHelper(Context context) {
        super(context, DATABASE_NAME, null, DATABASE_VERSION);
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
        db.execSQL(CREATE_MOZ_PLACES);
        db.execSQL(CREATE_MOZ_HISTORYVISITS);
        db.execSQL(CREATE_MOZ_BOOKMARKS);
        db.execSQL(CREATE_MOZ_BOOKMARKS_ROOTS);
        db.execSQL(CREATE_MOZ_FAVICONS);
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        Log.w("DatabaseHelper", "Upgrading database from version " + oldVersion + " to "
                + newVersion + ", which will destroy all old data");
        db.execSQL("DROP TABLE IF EXISTS places");
        onCreate(db);
    }

}

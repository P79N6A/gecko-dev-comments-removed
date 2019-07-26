



package org.mozilla.gecko.db;

import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.db.BrowserContract.HomeListItems;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.Log;

final class HomeListsDatabaseHelper extends SQLiteOpenHelper {

    static final int DATABASE_VERSION = 1;
    static final String DATABASE_NAME = "homelists.db";

    static final String TABLE_ITEMS = "items";

    public HomeListsDatabaseHelper(Context context, String databasePath) {
        super(context, databasePath, null, DATABASE_VERSION);
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
        db.execSQL("CREATE TABLE " + TABLE_ITEMS + "(" +
                HomeListItems._ID + " INTEGER PRIMARY KEY AUTOINCREMENT," +
                HomeListItems.PROVIDER_ID + " TEXT," +
                HomeListItems.TITLE + " TEXT," +
                HomeListItems.URL + " TEXT)"
        );
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        
    }

    @Override
    public void onOpen(SQLiteDatabase db) {
        
    }
}

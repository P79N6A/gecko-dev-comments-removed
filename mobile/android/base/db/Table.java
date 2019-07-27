




package org.mozilla.gecko.db;

import android.content.ContentValues;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.net.Uri;



interface Table {
    
    public static class ContentProviderInfo {
        public final int id; 
        public final String name; 
                                  
                                  

        public ContentProviderInfo(int id, String name) {
            if (name == null) {
                throw new IllegalArgumentException("Content provider info must specify a name");
            }
            this.id = id;
            this.name = name;
        }
    }

    
    ContentProviderInfo[] getContentProviderInfo();

    
    
    
    void onCreate(SQLiteDatabase db);
    void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion);

    
    
    Cursor query(SQLiteDatabase db, Uri uri, int dbId, String[] projection, String selection, String[] selectionArgs, String sortOrder, String groupBy, String limit);
    int update(SQLiteDatabase db, Uri uri, int dbId, ContentValues values, String selection, String[] selectionArgs);
    long insert(SQLiteDatabase db, Uri uri, int dbId, ContentValues values);
    int delete(SQLiteDatabase db, Uri uri, int dbId, String selection, String[] selectionArgs);
};

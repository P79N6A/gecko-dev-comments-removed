



package org.mozilla.gecko.sync.repositories.android;

import android.content.Context;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteDatabase.CursorFactory;
import android.database.sqlite.SQLiteOpenHelper;

public abstract class CachedSQLiteOpenHelper extends SQLiteOpenHelper {

  public CachedSQLiteOpenHelper(Context context, String name, CursorFactory factory,
      int version) {
    super(context, name, factory, version);
  }

  
  
  private SQLiteDatabase readableDatabase;
  private SQLiteDatabase writableDatabase;

  synchronized protected SQLiteDatabase getCachedReadableDatabase() {
    if (readableDatabase == null) {
      if (writableDatabase == null) {
        readableDatabase = this.getReadableDatabase();
        return readableDatabase;
      } else {
        return writableDatabase;
      }
    } else {
      return readableDatabase;
    }
  }

  synchronized protected SQLiteDatabase getCachedWritableDatabase() {
    if (writableDatabase == null) {
      writableDatabase = this.getWritableDatabase();
    }
    return writableDatabase;
  }

  synchronized public void close() {
    if (readableDatabase != null) {
      readableDatabase.close();
      readableDatabase = null;
    }
    if (writableDatabase != null) {
      writableDatabase.close();
      writableDatabase = null;
    }
    super.close();
  }

  
  public boolean isClosed() {
    return readableDatabase == null &&
           writableDatabase == null;
  }
}

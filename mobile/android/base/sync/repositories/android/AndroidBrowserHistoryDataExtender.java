



package org.mozilla.gecko.sync.repositories.android;

import java.util.ArrayList;

import org.json.simple.JSONArray;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.repositories.NullCursorException;
import org.mozilla.gecko.sync.repositories.domain.HistoryRecord;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;

public class AndroidBrowserHistoryDataExtender extends CachedSQLiteOpenHelper {

  public static final String LOG_TAG = "SyncHistoryVisits";

  
  protected static final String DB_NAME = "history_extension_database";
  protected static final int SCHEMA_VERSION = 1;

  
  public static final String   TBL_HISTORY_EXT = "HistoryExtension";
  public static final String   COL_GUID = "guid";
  public static final String   GUID_IS = COL_GUID + " = ?";
  public static final String   COL_VISITS = "visits";
  public static final String[] TBL_COLUMNS = { COL_GUID, COL_VISITS };

  private final RepoUtils.QueryHelper queryHelper;

  public AndroidBrowserHistoryDataExtender(Context context) {
    super(context, DB_NAME, null, SCHEMA_VERSION);
    this.queryHelper = new RepoUtils.QueryHelper(context, null, LOG_TAG);
  }

  @Override
  public void onCreate(SQLiteDatabase db) {
    String createTableSql = "CREATE TABLE " + TBL_HISTORY_EXT + " ("
        + COL_GUID + " TEXT PRIMARY KEY, "
        + COL_VISITS + " TEXT)";
    db.execSQL(createTableSql);
  }

  @Override
  public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
    
    db.execSQL("DROP TABLE IF EXISTS " + TBL_HISTORY_EXT);
    onCreate(db);
  }

  public void wipe() {
    SQLiteDatabase db = this.getCachedWritableDatabase();
    onUpgrade(db, SCHEMA_VERSION, SCHEMA_VERSION);
  }

  









  protected void store(SQLiteDatabase db, String guid, JSONArray visits) {
    ContentValues cv = new ContentValues();
    cv.put(COL_GUID, guid);
    if (visits == null) {
      cv.put(COL_VISITS, "[]");
    } else {
      cv.put(COL_VISITS, visits.toJSONString());
    }

    String[] args = new String[] { guid };
    int rowsUpdated = db.update(TBL_HISTORY_EXT, cv, GUID_IS, args);
    if (rowsUpdated >= 1) {
      Logger.debug(LOG_TAG, "Replaced history extension record for row with GUID " + guid);
    } else {
      long rowId = db.insert(TBL_HISTORY_EXT, null, cv);
      Logger.debug(LOG_TAG, "Inserted history extension record into row: " + rowId);
    }
  }

  








  public void store(String guid, JSONArray visits) {
    SQLiteDatabase db = this.getCachedWritableDatabase();
    store(db, guid, visits);
  }

  


  public void bulkInsert(ArrayList<HistoryRecord> records) {
    SQLiteDatabase db = this.getCachedWritableDatabase();
    try {
      db.beginTransaction();

      for (HistoryRecord record : records) {
        store(db, record.guid, record.visits);
      }

      db.setTransactionSuccessful();
    } catch (SQLException e) {
      Logger.error(LOG_TAG, "Caught exception in bulkInsert new history visits.", e);
    } finally {
      db.endTransaction();
    }
  }

  






  public Cursor fetch(String guid) throws NullCursorException {
    String[] args = new String[] { guid };

    SQLiteDatabase db = this.getCachedReadableDatabase();
    Cursor cur = queryHelper.safeQuery(db, ".fetch",
        TBL_HISTORY_EXT, TBL_COLUMNS, GUID_IS, args);
    return cur;
  }

  public JSONArray visitsForGUID(String guid) throws NullCursorException {
    if (guid == null) {
      Logger.warn(LOG_TAG, "Asked for visits for null GUID.");
      return new JSONArray();
    }

    Logger.debug(LOG_TAG, "Fetching visits for GUID " + guid);
    Cursor visits = fetch(guid);
    try {
      if (!visits.moveToFirst()) {
        
        return new JSONArray();
      } else {
        return RepoUtils.getJSONArrayFromCursor(visits, COL_VISITS);
      }
    } finally {
      visits.close();
    }
  }

  





  public int delete(String guid) {
    String[] args = new String[] { guid };

    SQLiteDatabase db = this.getCachedWritableDatabase();
    return db.delete(TBL_HISTORY_EXT, GUID_IS, args);
  }

  





  public Cursor fetchAll() throws NullCursorException {
    SQLiteDatabase db = this.getCachedReadableDatabase();
    Cursor cur = queryHelper.safeQuery(db, ".fetchAll", TBL_HISTORY_EXT,
        TBL_COLUMNS,
        null, null);
    return cur;
  }
}

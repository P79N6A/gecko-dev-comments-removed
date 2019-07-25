





































package org.mozilla.gecko.sync.repositories.android;

import org.mozilla.gecko.sync.repositories.NullCursorException;
import org.mozilla.gecko.sync.repositories.domain.Record;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.util.Log;

public abstract class AndroidBrowserRepositoryDataAccessor {

  private static final String[] GUID_COLUMNS = new String[] { BrowserContract.SyncColumns.GUID };
  protected Context context;
  protected String LOG_TAG = "AndroidBrowserRepositoryDataAccessor";
  private final RepoUtils.QueryHelper queryHelper;

  public AndroidBrowserRepositoryDataAccessor(Context context) {
    this.context = context;
    this.queryHelper = new RepoUtils.QueryHelper(context, getUri(), LOG_TAG);
  }

  protected abstract String[] getAllColumns();
  protected abstract ContentValues getContentValues(Record record);
  protected abstract Uri getUri();

  public String dateModifiedWhere(long timestamp) {
    return BrowserContract.SyncColumns.DATE_MODIFIED + " >= " + Long.toString(timestamp);
  }

  public void wipe() {
    Log.i(LOG_TAG, "wiping: " + getUri());
    String where = BrowserContract.SyncColumns.GUID + " NOT IN ('mobile')";
    context.getContentResolver().delete(getUri(), where, null);
  }
  
  public void purgeDeleted() throws NullCursorException {
    String where = BrowserContract.SyncColumns.IS_DELETED + "= 1";
    Cursor cur = queryHelper.safeQuery(".purgeDeleted", GUID_COLUMNS, where, null, null);

    try {
      if (!cur.moveToFirst()) {
        return;
      }
      while (!cur.isAfterLast()) {
        delete(RepoUtils.getStringFromCursor(cur, BrowserContract.SyncColumns.GUID));
        cur.moveToNext();
      }
    } finally {
      cur.close();
    }
  }
  
  protected void delete(String guid) {
    String where  = BrowserContract.SyncColumns.GUID + " = ?";
    String[] args = new String[] { guid };

    int deleted = context.getContentResolver().delete(getUri(), where, args);
    if (deleted == 1) {
      return;
    }
    Log.w(LOG_TAG, "Unexpectedly deleted " + deleted + " rows for guid " + guid);
  }

  public void update(String guid, Record newRecord) {
    String where  = BrowserContract.SyncColumns.GUID + " = ?";
    String[] args = new String[] { guid };
    ContentValues cv = getContentValues(newRecord);
    int updated = context.getContentResolver().update(getUri(), cv, where, args);
    if (updated != 1) {
      Log.w(LOG_TAG, "Unexpectedly updated " + updated + " rows for guid " + guid);
    }
  }

  public Uri insert(Record record) {
    ContentValues cv = getContentValues(record);
    Log.d(LOG_TAG, "INSERTING: " + cv.getAsString("guid"));
    return context.getContentResolver().insert(getUri(), cv);
  }

  






  public Cursor fetchAll() throws NullCursorException {
    return queryHelper.safeQuery(".fetchAll", getAllColumns(), null, null, null);
  }
  
  







  public Cursor getGUIDsSince(long timestamp) throws NullCursorException {
    return queryHelper.safeQuery(".getGUIDsSince",
        GUID_COLUMNS,
        dateModifiedWhere(timestamp),
        null, null);
  }

  







  public Cursor fetchSince(long timestamp) throws NullCursorException {
    return queryHelper.safeQuery(".fetchSince",
        getAllColumns(),
        dateModifiedWhere(timestamp),
        null, null);
  }

  







  public Cursor fetch(String guids[]) throws NullCursorException {
    String where = computeSQLInClause(guids.length, "guid");
    return queryHelper.safeQuery(".fetch", getAllColumns(), where, guids, null);
  }

  protected String computeSQLInClause(int items, String field) {
    StringBuilder builder = new StringBuilder(field);
    builder.append(" IN (");
    int i = 0;
    for (; i < items - 1; ++i) {
      builder.append("?, ");
    }
    if (i < items) {
      builder.append("?");
    }
    builder.append(")");
    return builder.toString();
  }

  public void delete(Record record) {
    String where  = BrowserContract.SyncColumns.GUID + " = ?";
    String[] args = new String[] { record.guid };

    int deleted = context.getContentResolver().delete(getUri(), where, args);
    if (deleted == 1) {
      return;
    }
    Log.w(LOG_TAG, "Unexpectedly deleted " + deleted + " rows for guid " + record.guid);
  }

  public void updateByGuid(String guid, ContentValues cv) {
    String where  = BrowserContract.SyncColumns.GUID + " = ?";
    String[] args = new String[] { guid };

    int updated = context.getContentResolver().update(getUri(), cv, where, args);
    if (updated == 1) {
      return;
    }
    Log.w(LOG_TAG, "Unexpectedly updated " + updated + " rows for guid " + guid);
  }
}

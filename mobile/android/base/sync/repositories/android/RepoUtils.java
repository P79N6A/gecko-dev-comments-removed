



package org.mozilla.gecko.sync.repositories.android;

import java.io.IOException;

import org.json.simple.JSONArray;
import org.json.simple.parser.ParseException;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.NonArrayJSONException;
import org.mozilla.gecko.sync.repositories.NullCursorException;
import org.mozilla.gecko.sync.repositories.domain.ClientRecord;
import org.mozilla.gecko.sync.repositories.domain.HistoryRecord;

import android.content.ContentProviderClient;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.net.Uri;
import android.os.RemoteException;

public class RepoUtils {

  private static final String LOG_TAG = "RepoUtils";

  






  public static class QueryHelper {
    private final Context context;
    private final Uri     uri;
    private final String  tag;

    public QueryHelper(Context context, Uri uri, String tag) {
      this.context = context;
      this.uri     = uri;
      this.tag     = tag;
    }

    
    public Cursor safeQuery(String label, String[] projection,
                            String selection, String[] selectionArgs, String sortOrder) throws NullCursorException {
      long queryStart = android.os.SystemClock.uptimeMillis();
      Cursor c = context.getContentResolver().query(uri, projection, selection, selectionArgs, sortOrder);
      return checkAndLogCursor(label, queryStart, c);
    }

    public Cursor safeQuery(String[] projection, String selection, String[] selectionArgs, String sortOrder) throws NullCursorException {
      return this.safeQuery(null, projection, selection, selectionArgs, sortOrder);
    }

    
    public Cursor safeQuery(ContentProviderClient client, String label, String[] projection,
                            String selection, String[] selectionArgs, String sortOrder) throws NullCursorException, RemoteException {
      long queryStart = android.os.SystemClock.uptimeMillis();
      Cursor c = client.query(uri, projection, selection, selectionArgs, sortOrder);
      return checkAndLogCursor(label, queryStart, c);
    }

    
    public Cursor safeQuery(SQLiteDatabase db, String label, String table, String[] columns,
                            String selection, String[] selectionArgs,
                            String groupBy, String having, String orderBy, String limit) throws NullCursorException {
      long queryStart = android.os.SystemClock.uptimeMillis();
      Cursor c = db.query(table, columns, selection, selectionArgs, groupBy, having, orderBy, limit);
      return checkAndLogCursor(label, queryStart, c);
    }

    public Cursor safeQuery(SQLiteDatabase db, String label, String table, String[] columns,
                            String selection, String[] selectionArgs) throws NullCursorException {
      return safeQuery(db, label, table, columns, selection, selectionArgs, null, null, null, null);
    }

    private Cursor checkAndLogCursor(String label, long queryStart, Cursor c) throws NullCursorException {
      long queryEnd = android.os.SystemClock.uptimeMillis();
      String logLabel = (label == null) ? tag : (tag + label);
      RepoUtils.queryTimeLogger(logLabel, queryStart, queryEnd);
      return checkNullCursor(logLabel, c);
    }

    public Cursor checkNullCursor(String logLabel, Cursor cursor) throws NullCursorException {
      if (cursor == null) {
        Logger.error(tag, "Got null cursor exception in " + logLabel);
        throw new NullCursorException(null);
      }
      return cursor;
    }
  }

  




  public static String optStringFromCursor(final Cursor cur, final String colId) {
    final int col = cur.getColumnIndex(colId);
    if (cur.isNull(col)) {
      return null;
    }
    return cur.getString(col);
  }

  



  public static String getStringFromCursor(final Cursor cur, final String colId) {
    
    
    return cur.getString(cur.getColumnIndex(colId));
  }

  public static long getLongFromCursor(Cursor cur, String colId) {
    return cur.getLong(cur.getColumnIndex(colId));
  }

  public static int getIntFromCursor(Cursor cur, String colId) {
    return cur.getInt(cur.getColumnIndex(colId));
  }

  public static JSONArray getJSONArrayFromCursor(Cursor cur, String colId) {
    String jsonArrayAsString = getStringFromCursor(cur, colId);
    if (jsonArrayAsString == null) {
      return new JSONArray();
    }
    try {
      return ExtendedJSONObject.parseJSONArray(getStringFromCursor(cur, colId));
    } catch (NonArrayJSONException e) {
      Logger.error(LOG_TAG, "JSON parsing error for " + colId, e);
      return null;
    } catch (IOException e) {
      Logger.error(LOG_TAG, "JSON parsing error for " + colId, e);
      return null;
    } catch (ParseException e) {
      Logger.error(LOG_TAG, "JSON parsing error for " + colId, e);
      return null;
    }
  }

  





  public static boolean isValidHistoryURI(String uri) {
    if (uri == null || uri.length() == 0) {
      return false;
    }

    
    if (uri.startsWith("http:") || uri.startsWith("https:")) {
      return true;
    }

    String scheme = Uri.parse(uri).getScheme();
    if (scheme == null) {
      return false;
    }

    
    if (scheme.equals("about") ||
        scheme.equals("imap") ||
        scheme.equals("news") ||
        scheme.equals("mailbox") ||
        scheme.equals("moz-anno") ||
        scheme.equals("view-source") ||
        scheme.equals("chrome") ||
        scheme.equals("resource") ||
        scheme.equals("data") ||
        scheme.equals("wyciwyg") ||
        scheme.equals("javascript")) {
      return false;
    }

    return true;
  }

  





  public static HistoryRecord historyFromMirrorCursor(Cursor cur) {
    final String guid = getStringFromCursor(cur, BrowserContract.SyncColumns.GUID);
    if (guid == null) {
      Logger.debug(LOG_TAG, "Skipping history record with null GUID.");
      return null;
    }

    final String historyURI = getStringFromCursor(cur, BrowserContract.History.URL);
    if (!isValidHistoryURI(historyURI)) {
      Logger.debug(LOG_TAG, "Skipping history record " + guid + " with unwanted/invalid URI " + historyURI);
      return null;
    }

    final long visitCount = getLongFromCursor(cur, BrowserContract.History.VISITS);
    if (visitCount <= 0) {
      Logger.debug(LOG_TAG, "Skipping history record " + guid + " with <= 0 visit count.");
      return null;
    }

    final String collection = "history";
    final long lastModified = getLongFromCursor(cur, BrowserContract.SyncColumns.DATE_MODIFIED);
    final boolean deleted = getLongFromCursor(cur, BrowserContract.SyncColumns.IS_DELETED) == 1;

    final HistoryRecord rec = new HistoryRecord(guid, collection, lastModified, deleted);

    rec.androidID         = getLongFromCursor(cur, BrowserContract.History._ID);
    rec.fennecDateVisited = getLongFromCursor(cur, BrowserContract.History.DATE_LAST_VISITED);
    rec.fennecVisitCount  = visitCount;
    rec.histURI           = historyURI;
    rec.title             = getStringFromCursor(cur, BrowserContract.History.TITLE);

    return logHistory(rec);
  }

  private static HistoryRecord logHistory(HistoryRecord rec) {
    try {
      Logger.debug(LOG_TAG, "Returning history record " + rec.guid + " (" + rec.androidID + ")");
      Logger.debug(LOG_TAG, "> Visited:          " + rec.fennecDateVisited);
      Logger.debug(LOG_TAG, "> Visits:           " + rec.fennecVisitCount);
      if (Logger.LOG_PERSONAL_INFORMATION) {
        Logger.pii(LOG_TAG, "> Title:            " + rec.title);
        Logger.pii(LOG_TAG, "> URI:              " + rec.histURI);
      }
    } catch (Exception e) {
      Logger.debug(LOG_TAG, "Exception logging history record " + rec, e);
    }
    return rec;
  }

  public static void logClient(ClientRecord rec) {
    if (Logger.shouldLogVerbose(LOG_TAG)) {
      Logger.trace(LOG_TAG, "Returning client record " + rec.guid + " (" + rec.androidID + ")");
      Logger.trace(LOG_TAG, "Client Name:   " + rec.name);
      Logger.trace(LOG_TAG, "Client Type:   " + rec.type);
      Logger.trace(LOG_TAG, "Last Modified: " + rec.lastModified);
      Logger.trace(LOG_TAG, "Deleted:       " + rec.deleted);
    }
  }

  public static void queryTimeLogger(String methodCallingQuery, long queryStart, long queryEnd) {
    long elapsedTime = queryEnd - queryStart;
    Logger.debug(LOG_TAG, "Query timer: " + methodCallingQuery + " took " + elapsedTime + "ms.");
  }

  public static boolean stringsEqual(String a, String b) {
    
    if (a == b) return true;
    if (a == null && b != null) return false;
    if (a != null && b == null) return false;

    return a.equals(b);
  }

  public static String computeSQLLongInClause(long[] items, String field) {
    final StringBuilder builder = new StringBuilder(field);
    builder.append(" IN (");
    int i = 0;
    for (; i < items.length - 1; ++i) {
      builder.append(items[i]);
      builder.append(", ");
    }
    if (i < items.length) {
      builder.append(items[i]);
    }
    builder.append(")");
    return builder.toString();
  }

  public static String computeSQLInClause(int items, String field) {
    final StringBuilder builder = new StringBuilder(field);
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
}

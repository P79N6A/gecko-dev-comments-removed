





































package org.mozilla.gecko.sync.repositories.android;

import java.util.HashMap;

import org.json.simple.JSONArray;
import org.json.simple.parser.JSONParser;
import org.json.simple.parser.ParseException;
import org.mozilla.gecko.R;
import org.mozilla.gecko.sync.repositories.domain.BookmarkRecord;
import org.mozilla.gecko.sync.repositories.domain.HistoryRecord;
import org.mozilla.gecko.sync.repositories.domain.PasswordRecord;

import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.util.Log;

public class RepoUtils {

  private static final String LOG_TAG = "DBUtils";
  public static String[] SPECIAL_GUIDS = new String[] {
    "menu",
    "places",
    "toolbar",
    "unfiled",
    "mobile"
  };
  
  
  public static HashMap<String, String> SPECIAL_GUIDS_MAP;
  public static void initialize(Context context) {
    if (SPECIAL_GUIDS_MAP == null) {
      SPECIAL_GUIDS_MAP = new HashMap<String, String>();
      SPECIAL_GUIDS_MAP.put("menu",    context.getString(R.string.bookmarks_folder_menu));
      SPECIAL_GUIDS_MAP.put("places",  context.getString(R.string.bookmarks_folder_places));
      SPECIAL_GUIDS_MAP.put("toolbar", context.getString(R.string.bookmarks_folder_toolbar));
      SPECIAL_GUIDS_MAP.put("unfiled", context.getString(R.string.bookmarks_folder_unfiled));
      SPECIAL_GUIDS_MAP.put("mobile",  context.getString(R.string.bookmarks_folder_mobile));
    }
  }

  public static String getStringFromCursor(Cursor cur, String colId) {
    return cur.getString(cur.getColumnIndex(colId));
  }

  public static long getLongFromCursor(Cursor cur, String colId) {
    return cur.getLong(cur.getColumnIndex(colId));
  }

  public static JSONArray getJSONArrayFromCursor(Cursor cur, String colId) {
    String jsonArrayAsString = getStringFromCursor(cur, colId);
    if (jsonArrayAsString == null) {
      return new JSONArray();
    }
    try {
      return (JSONArray) new JSONParser().parse(getStringFromCursor(cur, colId));
    } catch (ParseException e) {
      Log.e(LOG_TAG, "JSON parsing error for " + colId, e);
      return null;
    }
  }

  
  
  public static long getAndroidIdFromUri(Uri uri) {
    String path = uri.getPath();
    int lastSlash = path.lastIndexOf('/');
    return Long.parseLong(path.substring(lastSlash + 1));
  }

  
  public static BookmarkRecord bookmarkFromMirrorCursor(Cursor cur, String parentId, String parentName, JSONArray children) {

    String guid = getStringFromCursor(cur, BrowserContract.SyncColumns.GUID);
    String collection = "bookmarks";
    long lastModified = getLongFromCursor(cur, BrowserContract.SyncColumns.DATE_MODIFIED);
    boolean deleted = getLongFromCursor(cur, BrowserContract.SyncColumns.IS_DELETED) == 1 ? true : false;
    BookmarkRecord rec = new BookmarkRecord(guid, collection, lastModified, deleted);

    rec.title = getStringFromCursor(cur, BrowserContract.Bookmarks.TITLE);
    rec.bookmarkURI = getStringFromCursor(cur, BrowserContract.Bookmarks.URL);
    rec.description = getStringFromCursor(cur, BrowserContract.Bookmarks.DESCRIPTION);
    rec.tags = getJSONArrayFromCursor(cur, BrowserContract.Bookmarks.TAGS);
    rec.keyword = getStringFromCursor(cur, BrowserContract.Bookmarks.KEYWORD);
    rec.type = cur.getInt(cur.getColumnIndex(BrowserContract.Bookmarks.IS_FOLDER)) == 0 ?
      AndroidBrowserBookmarksDataAccessor.TYPE_BOOKMARK : AndroidBrowserBookmarksDataAccessor.TYPE_FOLDER;

    rec.androidID = getLongFromCursor(cur, BrowserContract.Bookmarks._ID);
    rec.androidPosition = getLongFromCursor(cur, BrowserContract.Bookmarks.POSITION);
    rec.children = children;

    
    rec.parentID = parentId;
    
    
    if (SPECIAL_GUIDS_MAP.containsKey(rec.parentID)) {
      rec.parentName = SPECIAL_GUIDS_MAP.get(rec.parentID);
    } else {
      rec.parentName = parentName;
    }
    return rec;
  }

  
  public static HistoryRecord historyFromMirrorCursor(Cursor cur) {

    String guid = getStringFromCursor(cur, BrowserContract.SyncColumns.GUID);
    String collection = "history";
    long lastModified = getLongFromCursor(cur,BrowserContract.SyncColumns.DATE_MODIFIED);
    boolean deleted = getLongFromCursor(cur, BrowserContract.SyncColumns.IS_DELETED) == 1 ? true : false;
    HistoryRecord rec = new HistoryRecord(guid, collection, lastModified, deleted);

    rec.title = getStringFromCursor(cur, BrowserContract.History.TITLE);
    rec.histURI = getStringFromCursor(cur, BrowserContract.History.URL);
    rec.androidID = getLongFromCursor(cur, BrowserContract.History._ID);
    rec.fennecDateVisited = getLongFromCursor(cur, BrowserContract.History.DATE_LAST_VISITED);
    rec.fennecVisitCount = getLongFromCursor(cur, BrowserContract.History.VISITS);

    return rec;
  }
  
  public static PasswordRecord passwordFromMirrorCursor(Cursor cur) {
    
    String guid = getStringFromCursor(cur, BrowserContract.SyncColumns.GUID);
    String collection = "passwords";
    long lastModified = getLongFromCursor(cur, BrowserContract.SyncColumns.DATE_MODIFIED);
    boolean deleted = getLongFromCursor(cur, BrowserContract.SyncColumns.IS_DELETED) == 1 ? true : false;
    PasswordRecord rec = new PasswordRecord(guid, collection, lastModified, deleted);
    rec.hostname = getStringFromCursor(cur, BrowserContract.Passwords.HOSTNAME);
    rec.httpRealm = getStringFromCursor(cur, BrowserContract.Passwords.HTTP_REALM);
    rec.formSubmitURL = getStringFromCursor(cur, BrowserContract.Passwords.FORM_SUBMIT_URL);
    rec.usernameField = getStringFromCursor(cur, BrowserContract.Passwords.USERNAME_FIELD);
    rec.passwordField = getStringFromCursor(cur, BrowserContract.Passwords.PASSWORD_FIELD);
    rec.encType = getStringFromCursor(cur, BrowserContract.Passwords.ENC_TYPE);
    
    
    rec.username = getStringFromCursor(cur, BrowserContract.Passwords.ENCRYPTED_USERNAME);
    rec.password = getStringFromCursor(cur, BrowserContract.Passwords.ENCRYPTED_PASSWORD);
    
    rec.timeLastUsed = getLongFromCursor(cur, BrowserContract.Passwords.TIME_LAST_USED);
    rec.timesUsed = getLongFromCursor(cur, BrowserContract.Passwords.TIMES_USED);
    
    return rec;
  }
  
  public static void queryTimeLogger(String methodCallingQuery, long queryStart, long queryEnd) {
    long elapsedTime = queryEnd - queryStart;
    Log.i(LOG_TAG, "Query timer: " + methodCallingQuery + " took " + elapsedTime + "ms.");
  }

  public static boolean stringsEqual(String a, String b) {
    
    if (a == b) return true;
    if (a == null && b != null) return false;
    if (a != null && b == null) return false;
    
    return a.equals(b);
  }
}

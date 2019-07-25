





































package org.mozilla.gecko.sync.repositories.android;

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

import org.json.simple.JSONArray;
import org.json.simple.parser.JSONParser;
import org.json.simple.parser.ParseException;
import org.mozilla.gecko.R;
import org.mozilla.gecko.sync.Logger;
import org.mozilla.gecko.sync.repositories.NullCursorException;
import org.mozilla.gecko.sync.repositories.domain.BookmarkRecord;
import org.mozilla.gecko.sync.repositories.domain.HistoryRecord;
import org.mozilla.gecko.sync.repositories.domain.PasswordRecord;

import android.content.Context;
import android.database.Cursor;
import android.net.Uri;

public class RepoUtils {

  private static final String LOG_TAG = "DBUtils";

  


  public static String[] SPECIAL_GUIDS = new String[] {
    
    "mobile",
    "places",
    "toolbar",
    "menu",
    "unfiled"
  };

  









































  public static final Map<String, String> SPECIAL_GUID_PARENTS;
  static {
    HashMap<String, String> m = new HashMap<String, String>();
    m.put("places",  null);
    m.put("menu",    "places");
    m.put("toolbar", "places");
    m.put("tags",    "places");
    m.put("unfiled", "places");
    m.put("mobile",  "places");
    SPECIAL_GUID_PARENTS = Collections.unmodifiableMap(m);
  }

  


  
  public static Map<String, String> SPECIAL_GUIDS_MAP;
  public static void initialize(Context context) {
    if (SPECIAL_GUIDS_MAP == null) {
      HashMap<String, String> m = new HashMap<String, String>();
      m.put("menu",    context.getString(R.string.bookmarks_folder_menu));
      m.put("places",  context.getString(R.string.bookmarks_folder_places));
      m.put("toolbar", context.getString(R.string.bookmarks_folder_toolbar));
      m.put("unfiled", context.getString(R.string.bookmarks_folder_unfiled));
      m.put("mobile",  context.getString(R.string.bookmarks_folder_mobile));
      SPECIAL_GUIDS_MAP = Collections.unmodifiableMap(m);
    }
  }

  






  public static class QueryHelper {
    private final Context context;
    private final Uri     uri;
    private final String  tag;

    public QueryHelper(Context context, Uri uri, String tag) {
      this.context = context;
      this.uri     = uri;
      this.tag     = tag;
    }

    public Cursor query(String[] projection, String selection, String[] selectionArgs, String sortOrder) {
      return this.query(null, projection, selection, selectionArgs, sortOrder);
    }

    public Cursor query(String label, String[] projection, String selection, String[] selectionArgs, String sortOrder) {
      String logLabel = (label == null) ? this.tag : this.tag + label;
      long queryStart = android.os.SystemClock.uptimeMillis();
      Cursor c = context.getContentResolver().query(uri, projection, selection, selectionArgs, sortOrder);
      long queryEnd   = android.os.SystemClock.uptimeMillis();
      RepoUtils.queryTimeLogger(logLabel, queryStart, queryEnd);
      return c;
    }

    public Cursor safeQuery(String label, String[] projection, String selection, String[] selectionArgs, String sortOrder) throws NullCursorException {
      Cursor c = this.query(label, projection, selection, selectionArgs, sortOrder);
      if (c == null) {
        Logger.error(tag, "Got null cursor exception in " + tag + ((label == null) ? "" : label));
        throw new NullCursorException(null);
      }
      return c;
    }
  }

  public static String getStringFromCursor(Cursor cur, String colId) {
    
    
    return cur.getString(cur.getColumnIndex(colId));
  }

  public static long getLongFromCursor(Cursor cur, String colId) {
    return cur.getLong(cur.getColumnIndex(colId));
  }

  public static long getIntFromCursor(Cursor cur, String colId) {
    return cur.getInt(cur.getColumnIndex(colId));
  }

  public static JSONArray getJSONArrayFromCursor(Cursor cur, String colId) {
    String jsonArrayAsString = getStringFromCursor(cur, colId);
    if (jsonArrayAsString == null) {
      return new JSONArray();
    }
    try {
      return (JSONArray) new JSONParser().parse(getStringFromCursor(cur, colId));
    } catch (ParseException e) {
      Logger.error(LOG_TAG, "JSON parsing error for " + colId, e);
      return null;
    }
  }

  
  
  public static long getAndroidIdFromUri(Uri uri) {
    String path = uri.getPath();
    int lastSlash = path.lastIndexOf('/');
    return Long.parseLong(path.substring(lastSlash + 1));
  }

  public static BookmarkRecord computeParentFields(BookmarkRecord rec, String suggestedParentID, String suggestedParentName) {
    final String guid = rec.guid;
    if (guid == null) {
      
      Logger.error(LOG_TAG, "No guid in computeParentFields!");
      return null;
    }

    String realParent = SPECIAL_GUID_PARENTS.get(guid);
    if (realParent == null) {
      
      realParent = suggestedParentID;
    } else {
      Logger.debug(LOG_TAG, "Ignoring suggested parent ID " + suggestedParentID +
                           " for " + guid + "; using " + realParent);
    }

    if (realParent == null) {
      
      Logger.error(LOG_TAG, "No parent for record " + guid);
      return null;
    }

    
    String parentName = SPECIAL_GUIDS_MAP.get(realParent);
    if (parentName == null) {
      parentName = suggestedParentName;
    }

    rec.parentID = realParent;
    rec.parentName = parentName;
    return rec;
  }

  
  public static BookmarkRecord bookmarkFromMirrorCursor(Cursor cur, String parentId, String parentName, JSONArray children) {

    String guid = getStringFromCursor(cur, BrowserContract.SyncColumns.GUID);
    String collection = "bookmarks";
    long lastModified = getLongFromCursor(cur, BrowserContract.SyncColumns.DATE_MODIFIED);
    boolean deleted   = getLongFromCursor(cur, BrowserContract.SyncColumns.IS_DELETED) == 1 ? true : false;
    boolean isFolder  = getIntFromCursor(cur, BrowserContract.Bookmarks.IS_FOLDER) == 1;
    BookmarkRecord rec = new BookmarkRecord(guid, collection, lastModified, deleted);

    rec.title = getStringFromCursor(cur, BrowserContract.Bookmarks.TITLE);
    rec.bookmarkURI = getStringFromCursor(cur, BrowserContract.Bookmarks.URL);
    rec.description = getStringFromCursor(cur, BrowserContract.Bookmarks.DESCRIPTION);
    rec.tags = getJSONArrayFromCursor(cur, BrowserContract.Bookmarks.TAGS);
    rec.keyword = getStringFromCursor(cur, BrowserContract.Bookmarks.KEYWORD);
    rec.type = isFolder ? AndroidBrowserBookmarksDataAccessor.TYPE_FOLDER :
                          AndroidBrowserBookmarksDataAccessor.TYPE_BOOKMARK;

    rec.androidID = getLongFromCursor(cur, BrowserContract.Bookmarks._ID);
    rec.androidPosition = getLongFromCursor(cur, BrowserContract.Bookmarks.POSITION);
    rec.children = children;

    
    
    
    return logBookmark(computeParentFields(rec, parentId, parentName));
  }

  private static BookmarkRecord logBookmark(BookmarkRecord rec) {
    try {
      Logger.debug(LOG_TAG, "Returning bookmark record " + rec.guid + " (" + rec.androidID +
                           ", parent " + rec.parentID + ")");
      if (Logger.LOG_PERSONAL_INFORMATION) {
        Logger.pii(LOG_TAG, "> Parent name:      " + rec.parentName);
        Logger.pii(LOG_TAG, "> Title:            " + rec.title);
        Logger.pii(LOG_TAG, "> Type:             " + rec.type);
        Logger.pii(LOG_TAG, "> URI:              " + rec.bookmarkURI);
        Logger.pii(LOG_TAG, "> Android position: " + rec.androidPosition);
        Logger.pii(LOG_TAG, "> Position:         " + rec.pos);
        if (rec.isFolder()) {
          Logger.pii(LOG_TAG, "FOLDER: Children are " +
                             (rec.children == null ?
                                 "null" :
                                 rec.children.toJSONString()));
        }
      }
    } catch (Exception e) {
      Logger.debug(LOG_TAG, "Exception logging bookmark record " + rec, e);
    }
    return rec;
  }

  
  public static HistoryRecord historyFromMirrorCursor(Cursor cur) {

    String guid = getStringFromCursor(cur, BrowserContract.SyncColumns.GUID);
    String collection = "history";
    long lastModified = getLongFromCursor(cur, BrowserContract.SyncColumns.DATE_MODIFIED);
    boolean deleted = getLongFromCursor(cur, BrowserContract.SyncColumns.IS_DELETED) == 1 ? true : false;
    HistoryRecord rec = new HistoryRecord(guid, collection, lastModified, deleted);

    rec.title = getStringFromCursor(cur, BrowserContract.History.TITLE);
    rec.histURI = getStringFromCursor(cur, BrowserContract.History.URL);
    rec.androidID = getLongFromCursor(cur, BrowserContract.History._ID);
    rec.fennecDateVisited = getLongFromCursor(cur, BrowserContract.History.DATE_LAST_VISITED);
    rec.fennecVisitCount = getLongFromCursor(cur, BrowserContract.History.VISITS);

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
      Logger.debug(LOG_TAG, "Exception logging bookmark record " + rec, e);
    }
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
    Logger.debug(LOG_TAG, "Query timer: " + methodCallingQuery + " took " + elapsedTime + "ms.");
  }

  public static boolean stringsEqual(String a, String b) {
    
    if (a == b) return true;
    if (a == null && b != null) return false;
    if (a != null && b == null) return false;
    
    return a.equals(b);
  }
}





package org.mozilla.gecko.sync.repositories.android;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

import org.json.simple.JSONArray;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.sync.repositories.NullCursorException;
import org.mozilla.gecko.sync.repositories.domain.BookmarkRecord;
import org.mozilla.gecko.sync.repositories.domain.Record;

import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;

public class AndroidBrowserBookmarksDataAccessor extends AndroidBrowserRepositoryDataAccessor {

  private static final String LOG_TAG = "BookmarksDataAccessor";

  


  private static final String BOOKMARK_IS_FOLDER = BrowserContract.Bookmarks.TYPE + " = " +
                                                   BrowserContract.Bookmarks.TYPE_FOLDER;
  private static final String GUID_NOT_TAGS_OR_PLACES = BrowserContract.SyncColumns.GUID + " NOT IN ('" +
                                                        BrowserContract.Bookmarks.TAGS_FOLDER_GUID + "', '" +
                                                        BrowserContract.Bookmarks.PLACES_FOLDER_GUID + "')";

  private static final String EXCLUDE_SPECIAL_GUIDS_WHERE_CLAUSE;
  static {
    if (AndroidBrowserBookmarksRepositorySession.SPECIAL_GUIDS.length > 0) {
      StringBuilder b = new StringBuilder(BrowserContract.SyncColumns.GUID + " NOT IN (");

      int remaining = AndroidBrowserBookmarksRepositorySession.SPECIAL_GUIDS.length - 1;
      for (String specialGuid : AndroidBrowserBookmarksRepositorySession.SPECIAL_GUIDS) {
        b.append('"');
        b.append(specialGuid);
        b.append('"');
        if (remaining-- > 0) {
          b.append(", ");
        }
      }
      b.append(')');
      EXCLUDE_SPECIAL_GUIDS_WHERE_CLAUSE = b.toString();
    } else {
      EXCLUDE_SPECIAL_GUIDS_WHERE_CLAUSE = null;       
    }
  }

  public static final String TYPE_FOLDER = "folder";
  public static final String TYPE_BOOKMARK = "bookmark";

  private final RepoUtils.QueryHelper queryHelper;

  public AndroidBrowserBookmarksDataAccessor(Context context) {
    super(context);
    this.queryHelper = new RepoUtils.QueryHelper(context, getUri(), LOG_TAG);
  }

  @Override
  protected Uri getUri() {
    return BrowserContractHelpers.BOOKMARKS_CONTENT_URI;
  }

  protected Uri getPositionsUri() {
    return BrowserContractHelpers.BOOKMARKS_POSITIONS_CONTENT_URI;
  }

  @Override
  public void wipe() {
    Uri uri = getUri();
    Logger.info(LOG_TAG, "wiping (except for special guids): " + uri);
    context.getContentResolver().delete(uri, EXCLUDE_SPECIAL_GUIDS_WHERE_CLAUSE, null);
  }

  private String[] GUID_AND_ID = new String[] { BrowserContract.Bookmarks.GUID,
                                                BrowserContract.Bookmarks._ID };

  protected Cursor getGuidsIDsForFolders() throws NullCursorException {
    
    String where = BOOKMARK_IS_FOLDER + " AND " + GUID_NOT_TAGS_OR_PLACES;
    return queryHelper.safeQuery(".getGuidsIDsForFolders", GUID_AND_ID, where, null, null);
  }

  







  public int updatePositions(ArrayList<String> childArray) {
    final int size = childArray.size();
    if (size == 0) {
      return 0;
    }

    Logger.debug(LOG_TAG, "Updating positions for " + size + " items.");
    String[] args = childArray.toArray(new String[size]);
    return context.getContentResolver().update(getPositionsUri(), new ContentValues(), null, args);
  }

  public int bumpModifiedByGUID(Collection<String> ids, long modified) {
    final int size = ids.size();
    if (size == 0) {
      return 0;
    }

    Logger.debug(LOG_TAG, "Bumping modified for " + size + " items to " + modified);
    String where = RepoUtils.computeSQLInClause(size, BrowserContract.Bookmarks.GUID);
    String[] selectionArgs = ids.toArray(new String[size]);
    ContentValues values = new ContentValues();
    values.put(BrowserContract.Bookmarks.DATE_MODIFIED, modified);

    return context.getContentResolver().update(getUri(), values, where, selectionArgs);
  }

  


  public int bumpModified(long id, long modified) {
    Logger.debug(LOG_TAG, "Bumping modified for " + id + " to " + modified);
    String where = BrowserContract.Bookmarks._ID + " = ?";
    String[] selectionArgs = new String[] { String.valueOf(id) };
    ContentValues values = new ContentValues();
    values.put(BrowserContract.Bookmarks.DATE_MODIFIED, modified);

    return context.getContentResolver().update(getUri(), values, where, selectionArgs);
  }

  protected void updateParentAndPosition(String guid, long newParentId, long position) {
    ContentValues cv = new ContentValues();
    cv.put(BrowserContract.Bookmarks.PARENT, newParentId);
    if (position >= 0) {
      cv.put(BrowserContract.Bookmarks.POSITION, position);
    }
    updateByGuid(guid, cv);
  }

  protected Map<String, Long> idsForGUIDs(String[] guids) throws NullCursorException {
    final String where = RepoUtils.computeSQLInClause(guids.length, BrowserContract.Bookmarks.GUID);
    Cursor c = queryHelper.safeQuery(".idsForGUIDs", GUID_AND_ID, where, guids, null);
    try {
      HashMap<String, Long> out = new HashMap<String, Long>();
      if (!c.moveToFirst()) {
        return out;
      }
      final int guidIndex = c.getColumnIndexOrThrow(BrowserContract.Bookmarks.GUID);
      final int idIndex = c.getColumnIndexOrThrow(BrowserContract.Bookmarks._ID);
      while (!c.isAfterLast()) {
        out.put(c.getString(guidIndex), c.getLong(idIndex));
        c.moveToNext();
      }
      return out;
    } finally {
      c.close();
    }
  }

  








  protected int moveChildren(String[] fromIDs, long to) {
    long now = System.currentTimeMillis();
    long pos = -1;

    ContentValues cv = new ContentValues();
    cv.put(BrowserContract.Bookmarks.PARENT, to);
    cv.put(BrowserContract.Bookmarks.DATE_MODIFIED, now);
    cv.put(BrowserContract.Bookmarks.POSITION, pos);

    final String where = RepoUtils.computeSQLInClause(fromIDs.length, BrowserContract.Bookmarks.PARENT);
    return context.getContentResolver().update(getUri(), cv, where, fromIDs);
  }
  
  



  public void checkAndBuildSpecialGuids() throws NullCursorException {
    final String[] specialGUIDs = AndroidBrowserBookmarksRepositorySession.SPECIAL_GUIDS;
    Cursor cur = fetch(specialGUIDs);
    long placesRoot = 0;

    
    HashMap<String, Boolean> statuses = new HashMap<String, Boolean>(specialGUIDs.length);
    try {
      if (cur.moveToFirst()) {
        while (!cur.isAfterLast()) {
          String guid = RepoUtils.getStringFromCursor(cur, BrowserContract.SyncColumns.GUID);
          if ("places".equals(guid)) {
            placesRoot = RepoUtils.getLongFromCursor(cur, BrowserContract.CommonColumns._ID);
          }
          
          boolean deleted = RepoUtils.getLongFromCursor(cur, BrowserContract.SyncColumns.IS_DELETED) == 1;
          statuses.put(guid, deleted);
          cur.moveToNext();
        }
      }
    } finally {
      cur.close();
    }

    
    for (String guid : specialGUIDs) {
      if (statuses.containsKey(guid)) {
        if (statuses.get(guid)) {
          
          Logger.info(LOG_TAG, "Undeleting special GUID " + guid);
          ContentValues cv = new ContentValues();
          cv.put(BrowserContract.SyncColumns.IS_DELETED, 0);
          updateByGuid(guid, cv);
        }
      } else {
        
        if (guid.equals("places")) {
          
          Logger.info(LOG_TAG, "No places root. Inserting one.");
          placesRoot = insertSpecialFolder("places", 0);
        } else if (guid.equals("mobile")) {
          Logger.info(LOG_TAG, "No mobile folder. Inserting one under the places root.");
          insertSpecialFolder("mobile", placesRoot);
        } else {
          
          Logger.info(LOG_TAG, "No " + guid + " root. Inserting one under places (" + placesRoot + ").");
          insertSpecialFolder(guid, placesRoot);
        }
      }
    }
  }

  private long insertSpecialFolder(String guid, long parentId) {
    BookmarkRecord record = new BookmarkRecord(guid);
    record.title = AndroidBrowserBookmarksRepositorySession.SPECIAL_GUIDS_MAP.get(guid);
    record.type = "folder";
    record.androidParentID = parentId;
    return ContentUris.parseId(insert(record));
  }

  @Override
  protected ContentValues getContentValues(Record record) {
    BookmarkRecord rec = (BookmarkRecord) record;

    if (rec.deleted) {
      ContentValues cv = new ContentValues();
      cv.put(BrowserContract.SyncColumns.GUID,     rec.guid);
      cv.put(BrowserContract.Bookmarks.IS_DELETED, 1);
      return cv;
    }

    final int recordType = BrowserContractHelpers.typeCodeForString(rec.type);
    if (recordType == -1) {
      throw new IllegalStateException("Unexpected record type " + rec.type);
    }

    ContentValues cv = new ContentValues();
    cv.put(BrowserContract.SyncColumns.GUID,      rec.guid);
    cv.put(BrowserContract.Bookmarks.TYPE,        recordType);
    cv.put(BrowserContract.Bookmarks.TITLE,       rec.title);
    cv.put(BrowserContract.Bookmarks.URL,         rec.bookmarkURI);
    cv.put(BrowserContract.Bookmarks.DESCRIPTION, rec.description);
    if (rec.tags == null) {
      rec.tags = new JSONArray();
    }
    cv.put(BrowserContract.Bookmarks.TAGS,        rec.tags.toJSONString());
    cv.put(BrowserContract.Bookmarks.KEYWORD,     rec.keyword);
    cv.put(BrowserContract.Bookmarks.PARENT,      rec.androidParentID);
    cv.put(BrowserContract.Bookmarks.POSITION,    rec.androidPosition);

    
    
    return cv;
  }

  


  public Cursor getChildren(long androidID) throws NullCursorException {
    return getChildren(androidID, false);
  }

  



  public Cursor getChildren(long androidID, boolean includeDeleted) throws NullCursorException {
    final String where = BrowserContract.Bookmarks.PARENT + " = ? AND " +
                         BrowserContract.SyncColumns.GUID + " <> ? " +
                         (!includeDeleted ? ("AND " + BrowserContract.SyncColumns.IS_DELETED + " = 0") : "");

    final String[] args = new String[] { String.valueOf(androidID), "places" };

    
    final String order = BrowserContract.Bookmarks.POSITION + ", " +
                         BrowserContract.SyncColumns.DATE_CREATED + ", " +
                         BrowserContract.Bookmarks._ID;
    return queryHelper.safeQuery(".getChildren", getAllColumns(), where, args, order);
  }

  
  @Override
  protected String[] getAllColumns() {
    return BrowserContractHelpers.BookmarkColumns;
  }
}

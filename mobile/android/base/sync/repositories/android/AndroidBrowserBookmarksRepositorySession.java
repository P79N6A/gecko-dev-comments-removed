



package org.mozilla.gecko.sync.repositories.android;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;
import java.util.TreeMap;

import org.json.simple.JSONArray;
import org.mozilla.gecko.R;
import org.mozilla.gecko.sync.Logger;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.repositories.NoGuidForIdException;
import org.mozilla.gecko.sync.repositories.NullCursorException;
import org.mozilla.gecko.sync.repositories.ParentNotFoundException;
import org.mozilla.gecko.sync.repositories.Repository;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionBeginDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionFinishDelegate;
import org.mozilla.gecko.sync.repositories.domain.BookmarkRecord;
import org.mozilla.gecko.sync.repositories.domain.Record;

import android.content.Context;
import android.database.Cursor;

public class AndroidBrowserBookmarksRepositorySession extends AndroidBrowserRepositorySession {

  
  private HashMap<String, Long> guidToID = new HashMap<String, Long>();
  private HashMap<Long, String> idToGuid = new HashMap<Long, String>();

  





















































  
  private HashMap<String, ArrayList<String>> missingParentToChildren = new HashMap<String, ArrayList<String>>();
  private HashMap<String, JSONArray>         parentToChildArray      = new HashMap<String, JSONArray>();
  private int needsReparenting = 0;

  private AndroidBrowserBookmarksDataAccessor dataAccessor;

  


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

  






  public static boolean forbiddenGUID(String recordGUID) {
    return recordGUID == null ||
           "places".equals(recordGUID) ||
           "tags".equals(recordGUID);
  }

  public AndroidBrowserBookmarksRepositorySession(Repository repository, Context context) {
    super(repository);

    if (SPECIAL_GUIDS_MAP == null) {
      HashMap<String, String> m = new HashMap<String, String>();
      m.put("menu",    context.getString(R.string.bookmarks_folder_menu));
      m.put("places",  context.getString(R.string.bookmarks_folder_places));
      m.put("toolbar", context.getString(R.string.bookmarks_folder_toolbar));
      m.put("unfiled", context.getString(R.string.bookmarks_folder_unfiled));
      m.put("mobile",  context.getString(R.string.bookmarks_folder_mobile));
      SPECIAL_GUIDS_MAP = Collections.unmodifiableMap(m);
    }

    dbHelper = new AndroidBrowserBookmarksDataAccessor(context);
    dataAccessor = (AndroidBrowserBookmarksDataAccessor) dbHelper;
  }

  private boolean rowIsFolder(Cursor cur) {
    return RepoUtils.getLongFromCursor(cur, BrowserContract.Bookmarks.IS_FOLDER) == 1;
  }

  private String getGUIDForID(long androidID) {
    String guid = idToGuid.get(androidID);
    trace("  " + androidID + " => " + guid);
    return guid;
  }

  private long getIDForGUID(String guid) {
    Long id = guidToID.get(guid);
    if (id == null) {
      Logger.warn(LOG_TAG, "Couldn't find local ID for GUID " + guid);
      return -1;
    }
    return id.longValue();
  }

  private String getGUID(Cursor cur) {
    return RepoUtils.getStringFromCursor(cur, "guid");
  }

  private long getParentID(Cursor cur) {
    return RepoUtils.getLongFromCursor(cur, BrowserContract.Bookmarks.PARENT);
  }

  
  private long getPosition(Cursor cur, int positionIndex) {
    return cur.getLong(positionIndex);
  }
  private long getPosition(Cursor cur) {
    return RepoUtils.getLongFromCursor(cur, BrowserContract.Bookmarks.POSITION);
  }

  private String getParentName(String parentGUID) throws ParentNotFoundException, NullCursorException {
    if (parentGUID == null) {
      return "";
    }
    if (SPECIAL_GUIDS_MAP.containsKey(parentGUID)) {
      return SPECIAL_GUIDS_MAP.get(parentGUID);
    }

    
    String parentName = "";
    Cursor name = dataAccessor.fetch(new String[] { parentGUID });
    try {
      name.moveToFirst();
      if (!name.isAfterLast()) {
        parentName = RepoUtils.getStringFromCursor(name, BrowserContract.Bookmarks.TITLE);
      }
      else {
        Logger.error(LOG_TAG, "Couldn't find record with guid '" + parentGUID + "' when looking for parent name.");
        throw new ParentNotFoundException(null);
      }
    } finally {
      name.close();
    }
    return parentName;
  }

  











  @SuppressWarnings("unchecked")
  private JSONArray getChildrenArray(long folderID, boolean persist) throws NullCursorException {
    trace("Calling getChildren for androidID " + folderID);
    JSONArray childArray = new JSONArray();
    Cursor children = dataAccessor.getChildren(folderID);
    try {
      if (!children.moveToFirst()) {
        trace("No children: empty cursor.");
        return childArray;
      }
      final int positionIndex = children.getColumnIndex(BrowserContract.Bookmarks.POSITION);
      final int count = children.getCount();
      Logger.debug(LOG_TAG, "Expecting " + count + " children.");

      
      TreeMap<Long, ArrayList<String>> guids = new TreeMap<Long, ArrayList<String>>();

      while (!children.isAfterLast()) {
        final String childGuid   = getGUID(children);
        final long childPosition = getPosition(children, positionIndex);
        trace("  Child GUID: " + childGuid);
        trace("  Child position: " + childPosition);
        Utils.addToIndexBucketMap(guids, Math.abs(childPosition), childGuid);
        children.moveToNext();
      }

      
      
      
      
      
      boolean changed = false;
      int i = 0;
      for (Entry<Long, ArrayList<String>> entry : guids.entrySet()) {
        long pos = entry.getKey().longValue();
        int atPos = entry.getValue().size();

        
        
        if (atPos > 1 || pos != i) {
          changed = true;
        }
        for (String guid : entry.getValue()) {
          if (!forbiddenGUID(guid)) {
            childArray.add(guid);
          }
        }
      }

      if (Logger.logVerbose(LOG_TAG)) {
        
        Logger.trace(LOG_TAG, "Output child array: " + childArray.toJSONString());
      }

      if (!changed) {
        Logger.debug(LOG_TAG, "Nothing moved! Database reflects child array.");
        return childArray;
      }

      if (!persist) {
        return childArray;
      }

      Logger.debug(LOG_TAG, "Generating child array required moving records. Updating DB.");
      final long time = now();
      if (0 < dataAccessor.updatePositions(childArray)) {
        Logger.debug(LOG_TAG, "Bumping parent time to " + time + ".");
        dataAccessor.bumpModified(folderID, time);
      }
    } finally {
      children.close();
    }

    return childArray;
  }

  protected static boolean isDeleted(Cursor cur) {
    return RepoUtils.getLongFromCursor(cur, BrowserContract.SyncColumns.IS_DELETED) != 0;
  }

  @Override
  protected Record retrieveDuringStore(Cursor cur) throws NoGuidForIdException, NullCursorException, ParentNotFoundException {
    
    
    
    return retrieveRecord(cur, false);
  }

  @Override
  protected Record retrieveDuringFetch(Cursor cur) throws NoGuidForIdException, NullCursorException, ParentNotFoundException {
    return retrieveRecord(cur, true);
  }

  



  protected BookmarkRecord retrieveRecord(Cursor cur, boolean computeAndPersistChildren) throws NoGuidForIdException, NullCursorException, ParentNotFoundException {
    String recordGUID = getGUID(cur);
    Logger.trace(LOG_TAG, "Record from mirror cursor: " + recordGUID);

    if (forbiddenGUID(recordGUID)) {
      Logger.debug(LOG_TAG, "Ignoring " + recordGUID + " record in recordFromMirrorCursor.");
      return null;
    }

    
    if (isDeleted(cur)) {
      return AndroidBrowserBookmarksRepositorySession.bookmarkFromMirrorCursor(cur, null, null, null);
    }

    long androidParentID = getParentID(cur);

    
    String androidParentGUID = SPECIAL_GUID_PARENTS.get(recordGUID);
    if (androidParentGUID == null) {
      androidParentGUID = getGUIDForID(androidParentID);
    }

    boolean needsReparenting = false;

    if (androidParentGUID == null) {
      Logger.debug(LOG_TAG, "No parent GUID for record " + recordGUID + " with parent " + androidParentID);
      
      if (idToGuid.containsKey(androidParentID)) {
        Logger.error(LOG_TAG, "Have the parent android ID for the record but the parent's GUID wasn't found.");
        throw new NoGuidForIdException(null);
      }

      
      
      
      needsReparenting = true;
    }

    
    final JSONArray childArray;
    if (computeAndPersistChildren) {
      childArray = getChildrenArrayForRecordCursor(cur, recordGUID, true);
    } else {
      childArray = null;
    }
    String parentName = getParentName(androidParentGUID);
    BookmarkRecord bookmark = AndroidBrowserBookmarksRepositorySession.bookmarkFromMirrorCursor(cur, androidParentGUID, parentName, childArray);

    if (needsReparenting) {
      Logger.warn(LOG_TAG, "Bookmark record " + recordGUID + " has a bad parent pointer. Reparenting now.");

      String destination = bookmark.deleted ? "unfiled" : "mobile";
      bookmark.androidParentID = getIDForGUID(destination);
      bookmark.androidPosition = getPosition(cur);
      bookmark.parentID        = destination;
      bookmark.parentName      = getParentName(destination);
      if (!bookmark.deleted) {
        
        
        relocateBookmark(bookmark);
      }
    }

    return bookmark;
  }

  





  private void relocateBookmark(BookmarkRecord bookmark) {
    dataAccessor.updateParentAndPosition(bookmark.guid, bookmark.androidParentID, bookmark.androidPosition);
  }

  protected JSONArray getChildrenArrayForRecordCursor(Cursor cur, String recordGUID, boolean persist) throws NullCursorException {
    boolean isFolder = rowIsFolder(cur);
    if (!isFolder) {
      return null;
    }

    long androidID = guidToID.get(recordGUID);
    JSONArray childArray = getChildrenArray(androidID, persist);
    if (childArray == null) {
      return null;
    }

    Logger.debug(LOG_TAG, "Fetched " + childArray.size() + " children for " + recordGUID);
    return childArray;
  }

  @Override
  protected boolean checkRecordType(Record record) {
    BookmarkRecord bmk = (BookmarkRecord) record;
    if (bmk.type.equalsIgnoreCase(AndroidBrowserBookmarksDataAccessor.TYPE_BOOKMARK) ||
        bmk.type.equalsIgnoreCase(AndroidBrowserBookmarksDataAccessor.TYPE_FOLDER)) {
      return true;
    }
    Logger.info(LOG_TAG, "Ignoring record with guid: " + record.guid + " and type: " + ((BookmarkRecord)record).type);
    return false;
  }
  
  @Override
  public void begin(RepositorySessionBeginDelegate delegate) {
    
    
    Cursor cur;
    try {
      Logger.debug(LOG_TAG, "Check and build special GUIDs.");
      dataAccessor.checkAndBuildSpecialGuids();
      cur = dataAccessor.getGuidsIDsForFolders();
      Logger.debug(LOG_TAG, "Got GUIDs for folders.");
    } catch (android.database.sqlite.SQLiteConstraintException e) {
      Logger.error(LOG_TAG, "Got sqlite constraint exception working with Fennec bookmark DB.", e);
      delegate.onBeginFailed(e);
      return;
    } catch (NullCursorException e) {
      delegate.onBeginFailed(e);
      return;
    } catch (Exception e) {
      delegate.onBeginFailed(e);
      return;
    }
    
    
    

    Logger.debug(LOG_TAG, "Preparing folder ID mappings.");

    
    Logger.debug(LOG_TAG, "Tracking places root as ID 0.");
    idToGuid.put(0L, "places");
    guidToID.put("places", 0L);
    try {
      cur.moveToFirst();
      while (!cur.isAfterLast()) {
        String guid = getGUID(cur);
        long id = RepoUtils.getLongFromCursor(cur, BrowserContract.Bookmarks._ID);
        guidToID.put(guid, id);
        idToGuid.put(id, guid);
        Logger.debug(LOG_TAG, "GUID " + guid + " maps to " + id);
        cur.moveToNext();
      }
    } finally {
      cur.close();
    }
    Logger.debug(LOG_TAG, "Done with initial setup of bookmarks session.");
    super.begin(delegate);
  }

  @Override
  public void finish(RepositorySessionFinishDelegate delegate) {
    
    
    if (needsReparenting != 0) {
      Logger.error(LOG_TAG, "Finish called but " + needsReparenting +
                            " bookmark(s) have been placed in unsorted bookmarks and not been reparented.");

      
      
    }
    super.finish(delegate);
  };

  @Override
  protected Record reconcileRecords(Record remoteRecord, Record localRecord,
                                    long lastRemoteRetrieval,
                                    long lastLocalRetrieval) {

    BookmarkRecord reconciled = (BookmarkRecord) super.reconcileRecords(remoteRecord, localRecord,
                                                                        lastRemoteRetrieval,
                                                                        lastLocalRetrieval);

    
    
    reconciled.children = ((BookmarkRecord) remoteRecord).children;
    return reconciled;
  }

  @Override
  protected Record prepareRecord(Record record) {
    BookmarkRecord bmk = (BookmarkRecord) record;

    if (!isSpecialRecord(record)) {
      
      handleParenting(bmk);
    }

    if (Logger.LOG_PERSONAL_INFORMATION) {
      if (bmk.isFolder()) {
        Logger.pii(LOG_TAG, "Inserting folder " + bmk.guid + ", " + bmk.title +
                            " with parent " + bmk.androidParentID +
                            " (" + bmk.parentID + ", " + bmk.parentName +
                            ", " + bmk.pos + ")");
      } else {
        Logger.pii(LOG_TAG, "Inserting bookmark " + bmk.guid + ", " + bmk.title + ", " +
                            bmk.bookmarkURI + " with parent " + bmk.androidParentID +
                            " (" + bmk.parentID + ", " + bmk.parentName +
                            ", " + bmk.pos + ")");
      }
    } else {
      if (bmk.isFolder()) {
        Logger.debug(LOG_TAG, "Inserting folder " + bmk.guid +  ", parent " +
                              bmk.androidParentID +
                              " (" + bmk.parentID + ", " + bmk.pos + ")");
      } else {
        Logger.debug(LOG_TAG, "Inserting bookmark " + bmk.guid + " with parent " +
                              bmk.androidParentID +
                              " (" + bmk.parentID + ", " + ", " + bmk.pos + ")");
      }
    }
    return bmk;
  }

  





  private void handleParenting(BookmarkRecord bmk) {
    if (guidToID.containsKey(bmk.parentID)) {
      bmk.androidParentID = guidToID.get(bmk.parentID);

      
      JSONArray children = parentToChildArray.get(bmk.parentID);
      if (children != null) {
        int index = children.indexOf(bmk.guid);
        if (index >= 0) {
          bmk.androidPosition = index;
        }
      }
    }
    else {
      bmk.androidParentID = guidToID.get("unfiled");
      ArrayList<String> children;
      if (missingParentToChildren.containsKey(bmk.parentID)) {
        children = missingParentToChildren.get(bmk.parentID);
      } else {
        children = new ArrayList<String>();
      }
      children.add(bmk.guid);
      needsReparenting++;
      missingParentToChildren.put(bmk.parentID, children);
    }
  }

  private boolean isSpecialRecord(Record record) {
    return SPECIAL_GUID_PARENTS.containsKey(record.guid);
  }

  @Override
  protected void updateBookkeeping(Record record) throws NoGuidForIdException,
                                                         NullCursorException,
                                                         ParentNotFoundException {
    super.updateBookkeeping(record);
    BookmarkRecord bmk = (BookmarkRecord) record;

    
    if (!bmk.isFolder()) {
      Logger.debug(LOG_TAG, "Not a folder. No bookkeeping.");
      return;
    }

    Logger.debug(LOG_TAG, "Updating bookkeeping for folder " + record.guid);

    
    
    
    guidToID.put(bmk.guid,      bmk.androidID);
    idToGuid.put(bmk.androidID, bmk.guid);

    JSONArray childArray = bmk.children;

    if (Logger.logVerbose(LOG_TAG)) {
      Logger.trace(LOG_TAG, bmk.guid + " has children " + childArray.toJSONString());
    }
    parentToChildArray.put(bmk.guid, childArray);

    
    if (missingParentToChildren.containsKey(bmk.guid)) {
      for (String child : missingParentToChildren.get(bmk.guid)) {
        
        
        long position = childArray.indexOf(child);
        dataAccessor.updateParentAndPosition(child, bmk.androidID, position);
        needsReparenting--;
      }
      missingParentToChildren.remove(bmk.guid);
    }
  }

  @Override
  protected void storeRecordDeletion(final Record record) {
    if (SPECIAL_GUIDS_MAP.containsKey(record.guid)) {
      Logger.debug(LOG_TAG, "Told to delete record " + record.guid + ". Ignoring.");
      return;
    }
    final BookmarkRecord bookmarkRecord = (BookmarkRecord) record;
    if (bookmarkRecord.isFolder()) {
      Logger.debug(LOG_TAG, "Deleting folder. Ensuring consistency of children.");
      handleFolderDeletion(bookmarkRecord);
      return;
    }
    super.storeRecordDeletion(record);
  }

  











  protected void handleFolderDeletion(final BookmarkRecord folder) {
    
    
    super.storeRecordDeletion(folder);
  }

  @SuppressWarnings("unchecked")
  private void finishUp() {
    try {
      Logger.debug(LOG_TAG, "Have " + parentToChildArray.size() + " folders whose children might need repositioning.");
      for (Entry<String, JSONArray> entry : parentToChildArray.entrySet()) {
        String guid = entry.getKey();
        JSONArray onServer = entry.getValue();
        try {
          final long folderID = getIDForGUID(guid);
          JSONArray inDB = getChildrenArray(folderID, false);
          int added = 0;
          for (Object o : inDB) {
            if (!onServer.contains(o)) {
              onServer.add(o);
              added++;
            }
          }
          Logger.debug(LOG_TAG, "Added " + added + " items locally.");
          dataAccessor.updatePositions(new ArrayList<String>(onServer));
          dataAccessor.bumpModified(folderID, now());
          
          Logger.debug(LOG_TAG, "Untracking " + guid);
          final Record record = retrieveByGUIDDuringStore(guid);
          if (record == null) {
            return;
          }
          untrackRecord(record);
        } catch (Exception e) {
          Logger.warn(LOG_TAG, "Error repositioning children for " + guid, e);
        }
      }
    } finally {
      super.storeDone();
    }
  }

  @Override
  public void storeDone() {
    Runnable command = new Runnable() {
      @Override
      public void run() {
        finishUp();
      }
    };
    storeWorkQueue.execute(command);
  }

  @Override
  protected String buildRecordString(Record record) {
    BookmarkRecord bmk = (BookmarkRecord) record;
    return bmk.title + bmk.bookmarkURI + bmk.type + bmk.parentName;
  }

  public static BookmarkRecord computeParentFields(BookmarkRecord rec, String suggestedParentGUID, String suggestedParentName) {
    final String guid = rec.guid;
    if (guid == null) {
      
      Logger.error(LOG_TAG, "No guid in computeParentFields!");
      return null;
    }

    String realParent = SPECIAL_GUID_PARENTS.get(guid);
    if (realParent == null) {
      
      realParent = suggestedParentGUID;
    } else {
      Logger.debug(LOG_TAG, "Ignoring suggested parent ID " + suggestedParentGUID +
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

  private static BookmarkRecord logBookmark(BookmarkRecord rec) {
    try {
      Logger.debug(LOG_TAG, "Returning " + (rec.deleted ? "deleted " : "") +
                            "bookmark record " + rec.guid + " (" + rec.androidID +
                           ", parent " + rec.parentID + ")");
      if (!rec.deleted && Logger.LOG_PERSONAL_INFORMATION) {
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

  
  public static BookmarkRecord bookmarkFromMirrorCursor(Cursor cur, String parentGUID, String parentName, JSONArray children) {
    final String collection = "bookmarks";
    final String guid       = RepoUtils.getStringFromCursor(cur, BrowserContract.SyncColumns.GUID);
    final long lastModified = RepoUtils.getLongFromCursor(cur,   BrowserContract.SyncColumns.DATE_MODIFIED);
    final boolean deleted   = isDeleted(cur);
    BookmarkRecord rec = new BookmarkRecord(guid, collection, lastModified, deleted);

    
    if (deleted) {
      return logBookmark(rec);
    }

    boolean isFolder  = RepoUtils.getIntFromCursor(cur, BrowserContract.Bookmarks.IS_FOLDER) == 1;

    rec.title = RepoUtils.getStringFromCursor(cur, BrowserContract.Bookmarks.TITLE);
    rec.bookmarkURI = RepoUtils.getStringFromCursor(cur, BrowserContract.Bookmarks.URL);
    rec.description = RepoUtils.getStringFromCursor(cur, BrowserContract.Bookmarks.DESCRIPTION);
    rec.tags = RepoUtils.getJSONArrayFromCursor(cur, BrowserContract.Bookmarks.TAGS);
    rec.keyword = RepoUtils.getStringFromCursor(cur, BrowserContract.Bookmarks.KEYWORD);
    rec.type = isFolder ? AndroidBrowserBookmarksDataAccessor.TYPE_FOLDER :
                          AndroidBrowserBookmarksDataAccessor.TYPE_BOOKMARK;

    rec.androidID = RepoUtils.getLongFromCursor(cur, BrowserContract.Bookmarks._ID);
    rec.androidPosition = RepoUtils.getLongFromCursor(cur, BrowserContract.Bookmarks.POSITION);
    rec.children = children;

    
    
    
    BookmarkRecord withParentFields = computeParentFields(rec, parentGUID, parentName);
    if (withParentFields == null) {
      
      return null;
    }
    return logBookmark(withParentFields);
  }
}

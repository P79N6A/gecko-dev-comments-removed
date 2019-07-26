


package org.mozilla.gecko.background.db;

import java.util.ArrayList;

import org.json.simple.JSONArray;
import org.mozilla.gecko.background.sync.helpers.BookmarkHelpers;
import org.mozilla.gecko.background.sync.helpers.ExpectFetchDelegate;
import org.mozilla.gecko.background.sync.helpers.ExpectFetchSinceDelegate;
import org.mozilla.gecko.background.sync.helpers.ExpectFinishDelegate;
import org.mozilla.gecko.background.sync.helpers.ExpectGuidsSinceDelegate;
import org.mozilla.gecko.background.sync.helpers.ExpectInvalidTypeStoreDelegate;
import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.repositories.InactiveSessionException;
import org.mozilla.gecko.sync.repositories.NullCursorException;
import org.mozilla.gecko.sync.repositories.RepositorySession;
import org.mozilla.gecko.sync.repositories.android.AndroidBrowserBookmarksDataAccessor;
import org.mozilla.gecko.sync.repositories.android.AndroidBrowserBookmarksRepository;
import org.mozilla.gecko.sync.repositories.android.AndroidBrowserBookmarksRepositorySession;
import org.mozilla.gecko.sync.repositories.android.AndroidBrowserRepository;
import org.mozilla.gecko.sync.repositories.android.AndroidBrowserRepositoryDataAccessor;
import org.mozilla.gecko.sync.repositories.android.BrowserContractHelpers;
import org.mozilla.gecko.sync.repositories.android.RepoUtils;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionCreationDelegate;
import org.mozilla.gecko.sync.repositories.domain.BookmarkRecord;
import org.mozilla.gecko.sync.repositories.domain.Record;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;

public class TestAndroidBrowserBookmarksRepository extends AndroidBrowserRepositoryTestCase {

  @Override
  protected AndroidBrowserRepository getRepository() {

    



    return new AndroidBrowserBookmarksRepository() {
      @Override
      protected void sessionCreator(RepositorySessionCreationDelegate delegate, Context context) {
        AndroidBrowserBookmarksRepositorySession session;
        session = new AndroidBrowserBookmarksRepositorySession(this, context) {
          @Override
          protected synchronized void trackGUID(String guid) {
            System.out.println("Ignoring trackGUID call: this is a test!");
          }
        };
        delegate.deferredCreationDelegate().onSessionCreated(session);
      }
    };
  }

  @Override
  protected AndroidBrowserRepositoryDataAccessor getDataAccessor() {
    return new AndroidBrowserBookmarksDataAccessor(getApplicationContext());
  }

  


  @Override
  public ExpectFetchDelegate preparedExpectFetchDelegate(Record[] expected) {
    ExpectFetchDelegate delegate = new ExpectFetchDelegate(expected);
    delegate.ignore.addAll(AndroidBrowserBookmarksRepositorySession.SPECIAL_GUIDS_MAP.keySet());
    return delegate;
  }

  


  public ExpectGuidsSinceDelegate preparedExpectOnlySpecialGuidsSinceDelegate() {
    ExpectGuidsSinceDelegate delegate = new ExpectGuidsSinceDelegate(AndroidBrowserBookmarksRepositorySession.SPECIAL_GUIDS_MAP.keySet().toArray(new String[] {}));
    return delegate;
  }

  


  @Override
  public ExpectGuidsSinceDelegate preparedExpectGuidsSinceDelegate(String[] expected) {
    ExpectGuidsSinceDelegate delegate = new ExpectGuidsSinceDelegate(expected);
    delegate.ignore.addAll(AndroidBrowserBookmarksRepositorySession.SPECIAL_GUIDS_MAP.keySet());
    return delegate;
  }

  


  public ExpectFetchSinceDelegate preparedExpectFetchSinceDelegate(long timestamp, String[] expected) {
    ExpectFetchSinceDelegate delegate = new ExpectFetchSinceDelegate(timestamp, expected);
    delegate.ignore.addAll(AndroidBrowserBookmarksRepositorySession.SPECIAL_GUIDS_MAP.keySet());
    return delegate;
  }

  
  
  
  
  
  
  

  public void testFetchOneWithChildren() {
    BookmarkRecord folder = BookmarkHelpers.createFolder1();
    BookmarkRecord bookmark1 = BookmarkHelpers.createBookmark1();
    BookmarkRecord bookmark2 = BookmarkHelpers.createBookmark2();

    RepositorySession session = createAndBeginSession();

    Record[] records = new Record[] { folder, bookmark1, bookmark2 };
    performWait(storeManyRunnable(session, records));

    AndroidBrowserRepositoryDataAccessor helper = getDataAccessor();
    helper.dumpDB();
    closeDataAccessor(helper);

    String[] guids = new String[] { folder.guid };
    Record[] expected = new Record[] { folder };
    performWait(fetchRunnable(session, guids, expected));
    dispose(session);
  }

  @Override
  public void testFetchAll() {
    Record[] expected = new Record[3];
    expected[0] = BookmarkHelpers.createFolder1();
    expected[1] = BookmarkHelpers.createBookmark1();
    expected[2] = BookmarkHelpers.createBookmark2();
    basicFetchAllTest(expected);
  }

  @Override
  public void testGuidsSinceReturnMultipleRecords() {
    BookmarkRecord record0 = BookmarkHelpers.createBookmark1();
    BookmarkRecord record1 = BookmarkHelpers.createBookmark2();
    guidsSinceReturnMultipleRecords(record0, record1);
  }

  @Override
  public void testGuidsSinceReturnNoRecords() {
    guidsSinceReturnNoRecords(BookmarkHelpers.createBookmarkInMobileFolder1());
  }

  @Override
  public void testFetchSinceOneRecord() {
    fetchSinceOneRecord(BookmarkHelpers.createBookmarkInMobileFolder1(),
        BookmarkHelpers.createBookmarkInMobileFolder2());
  }

  @Override
  public void testFetchSinceReturnNoRecords() {
    fetchSinceReturnNoRecords(BookmarkHelpers.createBookmark1());
  }

  @Override
  public void testFetchOneRecordByGuid() {
    fetchOneRecordByGuid(BookmarkHelpers.createBookmarkInMobileFolder1(),
        BookmarkHelpers.createBookmarkInMobileFolder2());
  }

  @Override
  public void testFetchMultipleRecordsByGuids() {
    BookmarkRecord record0 = BookmarkHelpers.createFolder1();
    BookmarkRecord record1 = BookmarkHelpers.createBookmark1();
    BookmarkRecord record2 = BookmarkHelpers.createBookmark2();
    fetchMultipleRecordsByGuids(record0, record1, record2);
  }

  @Override
  public void testFetchNoRecordByGuid() {
    fetchNoRecordByGuid(BookmarkHelpers.createBookmark1());
  }


  @Override
  public void testWipe() {
    doWipe(BookmarkHelpers.createBookmarkInMobileFolder1(),
        BookmarkHelpers.createBookmarkInMobileFolder2());
  }

  @Override
  public void testStore() {
    basicStoreTest(BookmarkHelpers.createBookmark1());
  }


  public void testStoreFolder() {
    basicStoreTest(BookmarkHelpers.createFolder1());
  }

  



  




  

















  protected void basicStoreFailTest(Record record) {
    final RepositorySession session = createAndBeginSession();
    performWait(storeRunnable(session, record, new ExpectInvalidTypeStoreDelegate()));
    dispose(session);
  }

  


  
  
  public void testBasicReparenting() throws InactiveSessionException {
    Record[] expected = new Record[] {
        BookmarkHelpers.createBookmark1(),
        BookmarkHelpers.createBookmark2(),
        BookmarkHelpers.createFolder1()
    };
    doMultipleFolderReparentingTest(expected);
  }

  
  
  public void testMultipleFolderReparenting1() throws InactiveSessionException {
    Record[] expected = new Record[] {
        BookmarkHelpers.createBookmark1(),
        BookmarkHelpers.createBookmark2(),
        BookmarkHelpers.createBookmark3(),
        BookmarkHelpers.createFolder1(),
        BookmarkHelpers.createBookmark4(),
        BookmarkHelpers.createFolder3(),
        BookmarkHelpers.createFolder2(),
    };
    doMultipleFolderReparentingTest(expected);
  }

  public void testMultipleFolderReparenting2() throws InactiveSessionException {
    Record[] expected = new Record[] {
        BookmarkHelpers.createBookmark1(),
        BookmarkHelpers.createBookmark2(),
        BookmarkHelpers.createBookmark3(),
        BookmarkHelpers.createFolder1(),
        BookmarkHelpers.createBookmark4(),
        BookmarkHelpers.createFolder3(),
        BookmarkHelpers.createFolder2(),
    };
    doMultipleFolderReparentingTest(expected);
  }

  public void testMultipleFolderReparenting3() throws InactiveSessionException {
    Record[] expected = new Record[] {
        BookmarkHelpers.createBookmark1(),
        BookmarkHelpers.createBookmark2(),
        BookmarkHelpers.createBookmark3(),
        BookmarkHelpers.createFolder1(),
        BookmarkHelpers.createBookmark4(),
        BookmarkHelpers.createFolder3(),
        BookmarkHelpers.createFolder2(),
    };
    doMultipleFolderReparentingTest(expected);
  }

  private void doMultipleFolderReparentingTest(Record[] expected) throws InactiveSessionException {
    final RepositorySession session = createAndBeginSession();
    doStore(session, expected);
    ExpectFetchDelegate delegate = preparedExpectFetchDelegate(expected);
    performWait(fetchAllRunnable(session, delegate));
    performWait(finishRunnable(session, new ExpectFinishDelegate()));
  }

  




  @Override
  public void testStoreIdenticalExceptGuid() {
    storeIdenticalExceptGuid(BookmarkHelpers.createBookmarkInMobileFolder1());
  }

  







  public void testStoreIdenticalFoldersWithChildren() {
    final RepositorySession session = createAndBeginSession();
    Record record0 = BookmarkHelpers.createFolder1();

    
    
    
    BookmarkRecord rec0 = (BookmarkRecord) record0;
    rec0.children = new JSONArray();
    performWait(storeRunnable(session, record0));

    ExpectFetchDelegate timestampDelegate = preparedExpectFetchDelegate(new Record[] { rec0 });
    performWait(fetchRunnable(session, new String[] { record0.guid }, timestampDelegate));

    AndroidBrowserRepositoryDataAccessor helper = getDataAccessor();
    helper.dumpDB();
    closeDataAccessor(helper);

    Record record1 = BookmarkHelpers.createBookmark1();
    Record record2 = BookmarkHelpers.createBookmark2();
    Record record3 = BookmarkHelpers.createFolder1();
    BookmarkRecord bmk3 = (BookmarkRecord) record3;
    record3.guid = Utils.generateGuid();
    record3.lastModified = timestampDelegate.records.get(0).lastModified + 3000;
    assert(!record0.guid.equals(record3.guid));

    
    
    Record record4 = BookmarkHelpers.createBookmark3();
    BookmarkRecord bmk4 = (BookmarkRecord) record4;
    bmk4.parentID = bmk3.guid;
    bmk4.parentName = bmk3.parentName;

    doStore(session, new Record[] {
        record1, record2, record3, bmk4
    });
    BookmarkRecord bmk1 = (BookmarkRecord) record1;
    bmk1.parentID = record3.guid;
    BookmarkRecord bmk2 = (BookmarkRecord) record2;
    bmk2.parentID = record3.guid;
    Record[] expect = new Record[] {
        bmk1, bmk2, record3
    };
    fetchAllRunnable(session, preparedExpectFetchDelegate(expect));
    dispose(session);
  }

  @Override
  public void testRemoteNewerTimeStamp() {
    BookmarkRecord local = BookmarkHelpers.createBookmarkInMobileFolder1();
    BookmarkRecord remote = BookmarkHelpers.createBookmarkInMobileFolder2();
    remoteNewerTimeStamp(local, remote);
  }

  @Override
  public void testLocalNewerTimeStamp() {
    BookmarkRecord local = BookmarkHelpers.createBookmarkInMobileFolder1();
    BookmarkRecord remote = BookmarkHelpers.createBookmarkInMobileFolder2();
    localNewerTimeStamp(local, remote);
  }

  @Override
  public void testDeleteRemoteNewer() {
    BookmarkRecord local = BookmarkHelpers.createBookmarkInMobileFolder1();
    BookmarkRecord remote = BookmarkHelpers.createBookmarkInMobileFolder2();
    deleteRemoteNewer(local, remote);
  }

  @Override
  public void testDeleteLocalNewer() {
    BookmarkRecord local = BookmarkHelpers.createBookmarkInMobileFolder1();
    BookmarkRecord remote = BookmarkHelpers.createBookmarkInMobileFolder2();
    deleteLocalNewer(local, remote);
  }

  @Override
  public void testDeleteRemoteLocalNonexistent() {
    BookmarkRecord remote = BookmarkHelpers.createBookmark2();
    deleteRemoteLocalNonexistent(remote);
  }

  @Override
  public void testCleanMultipleRecords() {
    cleanMultipleRecords(
        BookmarkHelpers.createBookmarkInMobileFolder1(),
        BookmarkHelpers.createBookmarkInMobileFolder2(),
        BookmarkHelpers.createBookmark1(),
        BookmarkHelpers.createBookmark2(),
        BookmarkHelpers.createFolder1());
  }

  public void testBasicPositioning() {
    final RepositorySession session = createAndBeginSession();
    Record[] expected = new Record[] {
        BookmarkHelpers.createBookmark1(),
        BookmarkHelpers.createFolder1(),
        BookmarkHelpers.createBookmark2()
    };
    System.out.println("TEST: Inserting " + expected[0].guid + ", "
        + expected[1].guid + ", "
        + expected[2].guid);
    doStore(session, expected);

    ExpectFetchDelegate delegate = preparedExpectFetchDelegate(expected);
    performWait(fetchAllRunnable(session, delegate));

    int found = 0;
    boolean foundFolder = false;
    for (int i = 0; i < delegate.records.size(); i++) {
      BookmarkRecord rec = (BookmarkRecord) delegate.records.get(i);
      if (rec.guid.equals(expected[0].guid)) {
        assertEquals(0, ((BookmarkRecord) delegate.records.get(i)).androidPosition);
        found++;
      } else if (rec.guid.equals(expected[2].guid)) {
        assertEquals(1, ((BookmarkRecord) delegate.records.get(i)).androidPosition);
        found++;
      } else if (rec.guid.equals(expected[1].guid)) {
        foundFolder = true;
      } else {
        System.out.println("TEST: found " + rec.guid);
      }
    }
    assertTrue(foundFolder);
    assertEquals(2, found);
    dispose(session);
  }

  public void testSqlInjectPurgeDeleteAndUpdateByGuid() {
    
    RepositorySession session = createAndBeginSession();
    AndroidBrowserRepositoryDataAccessor db = getDataAccessor();

    ContentValues cv = new ContentValues();
    cv.put(BrowserContract.SyncColumns.IS_DELETED, 1);

    
    BookmarkRecord bmk1 = BookmarkHelpers.createBookmark1();
    BookmarkRecord bmk2 = BookmarkHelpers.createBookmark2();
    bmk2.guid = "' or '1'='1";

    db.insert(bmk1);
    db.insert(bmk2);

    
    db.updateByGuid(bmk2.guid, cv);

    
    Cursor cur = getAllBookmarks();
    int numBookmarks = cur.getCount();

    
    try {
      cur.moveToFirst();
      while (!cur.isAfterLast()) {
        String guid = RepoUtils.getStringFromCursor(cur, BrowserContract.SyncColumns.GUID);
        boolean deleted = RepoUtils.getLongFromCursor(cur, BrowserContract.SyncColumns.IS_DELETED) == 1;

        if (guid.equals(bmk2.guid)) {
          assertTrue(deleted);
        } else {
          assertFalse(deleted);
        }
        cur.moveToNext();
      }
    } finally {
      cur.close();
    }

    
    try {
      db.purgeDeleted();
    } catch (NullCursorException e) {
      e.printStackTrace();
    }

    cur = getAllBookmarks();
    int numBookmarksAfterDeletion = cur.getCount();

    
    assertEquals(numBookmarksAfterDeletion, numBookmarks - 1);

    
    try {
      cur.moveToFirst();
      while (!cur.isAfterLast()) {
        String guid = RepoUtils.getStringFromCursor(cur, BrowserContract.SyncColumns.GUID);
        boolean deleted = RepoUtils.getLongFromCursor(cur, BrowserContract.SyncColumns.IS_DELETED) == 1;

        if (guid.equals(bmk2.guid)) {
          fail("Evil guid was not deleted!");
        } else {
          assertFalse(deleted);
        }
        cur.moveToNext();
      }
    } finally {
      cur.close();
    }
    dispose(session);
  }

  protected Cursor getAllBookmarks() {
    Context context = getApplicationContext();
    Cursor cur = context.getContentResolver().query(BrowserContractHelpers.BOOKMARKS_CONTENT_URI,
        BrowserContractHelpers.BookmarkColumns, null, null, null);
    return cur;
  }

  public void testSqlInjectFetch() {
    
    RepositorySession session = createAndBeginSession();
    AndroidBrowserRepositoryDataAccessor db = getDataAccessor();

    
    BookmarkRecord bmk1 = BookmarkHelpers.createBookmark1();
    BookmarkRecord bmk2 = BookmarkHelpers.createBookmark2();
    BookmarkRecord bmk3 = BookmarkHelpers.createBookmark3();
    BookmarkRecord bmk4 = BookmarkHelpers.createBookmark4();
    bmk4.guid = "' or '1'='1";

    db.insert(bmk1);
    db.insert(bmk2);
    db.insert(bmk3);
    db.insert(bmk4);

    
    Cursor cur = null;
    try {
      cur = db.fetch(new String[] { bmk3.guid, bmk4.guid });
    } catch (NullCursorException e1) {
      e1.printStackTrace();
    }

    
    if (cur == null) {
      fail("No records were fetched.");
    }

    try {
      if (cur.getCount() != 2) {
        fail("Wrong number of guids fetched!");
      }
      cur.moveToFirst();
      while (!cur.isAfterLast()) {
        String guid = RepoUtils.getStringFromCursor(cur, BrowserContract.SyncColumns.GUID);
        if (!guid.equals(bmk3.guid) && !guid.equals(bmk4.guid)) {
          fail("Wrong guids were fetched!");
        }
        cur.moveToNext();
      }
    } finally {
      cur.close();
    }
    dispose(session);
  }

  public void testSqlInjectDelete() {
    
    RepositorySession session = createAndBeginSession();
    AndroidBrowserRepositoryDataAccessor db = getDataAccessor();

    
    BookmarkRecord bmk1 = BookmarkHelpers.createBookmark1();
    BookmarkRecord bmk2 = BookmarkHelpers.createBookmark2();
    bmk2.guid = "' or '1'='1";

    db.insert(bmk1);
    db.insert(bmk2);

    
    Cursor cur = getAllBookmarks();
    int numBookmarks = cur.getCount();

    db.purgeGuid(bmk2.guid);

    
    cur = getAllBookmarks();
    int numBookmarksAfterDelete = cur.getCount();

    
    assertEquals(numBookmarksAfterDelete, numBookmarks - 1);

    try {
      cur.moveToFirst();
      while (!cur.isAfterLast()) {
        String guid = RepoUtils.getStringFromCursor(cur, BrowserContract.SyncColumns.GUID);
        if (guid.equals(bmk2.guid)) {
          fail("Guid was not deleted!");
        }
        cur.moveToNext();
      }
    } finally {
      cur.close();
    }
    dispose(session);
  }

  



  public void testBulkInsert() throws NullCursorException {
    RepositorySession session = createAndBeginSession();
    AndroidBrowserRepositoryDataAccessor db = getDataAccessor();

    
    Cursor cur = db.fetch(new String[] { "mobile" } );
    assertEquals(1, cur.getCount());
    cur.moveToFirst();
    int mobileAndroidID = RepoUtils.getIntFromCursor(cur, BrowserContract.Bookmarks._ID);

    BookmarkRecord bookmark1 = BookmarkHelpers.createBookmarkInMobileFolder1();
    BookmarkRecord bookmark2 = BookmarkHelpers.createBookmarkInMobileFolder2();
    bookmark1.androidParentID = mobileAndroidID;
    bookmark2.androidParentID = mobileAndroidID;
    ArrayList<Record> recordList = new ArrayList<Record>();
    recordList.add(bookmark1);
    recordList.add(bookmark2);
    db.bulkInsert(recordList);

    String[] guids = new String[] { bookmark1.guid, bookmark2.guid };
    Record[] expected = new Record[] { bookmark1, bookmark2 };
    performWait(fetchRunnable(session, guids, expected));
    dispose(session);
  }
}

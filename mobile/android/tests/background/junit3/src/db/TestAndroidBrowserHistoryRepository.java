


package org.mozilla.gecko.background.db;

import java.util.ArrayList;

import org.json.simple.JSONObject;
import org.mozilla.gecko.background.sync.helpers.ExpectFetchDelegate;
import org.mozilla.gecko.background.sync.helpers.ExpectFinishDelegate;
import org.mozilla.gecko.background.sync.helpers.HistoryHelpers;
import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.repositories.InactiveSessionException;
import org.mozilla.gecko.sync.repositories.NullCursorException;
import org.mozilla.gecko.sync.repositories.Repository;
import org.mozilla.gecko.sync.repositories.RepositorySession;
import org.mozilla.gecko.sync.repositories.android.AndroidBrowserHistoryDataAccessor;
import org.mozilla.gecko.sync.repositories.android.AndroidBrowserHistoryRepository;
import org.mozilla.gecko.sync.repositories.android.AndroidBrowserHistoryRepositorySession;
import org.mozilla.gecko.sync.repositories.android.AndroidBrowserRepository;
import org.mozilla.gecko.sync.repositories.android.AndroidBrowserRepositoryDataAccessor;
import org.mozilla.gecko.sync.repositories.android.AndroidBrowserRepositorySession;
import org.mozilla.gecko.sync.repositories.android.BrowserContractHelpers;
import org.mozilla.gecko.sync.repositories.android.RepoUtils;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionCreationDelegate;
import org.mozilla.gecko.sync.repositories.domain.HistoryRecord;
import org.mozilla.gecko.sync.repositories.domain.Record;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;

public class TestAndroidBrowserHistoryRepository extends AndroidBrowserRepositoryTestCase {

  @Override
  protected AndroidBrowserRepository getRepository() {

    



    return new AndroidBrowserHistoryRepository() {
      @Override
      protected void sessionCreator(RepositorySessionCreationDelegate delegate, Context context) {
        AndroidBrowserHistoryRepositorySession session;
        session = new AndroidBrowserHistoryRepositorySession(this, context) {
          @Override
          protected synchronized void trackGUID(String guid) {
            System.out.println("Ignoring trackGUID call: this is a test!");
          }
        };
        delegate.onSessionCreated(session);
      }
    };
  }

  @Override
  protected AndroidBrowserRepositoryDataAccessor getDataAccessor() {
    return new AndroidBrowserHistoryDataAccessor(getApplicationContext());
  }

  @Override
  protected void closeDataAccessor(AndroidBrowserRepositoryDataAccessor dataAccessor) {
    if (!(dataAccessor instanceof AndroidBrowserHistoryDataAccessor)) {
      throw new IllegalArgumentException("Only expecting a history data accessor.");
    }
    ((AndroidBrowserHistoryDataAccessor) dataAccessor).closeExtender();
  }

  @Override
  public void testFetchAll() {
    Record[] expected = new Record[2];
    expected[0] = HistoryHelpers.createHistory3();
    expected[1] = HistoryHelpers.createHistory2();
    basicFetchAllTest(expected);
  }

  




  @Override
  public void testStoreIdenticalExceptGuid() {
    storeIdenticalExceptGuid(HistoryHelpers.createHistory1());
  }

  @Override
  public void testCleanMultipleRecords() {
    cleanMultipleRecords(
        HistoryHelpers.createHistory1(),
        HistoryHelpers.createHistory2(),
        HistoryHelpers.createHistory3(),
        HistoryHelpers.createHistory4(),
        HistoryHelpers.createHistory5()
        );
  }

  @Override
  public void testGuidsSinceReturnMultipleRecords() {
    HistoryRecord record0 = HistoryHelpers.createHistory1();
    HistoryRecord record1 = HistoryHelpers.createHistory2();
    guidsSinceReturnMultipleRecords(record0, record1);
  }

  @Override
  public void testGuidsSinceReturnNoRecords() {
    guidsSinceReturnNoRecords(HistoryHelpers.createHistory3());
  }

  @Override
  public void testFetchSinceOneRecord() {
    fetchSinceOneRecord(HistoryHelpers.createHistory1(),
        HistoryHelpers.createHistory2());
  }

  @Override
  public void testFetchSinceReturnNoRecords() {
    fetchSinceReturnNoRecords(HistoryHelpers.createHistory3());
  }

  @Override
  public void testFetchOneRecordByGuid() {
    fetchOneRecordByGuid(HistoryHelpers.createHistory1(),
        HistoryHelpers.createHistory2());
  }

  @Override
  public void testFetchMultipleRecordsByGuids() {
    HistoryRecord record0 = HistoryHelpers.createHistory1();
    HistoryRecord record1 = HistoryHelpers.createHistory2();
    HistoryRecord record2 = HistoryHelpers.createHistory3();
    fetchMultipleRecordsByGuids(record0, record1, record2);
  }

  @Override
  public void testFetchNoRecordByGuid() {
    fetchNoRecordByGuid(HistoryHelpers.createHistory1());
  }

  @Override
  public void testWipe() {
    doWipe(HistoryHelpers.createHistory2(), HistoryHelpers.createHistory3());
  }

  @Override
  public void testStore() {
    basicStoreTest(HistoryHelpers.createHistory1());
  }

  @Override
  public void testRemoteNewerTimeStamp() {
    HistoryRecord local = HistoryHelpers.createHistory1();
    HistoryRecord remote = HistoryHelpers.createHistory2();
    remoteNewerTimeStamp(local, remote);
  }

  @Override
  public void testLocalNewerTimeStamp() {
    HistoryRecord local = HistoryHelpers.createHistory1();
    HistoryRecord remote = HistoryHelpers.createHistory2();
    localNewerTimeStamp(local, remote);
  }

  @Override
  public void testDeleteRemoteNewer() {
    HistoryRecord local = HistoryHelpers.createHistory1();
    HistoryRecord remote = HistoryHelpers.createHistory2();
    deleteRemoteNewer(local, remote);
  }

  @Override
  public void testDeleteLocalNewer() {
    HistoryRecord local = HistoryHelpers.createHistory1();
    HistoryRecord remote = HistoryHelpers.createHistory2();
    deleteLocalNewer(local, remote);
  }

  @Override
  public void testDeleteRemoteLocalNonexistent() {
    deleteRemoteLocalNonexistent(HistoryHelpers.createHistory2());
  }

  


  protected class HelperHistorySession extends AndroidBrowserHistoryRepositorySession {
    public HelperHistorySession(Repository repository, Context context) {
      super(repository, context);
    }

    public boolean sameRecordString(HistoryRecord r1, HistoryRecord r2) {
      return buildRecordString(r1).equals(buildRecordString(r2));
    }
  }

  



  public void testRecordStringCollisionAndEquality() {
    final AndroidBrowserHistoryRepository repo = new AndroidBrowserHistoryRepository();
    final HelperHistorySession testSession = new HelperHistorySession(repo, getApplicationContext());

    final long now = RepositorySession.now();

    final HistoryRecord record0 = new HistoryRecord(null, "history", now + 1, false);
    final HistoryRecord record1 = new HistoryRecord(null, "history", now + 2, false);
    final HistoryRecord record2 = new HistoryRecord(null, "history", now + 3, false);

    record0.histURI = "http://example.com/foo";
    record1.histURI = "http://example.com/foo";
    record2.histURI = "http://example.com/bar";
    record0.title = "Foo 0";
    record1.title = "Foo 1";
    record2.title = "Foo 2";

    
    
    assertTrue(testSession.sameRecordString(record0, record1));
    assertFalse(testSession.sameRecordString(record0, record2));

    
    
    assertTrue(record0.congruentWith(record0));
    assertTrue(record0.congruentWith(record1));
    assertTrue(record1.congruentWith(record0));
    assertFalse(record0.congruentWith(record2));
    assertFalse(record1.congruentWith(record2));
    assertFalse(record2.congruentWith(record1));
    assertFalse(record2.congruentWith(record0));

    
    
    assertTrue(record0.equalPayloads(record0));
    assertTrue(record1.equalPayloads(record1));
    assertTrue(record2.equalPayloads(record2));
    assertFalse(record0.equalPayloads(record1));
    assertFalse(record1.equalPayloads(record0));
    assertFalse(record1.equalPayloads(record2));
  }

  



  @SuppressWarnings("unchecked")
  public void testAddOneVisit() {
    final RepositorySession session = createAndBeginSession();

    HistoryRecord record0 = HistoryHelpers.createHistory3();
    performWait(storeRunnable(session, record0));

    
    
    ContentValues cv = new ContentValues();
    int visits = record0.visits.size() + 1;
    long newVisitTime = System.currentTimeMillis();
    cv.put(BrowserContract.History.VISITS, visits);
    cv.put(BrowserContract.History.DATE_LAST_VISITED, newVisitTime);
    final AndroidBrowserRepositoryDataAccessor dataAccessor = getDataAccessor();
    dataAccessor.updateByGuid(record0.guid, cv);

    
    JSONObject expectedVisit = new JSONObject();
    expectedVisit.put("date", newVisitTime * 1000);    
    expectedVisit.put("type", 1L);
    record0.visits.add(expectedVisit);

    performWait(fetchRunnable(session, new String[] { record0.guid }, new ExpectFetchDelegate(new Record[] { record0 })));
    closeDataAccessor(dataAccessor);
  }

  @SuppressWarnings("unchecked")
  public void testAddMultipleVisits() {
    final RepositorySession session = createAndBeginSession();

    HistoryRecord record0 = HistoryHelpers.createHistory4();
    performWait(storeRunnable(session, record0));

    
    
    ContentValues cv = new ContentValues();
    int visits = record0.visits.size() + 3;
    long newVisitTime = System.currentTimeMillis();
    cv.put(BrowserContract.History.VISITS, visits);
    cv.put(BrowserContract.History.DATE_LAST_VISITED, newVisitTime);
    final AndroidBrowserRepositoryDataAccessor dataAccessor = getDataAccessor();
    dataAccessor.updateByGuid(record0.guid, cv);

    
    long newMicroVisitTime = newVisitTime * 1000;

    
    JSONObject expectedVisit = new JSONObject();
    expectedVisit.put("date", newMicroVisitTime);
    expectedVisit.put("type", 1L);
    record0.visits.add(expectedVisit);
    expectedVisit = new JSONObject();
    expectedVisit.put("date", newMicroVisitTime - 1000);
    expectedVisit.put("type", 1L);
    record0.visits.add(expectedVisit);
    expectedVisit = new JSONObject();
    expectedVisit.put("date", newMicroVisitTime - 2000);
    expectedVisit.put("type", 1L);
    record0.visits.add(expectedVisit);

    ExpectFetchDelegate delegate = new ExpectFetchDelegate(new Record[] { record0 });
    performWait(fetchRunnable(session, new String[] { record0.guid }, delegate));

    Record fetched = delegate.records.get(0);
    assertTrue(record0.equalPayloads(fetched));
    closeDataAccessor(dataAccessor);
  }

  public void testInvalidHistoryItemIsSkipped() throws NullCursorException {
    final AndroidBrowserHistoryRepositorySession session = (AndroidBrowserHistoryRepositorySession) createAndBeginSession();
    final AndroidBrowserRepositoryDataAccessor dbHelper = session.getDBHelper();

    final long now = System.currentTimeMillis();
    final HistoryRecord emptyURL = new HistoryRecord(Utils.generateGuid(), "history", now, false);
    final HistoryRecord noVisits = new HistoryRecord(Utils.generateGuid(), "history", now, false);
    final HistoryRecord aboutURL = new HistoryRecord(Utils.generateGuid(), "history", now, false);

    emptyURL.fennecDateVisited = now;
    emptyURL.fennecVisitCount  = 1;
    emptyURL.histURI           = "";
    emptyURL.title             = "Something";

    noVisits.fennecDateVisited = now;
    noVisits.fennecVisitCount  = 0;
    noVisits.histURI           = "http://example.org/novisits";
    noVisits.title             = "Something Else";

    aboutURL.fennecDateVisited = now;
    aboutURL.fennecVisitCount  = 1;
    aboutURL.histURI           = "about:home";
    aboutURL.title             = "Fennec Home";

    Uri one = dbHelper.insert(emptyURL);
    Uri two = dbHelper.insert(noVisits);
    Uri tre = dbHelper.insert(aboutURL);
    assertNotNull(one);
    assertNotNull(two);
    assertNotNull(tre);

    
    final Cursor all = dbHelper.fetchAll();
    assertEquals(3, all.getCount());
    all.close();

    
    performWait(fetchAllRunnable(session, new Record[] {}));

    
    assertTrue(session.shouldIgnore(aboutURL));

    session.abort();
  }

  public void testSqlInjectPurgeDelete() {
    
    RepositorySession session = createAndBeginSession();
    final AndroidBrowserRepositoryDataAccessor db = getDataAccessor();

    try {
      ContentValues cv = new ContentValues();
      cv.put(BrowserContract.SyncColumns.IS_DELETED, 1);

      
      HistoryRecord h1 = HistoryHelpers.createHistory1();
      HistoryRecord h2 = HistoryHelpers.createHistory2();
      h2.guid = "' or '1'='1";

      db.insert(h1);
      db.insert(h2);

      
      db.updateByGuid(h2.guid, cv);

      
      Cursor cur = getAllHistory();
      int numHistory = cur.getCount();

      
      try {
        cur.moveToFirst();
        while (!cur.isAfterLast()) {
          String guid = RepoUtils.getStringFromCursor(cur, BrowserContract.SyncColumns.GUID);
          boolean deleted = RepoUtils.getLongFromCursor(cur, BrowserContract.SyncColumns.IS_DELETED) == 1;

          if (guid.equals(h2.guid)) {
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

      cur = getAllHistory();
      int numHistoryAfterDeletion = cur.getCount();

      
      assertEquals(numHistoryAfterDeletion, numHistory - 1);

      
      try {
        cur.moveToFirst();
        while (!cur.isAfterLast()) {
          String guid = RepoUtils.getStringFromCursor(cur, BrowserContract.SyncColumns.GUID);
          boolean deleted = RepoUtils.getLongFromCursor(cur, BrowserContract.SyncColumns.IS_DELETED) == 1;

          if (guid.equals(h2.guid)) {
            fail("Evil guid was not deleted!");
          } else {
            assertFalse(deleted);
          }
          cur.moveToNext();
        }
      } finally {
        cur.close();
      }
    } finally {
      closeDataAccessor(db);
      session.abort();
    }
  }

  protected Cursor getAllHistory() {
    Context context = getApplicationContext();
    Cursor cur = context.getContentResolver().query(BrowserContractHelpers.HISTORY_CONTENT_URI,
        BrowserContractHelpers.HistoryColumns, null, null, null);
    return cur;
  }

  public void testDataAccessorBulkInsert() throws NullCursorException {
    final AndroidBrowserHistoryRepositorySession session = (AndroidBrowserHistoryRepositorySession) createAndBeginSession();
    AndroidBrowserHistoryDataAccessor db = (AndroidBrowserHistoryDataAccessor) session.getDBHelper();

    ArrayList<HistoryRecord> records = new ArrayList<HistoryRecord>();
    records.add(HistoryHelpers.createHistory1());
    records.add(HistoryHelpers.createHistory2());
    records.add(HistoryHelpers.createHistory3());
    db.bulkInsert(records);

    performWait(fetchAllRunnable(session, preparedExpectFetchDelegate(records.toArray(new Record[records.size()]))));
    session.abort();
  }

  public void testDataExtenderIsClosedBeforeBegin() {
    
    final AndroidBrowserRepositorySession session = (AndroidBrowserRepositorySession) createSession();
    AndroidBrowserHistoryDataAccessor db = (AndroidBrowserHistoryDataAccessor) session.getDBHelper();

    
    assertTrue(db.getHistoryDataExtender().isClosed());
  }

  public void testDataExtenderIsClosedAfterFinish() throws InactiveSessionException {
    final AndroidBrowserHistoryRepositorySession session = (AndroidBrowserHistoryRepositorySession) createAndBeginSession();
    AndroidBrowserHistoryDataAccessor db = (AndroidBrowserHistoryDataAccessor) session.getDBHelper();

    
    HistoryRecord h1 = HistoryHelpers.createHistory1();
    db.insert(h1);
    assertFalse(db.getHistoryDataExtender().isClosed());

    
    performWait(finishRunnable(session, new ExpectFinishDelegate()));
    assertTrue(db.getHistoryDataExtender().isClosed());
  }

  public void testDataExtenderIsClosedAfterAbort() throws InactiveSessionException {
    final AndroidBrowserHistoryRepositorySession session = (AndroidBrowserHistoryRepositorySession) createAndBeginSession();
    AndroidBrowserHistoryDataAccessor db = (AndroidBrowserHistoryDataAccessor) session.getDBHelper();

    
    HistoryRecord h1 = HistoryHelpers.createHistory1();
    db.insert(h1);
    assertFalse(db.getHistoryDataExtender().isClosed());

    
    session.abort();
    assertTrue(db.getHistoryDataExtender().isClosed());
  }
}

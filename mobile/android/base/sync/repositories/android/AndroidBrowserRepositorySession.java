





































package org.mozilla.gecko.sync.repositories.android;

import java.util.ArrayList;
import java.util.HashMap;

import org.mozilla.gecko.sync.repositories.InactiveSessionException;
import org.mozilla.gecko.sync.repositories.InvalidRequestException;
import org.mozilla.gecko.sync.repositories.InvalidSessionTransitionException;
import org.mozilla.gecko.sync.repositories.MultipleRecordsForGuidException;
import org.mozilla.gecko.sync.repositories.NoGuidForIdException;
import org.mozilla.gecko.sync.repositories.NoStoreDelegateException;
import org.mozilla.gecko.sync.repositories.NullCursorException;
import org.mozilla.gecko.sync.repositories.ParentNotFoundException;
import org.mozilla.gecko.sync.repositories.ProfileDatabaseException;
import org.mozilla.gecko.sync.repositories.Repository;
import org.mozilla.gecko.sync.repositories.RepositorySession;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionBeginDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionFetchRecordsDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionGuidsSinceDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionWipeDelegate;
import org.mozilla.gecko.sync.repositories.domain.Record;

import android.database.Cursor;
import android.util.Log;























public abstract class AndroidBrowserRepositorySession extends RepositorySession {

  protected AndroidBrowserRepositoryDataAccessor dbHelper;
  protected static final String LOG_TAG = "AndroidBrowserRepositorySession";
  private HashMap<String, String> recordToGuid;

  public AndroidBrowserRepositorySession(Repository repository) {
    super(repository);
  }

  









  protected abstract Record recordFromMirrorCursor(Cursor cur) throws NoGuidForIdException, NullCursorException, ParentNotFoundException;

  
  protected boolean checkRecordType(Record record) {
    return true;
  }

  








  protected Record transformRecord(Record record) throws NullCursorException {
    return record;
  }

  @Override
  public void begin(RepositorySessionBeginDelegate delegate) {
    RepositorySessionBeginDelegate deferredDelegate = delegate.deferredBeginDelegate(delegateQueue);
    try {
      super.sharedBegin();
    } catch (InvalidSessionTransitionException e) {
      deferredDelegate.onBeginFailed(e);
      return;
    }

    try {
      
      
      
      checkDatabase();
    } catch (ProfileDatabaseException e) {
      Log.e(LOG_TAG, "ProfileDatabaseException from begin. Fennec must be launched once until this error is fixed");
      deferredDelegate.onBeginFailed(e);
      return;
    } catch (NullCursorException e) {
      deferredDelegate.onBeginFailed(e);
      return;
    } catch (Exception e) {
      deferredDelegate.onBeginFailed(e);
      return;
    }
    deferredDelegate.onBeginSucceeded(this);
  }

  protected abstract String buildRecordString(Record record);

  protected void checkDatabase() throws ProfileDatabaseException, NullCursorException {
    Log.i(LOG_TAG, "Checking database.");
    try {
      dbHelper.fetch(new String[] { "none" }).close();
    } catch (NullPointerException e) {
      throw new ProfileDatabaseException(e);
    }
  }

  @Override
  public void guidsSince(long timestamp, RepositorySessionGuidsSinceDelegate delegate) {
    GuidsSinceRunnable command = new GuidsSinceRunnable(timestamp, delegate);
    delegateQueue.execute(command);
  }

  class GuidsSinceRunnable implements Runnable {

    private RepositorySessionGuidsSinceDelegate delegate;
    private long                                timestamp;

    public GuidsSinceRunnable(long timestamp,
                              RepositorySessionGuidsSinceDelegate delegate) {
      this.timestamp = timestamp;
      this.delegate = delegate;
    }

    @Override
    public void run() {
      if (!isActive()) {
        delegate.onGuidsSinceFailed(new InactiveSessionException(null));
        return;
      }

      Cursor cur;
      try {
        cur = dbHelper.getGUIDsSince(timestamp);
      } catch (NullCursorException e) {
        delegate.onGuidsSinceFailed(e);
        return;
      } catch (Exception e) {
        delegate.onGuidsSinceFailed(e);
        return;
      }

      ArrayList<String> guids;
      try {
        if (!cur.moveToFirst()) {
          delegate.onGuidsSinceSucceeded(new String[] {});
          return;
        }
        guids = new ArrayList<String>();
        while (!cur.isAfterLast()) {
          guids.add(RepoUtils.getStringFromCursor(cur, "guid"));
          cur.moveToNext();
        }
      } finally {
        Log.d(LOG_TAG, "Closing cursor after guidsSince.");
        cur.close();
      }

      String guidsArray[] = new String[guids.size()];
      guids.toArray(guidsArray);
      delegate.onGuidsSinceSucceeded(guidsArray);
    }
  }

  @Override
  public void fetch(String[] guids,
                    RepositorySessionFetchRecordsDelegate delegate) {
    FetchRunnable command = new FetchRunnable(guids, now(), delegate);
    delegateQueue.execute(command);
  }

  abstract class FetchingRunnable implements Runnable {
    protected RepositorySessionFetchRecordsDelegate delegate;

    public FetchingRunnable(RepositorySessionFetchRecordsDelegate delegate) {
      this.delegate = delegate;
    }

    protected void fetchFromCursor(Cursor cursor, long end) {
      Log.d(LOG_TAG, "Fetch from cursor:");
      try {
        try {
          if (!cursor.moveToFirst()) {
            delegate.onFetchCompleted(end);
            return;
          }
          while (!cursor.isAfterLast()) {
            Log.d(LOG_TAG, "... one more record.");
            Record r = transformRecord(recordFromMirrorCursor(cursor));
            if (r != null) {
              delegate.onFetchedRecord(r);
            }
            cursor.moveToNext();
          }
          delegate.onFetchCompleted(end);
        } catch (NoGuidForIdException e) {
          Log.w(LOG_TAG, "No GUID for ID.", e);
          delegate.onFetchFailed(e, null);
        } catch (Exception e) {
          Log.w(LOG_TAG, "Exception in fetchFromCursor.", e);
          delegate.onFetchFailed(e, null);
          return;
        }
      } finally {
        Log.d(LOG_TAG, "Closing cursor after fetch.");
        cursor.close();
      }
    }
  }

  class FetchRunnable extends FetchingRunnable {
    private String[] guids;
    private long     end;

    public FetchRunnable(String[] guids,
                         long end,
                         RepositorySessionFetchRecordsDelegate delegate) {
      super(delegate);
      this.guids = guids;
      this.end   = end;
    }

    @Override
    public void run() {
      if (!isActive()) {
        delegate.onFetchFailed(new InactiveSessionException(null), null);
        return;
      }

      if (guids == null || guids.length < 1) {
        Log.e(LOG_TAG, "No guids sent to fetch");
        delegate.onFetchFailed(new InvalidRequestException(null), null);
        return;
      }

      try {
        Cursor cursor = dbHelper.fetch(guids);
        this.fetchFromCursor(cursor, end);
      } catch (NullCursorException e) {
        delegate.onFetchFailed(e, null);
      }
    }
  }

  @Override
  public void fetchSince(long timestamp,
                         RepositorySessionFetchRecordsDelegate delegate) {
    Log.i(LOG_TAG, "Running fetchSince(" + timestamp + ").");
    FetchSinceRunnable command = new FetchSinceRunnable(timestamp, now(), delegate);
    delegateQueue.execute(command);
  }

  class FetchSinceRunnable extends FetchingRunnable {
    private long since;
    private long end;

    public FetchSinceRunnable(long since,
                              long end,
                              RepositorySessionFetchRecordsDelegate delegate) {
      super(delegate);
      this.since = since;
      this.end   = end;
    }

    @Override
    public void run() {
      if (!isActive()) {
        delegate.onFetchFailed(new InactiveSessionException(null), null);
        return;
      }

      try {
        Cursor cursor = dbHelper.fetchSince(since);
        this.fetchFromCursor(cursor, end);
      } catch (NullCursorException e) {
        delegate.onFetchFailed(e, null);
        return;
      }
    }
  }

  @Override
  public void fetchAll(RepositorySessionFetchRecordsDelegate delegate) {
    this.fetchSince(0, delegate);
  }

  @Override
  public void store(final Record record) throws NoStoreDelegateException {
    if (delegate == null) {
      throw new NoStoreDelegateException();
    }
    if (record == null) {
      Log.e(LOG_TAG, "Record sent to store was null");
      throw new IllegalArgumentException("Null record passed to AndroidBrowserRepositorySession.store().");
    }

    
    
    Runnable command = new Runnable() {

      @Override
      public void run() {
        if (!isActive()) {
          delegate.onRecordStoreFailed(new InactiveSessionException(null));
          return;
        }

        
        
        
        if (!checkRecordType(record)) {
          Log.d(LOG_TAG, "Ignoring record " + record.guid + " due to unknown record type.");

          
          
          return;
        }

        
        
        
        Record existingRecord;
        try {
          existingRecord = findExistingRecord(record);

          
          if (existingRecord == null && !record.deleted) {
            record.androidID = insert(record);
          } else if (existingRecord != null) {

            dbHelper.delete(existingRecord);
            
            
            if (!record.deleted || (record.deleted && existingRecord.lastModified > record.lastModified)) {
              
              Record store = reconcileRecords(existingRecord, record);
              record.androidID = insert(store);
            }
          }
        } catch (MultipleRecordsForGuidException e) {
          Log.e(LOG_TAG, "Multiple records returned for given guid: " + record.guid);
          delegate.onRecordStoreFailed(e);
          return;
        } catch (NoGuidForIdException e) {
          Log.e(LOG_TAG, "Store failed for " + record.guid, e);
          delegate.onRecordStoreFailed(e);
          return;
        } catch (NullCursorException e) {
          Log.e(LOG_TAG, "Store failed for " + record.guid, e);
          delegate.onRecordStoreFailed(e);
          return;
        } catch (Exception e) {
          Log.e(LOG_TAG, "Store failed for " + record.guid, e);
          delegate.onRecordStoreFailed(e);
          return;
        }

        
        delegate.onRecordStoreSucceeded(record);
      }
    };
    storeWorkQueue.execute(command);
  }
  
  protected long insert(Record record) throws NoGuidForIdException, NullCursorException, ParentNotFoundException {
    putRecordToGuidMap(buildRecordString(record), record.guid);
    return RepoUtils.getAndroidIdFromUri(dbHelper.insert(record));
  }

  protected Record recordForGUID(String guid) throws
                                             NoGuidForIdException,
                                             NullCursorException,
                                             ParentNotFoundException,
                                             MultipleRecordsForGuidException {
    Cursor cursor = dbHelper.fetch(new String[] { guid });
    try {
      if (!cursor.moveToFirst()) {
        return null;
      }

      Record r = recordFromMirrorCursor(cursor);

      cursor.moveToNext();
      if (cursor.isAfterLast()) {
        
        return r; 
      }

      
      throw (new MultipleRecordsForGuidException(null));
    } finally {
      cursor.close();
    }
  }

  
  protected Record findExistingRecord(Record record) throws MultipleRecordsForGuidException,
    NoGuidForIdException, NullCursorException, ParentNotFoundException {

    Log.d(LOG_TAG, "Finding existing record for GUID " + record.guid);
    Record r = recordForGUID(record.guid);

    
    if (r != null) {
      Log.d(LOG_TAG, "Found one by GUID.");
      return r;
    }

    
    
    String recordString = buildRecordString(record);
    Log.d(LOG_TAG, "Searching with record string " + recordString);
    String guid = getRecordToGuidMap().get(recordString);
    if (guid != null) {
      Log.d(LOG_TAG, "Found one. Returning computed record.");
      return recordForGUID(guid);
    }
    Log.d(LOG_TAG, "findExistingRecord failed to find one for " + record.guid);
    return null;
  }

  public HashMap<String, String> getRecordToGuidMap() throws NoGuidForIdException, NullCursorException, ParentNotFoundException {
    if (recordToGuid == null) {
      createRecordToGuidMap();
    }
    return recordToGuid;
  }

  private void createRecordToGuidMap() throws NoGuidForIdException, NullCursorException, ParentNotFoundException {
    recordToGuid = new HashMap<String, String>();
    Cursor cur = dbHelper.fetchAll();
    try {
      if (!cur.moveToFirst()) {
        return;
      }
      while (!cur.isAfterLast()) {
        Record record = recordFromMirrorCursor(cur);
        if (record != null) {
          recordToGuid.put(buildRecordString(record), record.guid);
        }
        cur.moveToNext();
      }
    } finally {
      cur.close();
    }
  }

  public void putRecordToGuidMap(String guid, String recordString) throws NoGuidForIdException, NullCursorException, ParentNotFoundException {
    if (recordToGuid == null) {
      createRecordToGuidMap();
    }
    recordToGuid.put(guid, recordString);
  }

  protected Record reconcileRecords(Record local, Record remote) {
    Log.i(LOG_TAG, "Reconciling " + local.guid + " against " + remote.guid);

    
    
    Record newer;
    if (local.lastModified > remote.lastModified) {
      newer = local;
    } else {
      newer = remote;
    }

    if (newer.guid != remote.guid) {
      newer.guid = remote.guid;
    }
    newer.androidID = local.androidID;

    return newer;
  }

  
  @Override
  public void wipe(RepositorySessionWipeDelegate delegate) {
    Runnable command = new WipeRunnable(delegate);
    storeWorkQueue.execute(command);
  }

  class WipeRunnable implements Runnable {
    private RepositorySessionWipeDelegate delegate;

    public WipeRunnable(RepositorySessionWipeDelegate delegate) {
      this.delegate = delegate;
    }

    public void run() {
      if (!isActive()) {
        delegate.onWipeFailed(new InactiveSessionException(null));
        return;
      }
      dbHelper.wipe();
      delegate.onWipeSucceeded();
    }
  }
}

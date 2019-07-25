





































package org.mozilla.gecko.sync.repositories.android;

import java.util.ArrayList;
import java.util.HashMap;

import org.mozilla.gecko.sync.Logger;
import org.mozilla.gecko.sync.repositories.InactiveSessionException;
import org.mozilla.gecko.sync.repositories.InvalidRequestException;
import org.mozilla.gecko.sync.repositories.InvalidSessionTransitionException;
import org.mozilla.gecko.sync.repositories.MultipleRecordsForGuidException;
import org.mozilla.gecko.sync.repositories.NoGuidForIdException;
import org.mozilla.gecko.sync.repositories.NoStoreDelegateException;
import org.mozilla.gecko.sync.repositories.NullCursorException;
import org.mozilla.gecko.sync.repositories.ParentNotFoundException;
import org.mozilla.gecko.sync.repositories.ProfileDatabaseException;
import org.mozilla.gecko.sync.repositories.RecordFilter;
import org.mozilla.gecko.sync.repositories.Repository;
import org.mozilla.gecko.sync.repositories.StoreTrackingRepositorySession;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionBeginDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionFetchRecordsDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionGuidsSinceDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionWipeDelegate;
import org.mozilla.gecko.sync.repositories.domain.Record;

import android.database.Cursor;
import android.net.Uri;























public abstract class AndroidBrowserRepositorySession extends StoreTrackingRepositorySession {

  protected AndroidBrowserRepositoryDataAccessor dbHelper;
  public static final String LOG_TAG = "BrowserRepoSession";
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
      Logger.error(LOG_TAG, "ProfileDatabaseException from begin. Fennec must be launched once until this error is fixed");
      deferredDelegate.onBeginFailed(e);
      return;
    } catch (NullCursorException e) {
      deferredDelegate.onBeginFailed(e);
      return;
    } catch (Exception e) {
      deferredDelegate.onBeginFailed(e);
      return;
    }
    storeTracker = createStoreTracker();
    deferredDelegate.onBeginSucceeded(this);
  }

  protected abstract String buildRecordString(Record record);

  protected void checkDatabase() throws ProfileDatabaseException, NullCursorException {
    Logger.info(LOG_TAG, "BEGIN: checking database.");
    try {
      dbHelper.fetch(new String[] { "none" }).close();
      Logger.info(LOG_TAG, "END: checking database.");
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
        Logger.debug(LOG_TAG, "Closing cursor after guidsSince.");
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
    FetchRunnable command = new FetchRunnable(guids, now(), null, delegate);
    delegateQueue.execute(command);
  }

  abstract class FetchingRunnable implements Runnable {
    protected RepositorySessionFetchRecordsDelegate delegate;

    public FetchingRunnable(RepositorySessionFetchRecordsDelegate delegate) {
      this.delegate = delegate;
    }

    protected void fetchFromCursor(Cursor cursor, RecordFilter filter, long end) {
      Logger.debug(LOG_TAG, "Fetch from cursor:");
      try {
        try {
          if (!cursor.moveToFirst()) {
            delegate.onFetchCompleted(end);
            return;
          }
          while (!cursor.isAfterLast()) {
            Record r = recordFromMirrorCursor(cursor);
            if (r != null) {
              if (filter == null || !filter.excludeRecord(r)) {
                Logger.trace(LOG_TAG, "Processing record " + r.guid);
                delegate.onFetchedRecord(transformRecord(r));
              } else {
                Logger.debug(LOG_TAG, "Skipping filtered record " + r.guid);
              }
            }
            cursor.moveToNext();
          }
          delegate.onFetchCompleted(end);
        } catch (NoGuidForIdException e) {
          Logger.warn(LOG_TAG, "No GUID for ID.", e);
          delegate.onFetchFailed(e, null);
        } catch (Exception e) {
          Logger.warn(LOG_TAG, "Exception in fetchFromCursor.", e);
          delegate.onFetchFailed(e, null);
          return;
        }
      } finally {
        Logger.trace(LOG_TAG, "Closing cursor after fetch.");
        cursor.close();
      }
    }
  }

  class FetchRunnable extends FetchingRunnable {
    private String[] guids;
    private long     end;
    private RecordFilter filter;

    public FetchRunnable(String[] guids,
                         long end,
                         RecordFilter filter,
                         RepositorySessionFetchRecordsDelegate delegate) {
      super(delegate);
      this.guids  = guids;
      this.end    = end;
      this.filter = filter;
    }

    @Override
    public void run() {
      if (!isActive()) {
        delegate.onFetchFailed(new InactiveSessionException(null), null);
        return;
      }

      if (guids == null || guids.length < 1) {
        Logger.error(LOG_TAG, "No guids sent to fetch");
        delegate.onFetchFailed(new InvalidRequestException(null), null);
        return;
      }

      try {
        Cursor cursor = dbHelper.fetch(guids);
        this.fetchFromCursor(cursor, filter, end);
      } catch (NullCursorException e) {
        delegate.onFetchFailed(e, null);
      }
    }
  }

  @Override
  public void fetchSince(long timestamp,
                         RepositorySessionFetchRecordsDelegate delegate) {
    if (this.storeTracker == null) {
      throw new IllegalStateException("Store tracker not yet initialized!");
    }

    Logger.info(LOG_TAG, "Running fetchSince(" + timestamp + ").");
    FetchSinceRunnable command = new FetchSinceRunnable(timestamp, now(), this.storeTracker.getFilter(), delegate);
    delegateQueue.execute(command);
  }

  class FetchSinceRunnable extends FetchingRunnable {
    private long since;
    private long end;
    private RecordFilter filter;

    public FetchSinceRunnable(long since,
                              long end,
                              RecordFilter filter,
                              RepositorySessionFetchRecordsDelegate delegate) {
      super(delegate);
      this.since  = since;
      this.end    = end;
      this.filter = filter;
    }

    @Override
    public void run() {
      if (!isActive()) {
        delegate.onFetchFailed(new InactiveSessionException(null), null);
        return;
      }

      try {
        Cursor cursor = dbHelper.fetchSince(since);
        this.fetchFromCursor(cursor, filter, end);
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
      Logger.error(LOG_TAG, "Record sent to store was null");
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
          Logger.debug(LOG_TAG, "Ignoring record " + record.guid + " due to unknown record type.");

          
          
          return;
        }


        
        
        
        
        
        
        long lastLocalRetrieval  = 0;      
        long lastRemoteRetrieval = 0;      
        boolean remotelyModified = record.lastModified > lastRemoteRetrieval;

        Record existingRecord;
        try {
          
          existingRecord = recordForGUID(record.guid);
          if (record.deleted) {
            if (existingRecord == null) {
              
              
              trace("Incoming record " + record.guid + " is deleted, and no local version. Bye!");
              return;
            }

            if (existingRecord.deleted) {
              trace("Local record already deleted. Bye!");
              return;
            }

            
            if (!remotelyModified) {
              trace("Ignoring deleted record from the past.");
              return;
            }

            boolean locallyModified = existingRecord.lastModified > lastLocalRetrieval;
            if (!locallyModified) {
              trace("Remote modified, local not. Deleting.");
              storeRecordDeletion(record);
              return;
            }

            trace("Both local and remote records have been modified.");
            if (record.lastModified > existingRecord.lastModified) {
              trace("Remote is newer, and deleted. Deleting local.");
              storeRecordDeletion(record);
              return;
            }

            trace("Remote is older, local is not deleted. Ignoring.");
            if (!locallyModified) {
              Logger.warn(LOG_TAG, "Inconsistency: old remote record is deleted, but local record not modified!");
              
            }
            return;
          }
          

          
          if (existingRecord == null) {
            trace("Looking up match for record " + record.guid);
            existingRecord = findExistingRecord(record);
          }

          if (existingRecord == null) {
            
            trace("No match. Inserting.");
            Record inserted = insert(record);
            trackRecord(inserted);
            delegate.onRecordStoreSucceeded(inserted);
            return;
          }

          
          trace("Incoming record " + record.guid + " dupes to local record " + existingRecord.guid);

          
          existingRecord = transformRecord(existingRecord);
          Record toStore = reconcileRecords(record, existingRecord, lastRemoteRetrieval, lastLocalRetrieval);

          if (toStore == null) {
            Logger.debug(LOG_TAG, "Reconciling returned null. Not inserting a record.");
            return;
          }

          
          Logger.debug(LOG_TAG, "Replacing " + existingRecord.guid + " with record " + toStore.guid);
          Record replaced = replace(toStore, existingRecord);

          
          
          Logger.debug(LOG_TAG, "Calling delegate callback with guid " + replaced.guid +
                                "(" + replaced.androidID + ")");
          delegate.onRecordStoreSucceeded(replaced);
          return;

        } catch (MultipleRecordsForGuidException e) {
          Logger.error(LOG_TAG, "Multiple records returned for given guid: " + record.guid);
          delegate.onRecordStoreFailed(e);
          return;
        } catch (NoGuidForIdException e) {
          Logger.error(LOG_TAG, "Store failed for " + record.guid, e);
          delegate.onRecordStoreFailed(e);
          return;
        } catch (NullCursorException e) {
          Logger.error(LOG_TAG, "Store failed for " + record.guid, e);
          delegate.onRecordStoreFailed(e);
          return;
        } catch (Exception e) {
          Logger.error(LOG_TAG, "Store failed for " + record.guid, e);
          delegate.onRecordStoreFailed(e);
          return;
        }
      }
    };
    storeWorkQueue.execute(command);
  }

  protected void storeRecordDeletion(final Record record) {
    
    
    dbHelper.delete(record);      
    delegate.onRecordStoreSucceeded(record);
  }

  protected Record insert(Record record) throws NoGuidForIdException, NullCursorException, ParentNotFoundException {
    Record toStore = prepareRecord(record);
    Uri recordURI = dbHelper.insert(toStore);
    long id = RepoUtils.getAndroidIdFromUri(recordURI);
    Logger.debug(LOG_TAG, "Inserted as " + id);

    toStore.androidID = id;
    updateBookkeeping(toStore);
    Logger.debug(LOG_TAG, "insert() returning record " + toStore.guid);
    return toStore;
  }

  protected Record replace(Record newRecord, Record existingRecord) throws NoGuidForIdException, NullCursorException, ParentNotFoundException {
    Record toStore = prepareRecord(newRecord);

    
    dbHelper.update(existingRecord.guid, toStore);
    updateBookkeeping(toStore);
    Logger.debug(LOG_TAG, "replace() returning record " + toStore.guid);
    return toStore;
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

    Logger.debug(LOG_TAG, "Finding existing record for incoming record with GUID " + record.guid);
    String recordString = buildRecordString(record);
    Logger.debug(LOG_TAG, "Searching with record string " + recordString);
    String guid = getRecordToGuidMap().get(recordString);
    if (guid != null) {
      Logger.debug(LOG_TAG, "Found one. Returning computed record.");
      return recordForGUID(guid);
    }
    Logger.debug(LOG_TAG, "findExistingRecord failed to find one for " + record.guid);
    return null;
  }

  public HashMap<String, String> getRecordToGuidMap() throws NoGuidForIdException, NullCursorException, ParentNotFoundException {
    if (recordToGuid == null) {
      createRecordToGuidMap();
    }
    return recordToGuid;
  }

  private void createRecordToGuidMap() throws NoGuidForIdException, NullCursorException, ParentNotFoundException {
    Logger.info(LOG_TAG, "BEGIN: creating record -> GUID map.");
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
    Logger.info(LOG_TAG, "END: creating record -> GUID map.");
  }

  public void putRecordToGuidMap(String recordString, String guid) throws NoGuidForIdException, NullCursorException, ParentNotFoundException {
    if (recordToGuid == null) {
      createRecordToGuidMap();
    }
    recordToGuid.put(recordString, guid);
  }

  protected abstract Record prepareRecord(Record record);
  protected void updateBookkeeping(Record record) throws NoGuidForIdException,
                                                 NullCursorException,
                                                 ParentNotFoundException {
    putRecordToGuidMap(buildRecordString(record), record.guid);
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

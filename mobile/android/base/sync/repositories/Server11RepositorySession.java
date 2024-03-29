



package org.mozilla.gecko.sync.repositories;

import java.io.IOException;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.atomic.AtomicLong;

import org.json.simple.JSONArray;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.CryptoRecord;
import org.mozilla.gecko.sync.DelayedWorkTracker;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.HTTPFailureException;
import org.mozilla.gecko.sync.Server11PreviousPostFailedException;
import org.mozilla.gecko.sync.Server11RecordPostFailedException;
import org.mozilla.gecko.sync.UnexpectedJSONException;
import org.mozilla.gecko.sync.crypto.KeyBundle;
import org.mozilla.gecko.sync.net.AuthHeaderProvider;
import org.mozilla.gecko.sync.net.SyncStorageCollectionRequest;
import org.mozilla.gecko.sync.net.SyncStorageRequest;
import org.mozilla.gecko.sync.net.SyncStorageRequestDelegate;
import org.mozilla.gecko.sync.net.SyncStorageResponse;
import org.mozilla.gecko.sync.net.WBOCollectionRequestDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionBeginDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionFetchRecordsDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionGuidsSinceDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionStoreDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionWipeDelegate;
import org.mozilla.gecko.sync.repositories.domain.Record;

import ch.boye.httpclientandroidlib.entity.ContentProducer;
import ch.boye.httpclientandroidlib.entity.EntityTemplate;

public class Server11RepositorySession extends RepositorySession {
  private static byte[] recordsStart;
  private static byte[] recordSeparator;
  private static byte[] recordsEnd;

  static {
    try {
      recordsStart    = "[\n".getBytes("UTF-8");
      recordSeparator = ",\n".getBytes("UTF-8");
      recordsEnd      = "\n]\n".getBytes("UTF-8");
    } catch (UnsupportedEncodingException e) {
      
    }
  }

  public static final String LOG_TAG = "Server11Session";

  private static final int UPLOAD_BYTE_THRESHOLD = 1024 * 1024;    
  private static final int UPLOAD_ITEM_THRESHOLD = 50;
  private static final int PER_RECORD_OVERHEAD   = 2;              
  
  private static final int PER_BATCH_OVERHEAD    = 5 - PER_RECORD_OVERHEAD;

  













  public static long getNormalizedTimestamp(SyncStorageResponse response) {
    long normalizedTimestamp = -1;
    try {
      normalizedTimestamp = response.normalizedWeaveTimestamp();
    } catch (NumberFormatException e) {
      Logger.warn(LOG_TAG, "Malformed X-Weave-Timestamp header received.", e);
    }
    if (-1 == normalizedTimestamp) {
      Logger.warn(LOG_TAG, "Computing stand-in timestamp from local clock. Clock drift could cause records to be skipped.");
      normalizedTimestamp = System.currentTimeMillis();
    }
    return normalizedTimestamp;
  }

  


  private final Set<SyncStorageCollectionRequest> pending = Collections.synchronizedSet(new HashSet<SyncStorageCollectionRequest>());

  @Override
  public void abort() {
    super.abort();
    for (SyncStorageCollectionRequest request : pending) {
      request.abort();
    }
    pending.clear();
  }

  






  public class RequestFetchDelegateAdapter extends WBOCollectionRequestDelegate {
    RepositorySessionFetchRecordsDelegate delegate;
    private final DelayedWorkTracker workTracker = new DelayedWorkTracker();

    
    private SyncStorageCollectionRequest request;

    public void setRequest(SyncStorageCollectionRequest request) {
      this.request = request;
    }
    private void removeRequestFromPending() {
      if (this.request == null) {
        return;
      }
      pending.remove(this.request);
      this.request = null;
    }

    public RequestFetchDelegateAdapter(RepositorySessionFetchRecordsDelegate delegate) {
      this.delegate = delegate;
    }

    @Override
    public AuthHeaderProvider getAuthHeaderProvider() {
      return serverRepository.getAuthHeaderProvider();
    }

    @Override
    public String ifUnmodifiedSince() {
      return null;
    }

    @Override
    public void handleRequestSuccess(SyncStorageResponse response) {
      Logger.debug(LOG_TAG, "Fetch done.");
      removeRequestFromPending();

      final long normalizedTimestamp = getNormalizedTimestamp(response);
      Logger.debug(LOG_TAG, "Fetch completed. Timestamp is " + normalizedTimestamp);

      
      workTracker.delayWorkItem(new Runnable() {
        @Override
        public void run() {
          Logger.debug(LOG_TAG, "Delayed onFetchCompleted running.");
          
          delegate.onFetchCompleted(normalizedTimestamp);
        }
      });
    }

    @Override
    public void handleRequestFailure(SyncStorageResponse response) {
      
      this.handleRequestError(new HTTPFailureException(response));
    }

    @Override
    public void handleRequestError(final Exception ex) {
      removeRequestFromPending();
      Logger.warn(LOG_TAG, "Got request error.", ex);
      
      workTracker.delayWorkItem(new Runnable() {
        @Override
        public void run() {
          Logger.debug(LOG_TAG, "Running onFetchFailed.");
          delegate.onFetchFailed(ex, null);
        }
      });
    }

    @Override
    public void handleWBO(CryptoRecord record) {
      workTracker.incrementOutstanding();
      try {
        delegate.onFetchedRecord(record);
      } catch (Exception ex) {
        Logger.warn(LOG_TAG, "Got exception calling onFetchedRecord with WBO.", ex);
        
        throw new RuntimeException(ex);
      } finally {
        workTracker.decrementOutstanding();
      }
    }

    
    @Override
    public KeyBundle keyBundle() {
      return null;
    }
  }


  Server11Repository serverRepository;
  AtomicLong uploadTimestamp = new AtomicLong(0);

  private void bumpUploadTimestamp(long ts) {
    while (true) {
      long existing = uploadTimestamp.get();
      if (existing > ts) {
        return;
      }
      if (uploadTimestamp.compareAndSet(existing, ts)) {
        return;
      }
    }
  }

  public Server11RepositorySession(Repository repository) {
    super(repository);
    serverRepository = (Server11Repository) repository;
  }

  private String flattenIDs(String[] guids) {
    
    
    if (guids.length == 0) {
      return "";
    }
    if (guids.length == 1) {
      return guids[0];
    }
    StringBuilder b = new StringBuilder();
    for (String guid : guids) {
      b.append(guid);
      b.append(",");
    }
    return b.substring(0, b.length() - 1);
  }

  @Override
  public void guidsSince(long timestamp,
                         RepositorySessionGuidsSinceDelegate delegate) {
    

  }

  protected void fetchWithParameters(long newer,
                                     long limit,
                                     boolean full,
                                     String sort,
                                     String ids,
                                     RequestFetchDelegateAdapter delegate)
                                         throws URISyntaxException {

    URI collectionURI = serverRepository.collectionURI(full, newer, limit, sort, ids);
    SyncStorageCollectionRequest request = new SyncStorageCollectionRequest(collectionURI);
    request.delegate = delegate;

    
    delegate.setRequest(request);
    pending.add(request);
    request.get();
  }

  public void fetchSince(long timestamp, long limit, String sort, RepositorySessionFetchRecordsDelegate delegate) {
    try {
      this.fetchWithParameters(timestamp, limit, true, sort, null, new RequestFetchDelegateAdapter(delegate));
    } catch (URISyntaxException e) {
      delegate.onFetchFailed(e, null);
    }
  }

  @Override
  public void fetchSince(long timestamp,
                         RepositorySessionFetchRecordsDelegate delegate) {
    try {
      long limit = serverRepository.getDefaultFetchLimit();
      String sort = serverRepository.getDefaultSort();
      this.fetchWithParameters(timestamp, limit, true, sort, null, new RequestFetchDelegateAdapter(delegate));
    } catch (URISyntaxException e) {
      delegate.onFetchFailed(e, null);
    }
  }

  @Override
  public void fetchAll(RepositorySessionFetchRecordsDelegate delegate) {
    this.fetchSince(-1, delegate);
  }

  @Override
  public void fetch(String[] guids,
                    RepositorySessionFetchRecordsDelegate delegate) {
    
    try {
      String ids = flattenIDs(guids);
      this.fetchWithParameters(-1, -1, true, "index", ids, new RequestFetchDelegateAdapter(delegate));
    } catch (URISyntaxException e) {
      delegate.onFetchFailed(e, null);
    }
  }

  @Override
  public void wipe(RepositorySessionWipeDelegate delegate) {
    if (!isActive()) {
      delegate.onWipeFailed(new InactiveSessionException(null));
      return;
    }
    
  }

  protected Object recordsBufferMonitor = new Object();

  







  protected ArrayList<byte[]> recordsBuffer = new ArrayList<byte[]>();

  






  protected ArrayList<String> recordGuidsBuffer = new ArrayList<String>();
  protected int byteCount = PER_BATCH_OVERHEAD;

  @Override
  public void store(Record record) throws NoStoreDelegateException {
    if (delegate == null) {
      throw new NoStoreDelegateException();
    }
    this.enqueue(record);
  }

  





  protected void enqueue(Record record) {
    
    byte[] json = record.toJSONBytes();
    int delta   = json.length;
    synchronized (recordsBufferMonitor) {
      if ((delta + byteCount     > UPLOAD_BYTE_THRESHOLD) ||
          (recordsBuffer.size() >= UPLOAD_ITEM_THRESHOLD)) {

        
        flush();
      }
      recordsBuffer.add(json);
      recordGuidsBuffer.add(record.guid);
      byteCount += PER_RECORD_OVERHEAD + delta;
    }
  }

  
  
  protected void flush() {
    if (recordsBuffer.size() > 0) {
      final ArrayList<byte[]> outgoing = recordsBuffer;
      final ArrayList<String> outgoingGuids = recordGuidsBuffer;
      RepositorySessionStoreDelegate uploadDelegate = this.delegate;
      storeWorkQueue.execute(new RecordUploadRunnable(uploadDelegate, outgoing, outgoingGuids, byteCount));

      recordsBuffer = new ArrayList<byte[]>();
      recordGuidsBuffer = new ArrayList<String>();
      byteCount = PER_BATCH_OVERHEAD;
    }
  }

  @Override
  public void storeDone() {
    Logger.debug(LOG_TAG, "storeDone().");
    synchronized (recordsBufferMonitor) {
      flush();
      
      final Runnable r = new Runnable() {
        @Override
        public void run() {
          synchronized (recordsBufferMonitor) {
            final long end = uploadTimestamp.get();
            Logger.debug(LOG_TAG, "Calling storeDone with " + end);
            storeDone(end);
          }
        }
      };
      storeWorkQueue.execute(r);
    }
  }

  






  protected volatile boolean recordUploadFailed;

  @Override
  public void begin(RepositorySessionBeginDelegate delegate) throws InvalidSessionTransitionException {
    recordUploadFailed = false;
    super.begin(delegate);
  }

  






  protected class RecordUploadRunnable implements Runnable, SyncStorageRequestDelegate {

    public final String LOG_TAG = "RecordUploadRunnable";
    private final ArrayList<byte[]> outgoing;
    private ArrayList<String> outgoingGuids;
    private final long byteCount;

    public RecordUploadRunnable(RepositorySessionStoreDelegate storeDelegate,
                                ArrayList<byte[]> outgoing,
                                ArrayList<String> outgoingGuids,
                                long byteCount) {
      Logger.debug(LOG_TAG, "Preparing record upload for " +
                  outgoing.size() + " records (" +
                  byteCount + " bytes).");
      this.outgoing = outgoing;
      this.outgoingGuids = outgoingGuids;
      this.byteCount = byteCount;
    }

    @Override
    public AuthHeaderProvider getAuthHeaderProvider() {
      return serverRepository.getAuthHeaderProvider();
    }

    @Override
    public String ifUnmodifiedSince() {
      return null;
    }

    @Override
    public void handleRequestSuccess(SyncStorageResponse response) {
      Logger.trace(LOG_TAG, "POST of " + outgoing.size() + " records done.");

      ExtendedJSONObject body;
      try {
        body = response.jsonObjectBody(); 
      } catch (Exception e) {
        Logger.error(LOG_TAG, "Got exception parsing POST success body.", e);
        this.handleRequestError(e);
        return;
      }

      
      if (body.containsKey("modified")) {
        Long modified = body.getTimestamp("modified");
        if (modified != null) {
          Logger.trace(LOG_TAG, "POST request success. Modified timestamp: " + modified);
        } else {
          Logger.warn(LOG_TAG, "POST success body contains malformed 'modified': " + body.toJSONString());
        }
      } else {
        Logger.warn(LOG_TAG, "POST success body does not contain key 'modified': " + body.toJSONString());
      }

      try {
        JSONArray          success = body.getArray("success");
        if ((success != null) &&
            (success.size() > 0)) {
          Logger.trace(LOG_TAG, "Successful records: " + success.toString());
          for (Object o : success) {
            try {
              delegate.onRecordStoreSucceeded((String) o);
            } catch (ClassCastException e) {
              Logger.error(LOG_TAG, "Got exception parsing POST success guid.", e);
              
            }
          }

          long normalizedTimestamp = getNormalizedTimestamp(response);
          Logger.trace(LOG_TAG, "Passing back upload X-Weave-Timestamp: " + normalizedTimestamp);
          bumpUploadTimestamp(normalizedTimestamp);
        }
        success = null; 

        ExtendedJSONObject failed  = body.getObject("failed");
        if ((failed != null) &&
            (failed.object.size() > 0)) {
          Logger.debug(LOG_TAG, "Failed records: " + failed.object.toString());
          Exception ex = new Server11RecordPostFailedException();
          for (String guid : failed.keySet()) {
            delegate.onRecordStoreFailed(ex, guid);
          }
        }
        failed = null; 
      } catch (UnexpectedJSONException e) {
        Logger.error(LOG_TAG, "Got exception processing success/failed in POST success body.", e);
        
        return;
      }
      Logger.debug(LOG_TAG, "POST of " + outgoing.size() + " records handled.");
    }

    @Override
    public void handleRequestFailure(SyncStorageResponse response) {
      
      this.handleRequestError(new HTTPFailureException(response));
    }

    @Override
    public void handleRequestError(final Exception ex) {
      Logger.warn(LOG_TAG, "Got request error.", ex);

      recordUploadFailed = true;
      ArrayList<String> failedOutgoingGuids = outgoingGuids;
      outgoingGuids = null; 
      for (String guid : failedOutgoingGuids) {
        delegate.onRecordStoreFailed(ex, guid);
      }
      return;
    }

    public class ByteArraysContentProducer implements ContentProducer {

      ArrayList<byte[]> outgoing;
      public ByteArraysContentProducer(ArrayList<byte[]> arrays) {
        outgoing = arrays;
      }

      @Override
      public void writeTo(OutputStream outstream) throws IOException {
        int count = outgoing.size();
        outstream.write(recordsStart);
        outstream.write(outgoing.get(0));
        for (int i = 1; i < count; ++i) {
          outstream.write(recordSeparator);
          outstream.write(outgoing.get(i));
        }
        outstream.write(recordsEnd);
      }
    }

    public class ByteArraysEntity extends EntityTemplate {
      private final long count;
      public ByteArraysEntity(ArrayList<byte[]> arrays, long totalBytes) {
        super(new ByteArraysContentProducer(arrays));
        this.count = totalBytes;
        this.setContentType("application/json");
        
      }

      @Override
      public long getContentLength() {
        return count;
      }

      @Override
      public boolean isRepeatable() {
        return true;
      }
    }

    public ByteArraysEntity getBodyEntity() {
      ByteArraysEntity body = new ByteArraysEntity(outgoing, byteCount);
      return body;
    }

    @Override
    public void run() {
      if (recordUploadFailed) {
        Logger.info(LOG_TAG, "Previous record upload failed.  Failing all records and not retrying.");
        Exception ex = new Server11PreviousPostFailedException();
        for (String guid : outgoingGuids) {
          delegate.onRecordStoreFailed(ex, guid);
        }
        return;
      }

      if (outgoing == null ||
          outgoing.size() == 0) {
        Logger.debug(LOG_TAG, "No items: RecordUploadRunnable returning immediately.");
        return;
      }

      URI u = serverRepository.collectionURI();
      SyncStorageRequest request = new SyncStorageRequest(u);

      request.delegate = this;

      
      
      
      ByteArraysEntity body = getBodyEntity();
      request.post(body);
    }
  }

  @Override
  public boolean dataAvailable() {
    return serverRepository.updateNeeded(getLastSyncTimestamp());
  }
}

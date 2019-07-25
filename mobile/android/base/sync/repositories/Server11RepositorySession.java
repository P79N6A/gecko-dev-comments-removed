




































package org.mozilla.gecko.sync.repositories;

import java.io.IOException;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.Date;

import org.json.simple.JSONArray;
import org.mozilla.gecko.sync.CryptoRecord;
import org.mozilla.gecko.sync.DelayedWorkTracker;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.HTTPFailureException;
import org.mozilla.gecko.sync.UnexpectedJSONException;
import org.mozilla.gecko.sync.crypto.KeyBundle;
import org.mozilla.gecko.sync.net.SyncStorageCollectionRequest;
import org.mozilla.gecko.sync.net.SyncStorageRequest;
import org.mozilla.gecko.sync.net.SyncStorageRequestDelegate;
import org.mozilla.gecko.sync.net.SyncStorageResponse;
import org.mozilla.gecko.sync.net.WBOCollectionRequestDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionFetchRecordsDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionGuidsSinceDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionStoreDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionWipeDelegate;
import org.mozilla.gecko.sync.repositories.domain.Record;

import android.util.Log;
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

  public static final String LOG_TAG = "Server11RepositorySession";

  private static final int UPLOAD_BYTE_THRESHOLD = 1024 * 1024;    
  private static final int UPLOAD_ITEM_THRESHOLD = 50;
  private static final int PER_RECORD_OVERHEAD   = 2;              
  
  private static final int PER_BATCH_OVERHEAD    = 5 - PER_RECORD_OVERHEAD;

  






  public class RequestFetchDelegateAdapter extends WBOCollectionRequestDelegate {
    RepositorySessionFetchRecordsDelegate delegate;
    private DelayedWorkTracker workTracker = new DelayedWorkTracker();

    public RequestFetchDelegateAdapter(RepositorySessionFetchRecordsDelegate delegate) {
      this.delegate = delegate;
    }

    @Override
    public String credentials() {
      return serverRepository.credentialsSource.credentials();
    }

    @Override
    public String ifUnmodifiedSince() {
      return null;
    }

    @Override
    public void handleRequestSuccess(SyncStorageResponse response) {
      Log.i(LOG_TAG, "Fetch done.");

      long normalizedTimestamp = -1;
      try {
        normalizedTimestamp = response.normalizedWeaveTimestamp();
      } catch (NumberFormatException e) {
        Log.w(LOG_TAG, "Malformed X-Weave-Timestamp header received.", e);
      }
      if (-1 == normalizedTimestamp) {
        Log.w(LOG_TAG, "Computing stand-in timestamp from local clock. Clock drift could cause records to be skipped.");
        normalizedTimestamp = new Date().getTime();
      }

      Log.d(LOG_TAG, "Fetch completed. Timestamp is " + normalizedTimestamp);
      final long ts = normalizedTimestamp;

      
      workTracker.delayWorkItem(new Runnable() {
        @Override
        public void run() {
          Log.d(LOG_TAG, "Delayed onFetchCompleted running.");
          
          delegate.onFetchCompleted(ts);
        }
      });
    }

    @Override
    public void handleRequestFailure(SyncStorageResponse response) {
      
      this.handleRequestError(new HTTPFailureException(response));
    }

    @Override
    public void handleRequestError(final Exception ex) {
      Log.i(LOG_TAG, "Got request error.", ex);
      
      workTracker.delayWorkItem(new Runnable() {
        @Override
        public void run() {
          Log.i(LOG_TAG, "Running onFetchFailed.");
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
        Log.i(LOG_TAG, "Got exception calling onFetchedRecord with WBO.", ex);
        
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
                                     boolean full,
                                     String ids,
                                     SyncStorageRequestDelegate delegate) throws URISyntaxException {

    URI collectionURI = serverRepository.collectionURI(full, newer, ids);
    SyncStorageCollectionRequest request = new SyncStorageCollectionRequest(collectionURI);
    request.delegate = delegate;
    request.get();
  }

  @Override
  public void fetchSince(long timestamp,
                         RepositorySessionFetchRecordsDelegate delegate) {
    try {
      this.fetchWithParameters(timestamp, true, null, new RequestFetchDelegateAdapter(delegate));
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
      this.fetchWithParameters(-1, true, ids, new RequestFetchDelegateAdapter(delegate));
    } catch (URISyntaxException e) {
      delegate.onFetchFailed(e, null);
    }
  }

  @Override
  public void wipe(RepositorySessionWipeDelegate delegate) {
    
  }

  protected Object recordsBufferMonitor = new Object();
  protected ArrayList<byte[]> recordsBuffer = new ArrayList<byte[]>();
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
      byteCount += PER_RECORD_OVERHEAD + delta;
    }
  }

  
  
  protected void flush() {
    if (recordsBuffer.size() > 0) {
      final ArrayList<byte[]> outgoing = recordsBuffer;
      RepositorySessionStoreDelegate uploadDelegate = this.delegate;
      storeWorkQueue.execute(new RecordUploadRunnable(uploadDelegate, outgoing, byteCount));

      recordsBuffer = new ArrayList<byte[]>();
      byteCount = PER_BATCH_OVERHEAD;
    }
  }

  @Override
  public void storeDone() {
    synchronized (recordsBufferMonitor) {
      flush();
      super.storeDone();
    }
  }

  






  protected class RecordUploadRunnable implements Runnable, SyncStorageRequestDelegate {

    public final String LOG_TAG = "RecordUploadRunnable";
    private ArrayList<byte[]> outgoing;
    private long byteCount;

    public RecordUploadRunnable(RepositorySessionStoreDelegate storeDelegate,
                                ArrayList<byte[]> outgoing,
                                long byteCount) {
      Log.i(LOG_TAG, "Preparing RecordUploadRunnable for " +
                     outgoing.size() + " records (" +
                     byteCount + " bytes).");
      this.outgoing  = outgoing;
      this.byteCount = byteCount;
    }

    @Override
    public String credentials() {
      return serverRepository.credentialsSource.credentials();
    }

    @Override
    public String ifUnmodifiedSince() {
      return null;
    }

    @Override
    public void handleRequestSuccess(SyncStorageResponse response) {
      Log.i(LOG_TAG, "POST of " + outgoing.size() + " records done.");

      ExtendedJSONObject body;
      try {
        body = response.jsonObjectBody();
      } catch (Exception e) {
        Log.e(LOG_TAG, "Got exception parsing POST success body.", e);
        
        return;
      }
      long modified = body.getTimestamp("modified");
      Log.i(LOG_TAG, "POST request success. Modified timestamp: " + modified);

      try {
        JSONArray          success = body.getArray("success");
        ExtendedJSONObject failed  = body.getObject("failed");
        if ((success != null) &&
            (success.size() > 0)) {
          Log.d(LOG_TAG, "Successful records: " + success.toString());
          
        }
        if ((failed != null) &&
            (failed.object.size() > 0)) {
          Log.d(LOG_TAG, "Failed records: " + failed.object.toString());
          
        }
      } catch (UnexpectedJSONException e) {
        Log.e(LOG_TAG, "Got exception processing success/failed in POST success body.", e);
        
        return;
      }
    }

    @Override
    public void handleRequestFailure(SyncStorageResponse response) {
      
      
      this.handleRequestError(new HTTPFailureException(response));
    }

    @Override
    public void handleRequestError(final Exception ex) {
      Log.i(LOG_TAG, "Got request error: " + ex, ex);
      delegate.onRecordStoreFailed(ex);
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
      private long count;
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
      if (outgoing == null ||
          outgoing.size() == 0) {
        Log.i(LOG_TAG, "No items: RecordUploadRunnable returning immediately.");
        return;
      }

      URI u = serverRepository.collectionURI();
      SyncStorageRequest request = new SyncStorageRequest(u);

      request.delegate = this;

      
      
      
      ByteArraysEntity body = getBodyEntity();
      request.post(body);
    }
  }
}

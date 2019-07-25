




































package org.mozilla.gecko.sync.repositories;

import java.net.URI;
import java.net.URISyntaxException;
import java.util.Date;

import org.mozilla.gecko.sync.crypto.KeyBundle;
import org.mozilla.gecko.sync.CryptoRecord;
import org.mozilla.gecko.sync.DelayedWorkTracker;
import org.mozilla.gecko.sync.HTTPFailureException;
import org.mozilla.gecko.sync.net.SyncStorageCollectionRequest;
import org.mozilla.gecko.sync.net.SyncStorageRequestDelegate;
import org.mozilla.gecko.sync.net.SyncStorageResponse;
import org.mozilla.gecko.sync.net.WBOCollectionRequestDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionFetchRecordsDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionGuidsSinceDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionStoreDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionWipeDelegate;
import org.mozilla.gecko.sync.repositories.domain.Record;

import android.util.Log;

public class Server11RepositorySession extends RepositorySession {

  public static final String LOG_TAG = "Server11RepositorySession";

  






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

  private void fetchWithParameters(long newer,
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
  public void store(Record record, RepositorySessionStoreDelegate delegate) {
    
  }

  @Override
  public void wipe(RepositorySessionWipeDelegate delegate) {
    
  }
}

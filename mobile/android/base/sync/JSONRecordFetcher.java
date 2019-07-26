



package org.mozilla.gecko.sync;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.delegates.JSONRecordFetchDelegate;
import org.mozilla.gecko.sync.net.SyncStorageRecordRequest;
import org.mozilla.gecko.sync.net.SyncStorageRequestDelegate;
import org.mozilla.gecko.sync.net.SyncStorageResponse;





public class JSONRecordFetcher {
  private static final long DEFAULT_AWAIT_TIMEOUT_MSEC = 2 * 60 * 1000;   
  private static final String LOG_TAG = "JSONRecordFetcher";

  protected final String credentials;
  protected final String uri;
  protected JSONRecordFetchDelegate delegate;

  public JSONRecordFetcher(final String uri, final String credentials) {
    this.uri = uri;
    this.credentials = credentials;
  }

  protected String getURI() {
    return this.uri;
  }

  private class JSONFetchHandler implements SyncStorageRequestDelegate {

    
    public String credentials() {
      return credentials;
    }

    public String ifUnmodifiedSince() {
      return null;
    }

    public void handleRequestSuccess(SyncStorageResponse response) {
      if (response.wasSuccessful()) {
        try {
          delegate.handleSuccess(response.jsonObjectBody());
        } catch (Exception e) {
          handleRequestError(e);
        }
        return;
      }
      handleRequestFailure(response);
    }

    @Override
    public void handleRequestFailure(SyncStorageResponse response) {
      delegate.handleFailure(response);
    }

    @Override
    public void handleRequestError(Exception ex) {
      delegate.handleError(ex);
    }
  }

  public void fetch(final JSONRecordFetchDelegate delegate) {
    this.delegate = delegate;
    try {
      final SyncStorageRecordRequest r = new SyncStorageRecordRequest(this.getURI());
      r.delegate = new JSONFetchHandler();
      r.get();
    } catch (Exception e) {
      delegate.handleError(e);
    }
  }

  private class LatchedJSONRecordFetchDelegate implements JSONRecordFetchDelegate {
    public ExtendedJSONObject body = null;
    public Exception exception = null;
    private CountDownLatch latch;

    public LatchedJSONRecordFetchDelegate(CountDownLatch latch) {
      this.latch = latch;
    }

    @Override
    public void handleFailure(SyncStorageResponse response) {
      this.exception = new HTTPFailureException(response);
      latch.countDown();
    }

    @Override
    public void handleError(Exception e) {
      this.exception = e;
      latch.countDown();
    }

    @Override
    public void handleSuccess(ExtendedJSONObject body) {
      this.body = body;
      latch.countDown();
    }
  }

  



  public ExtendedJSONObject fetchBlocking() throws HTTPFailureException, Exception {
    CountDownLatch latch = new CountDownLatch(1);
    LatchedJSONRecordFetchDelegate delegate = new LatchedJSONRecordFetchDelegate(latch);
    this.delegate = delegate;
    this.fetch(delegate);

    
    
    
    if (!latch.await(DEFAULT_AWAIT_TIMEOUT_MSEC, TimeUnit.MILLISECONDS)) {
      Logger.warn(LOG_TAG, "Interrupted fetching info record.");
      throw new InterruptedException("info fetch timed out.");
    }

    if (delegate.body != null) {
      return delegate.body;
    }

    if (delegate.exception != null) {
      throw delegate.exception;
    }

    throw new Exception("Unknown error.");
  }
}
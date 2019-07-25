



package org.mozilla.gecko.sync.repositories;

import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionBeginDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionFinishDelegate;
import org.mozilla.gecko.sync.repositories.domain.Record;

import android.util.Log;

public abstract class StoreTrackingRepositorySession extends RepositorySession {
  private static final String LOG_TAG = "StoreTrackingRepositorySession";
  protected StoreTracker storeTracker;

  protected static StoreTracker createStoreTracker() {
    return new HashSetStoreTracker();
  }

  public StoreTrackingRepositorySession(Repository repository) {
    super(repository);
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
    
    storeTracker = createStoreTracker();
    deferredDelegate.onBeginSucceeded(this);
  }

  @Override
  protected synchronized void trackRecord(Record record) {
    if (this.storeTracker == null) {
      throw new IllegalStateException("Store tracker not yet initialized!");
    }

    Log.d(LOG_TAG, "Tracking record " + record.guid +
                   " (" + record.lastModified + ") to avoid re-upload.");
    
    this.storeTracker.trackRecordForExclusion(record.guid);
  }

  @Override
  public void abort(RepositorySessionFinishDelegate delegate) {
    this.storeTracker = null;
    super.abort(delegate);
  }

  @Override
  public void finish(RepositorySessionFinishDelegate delegate) {
    this.storeTracker = null;
    super.finish(delegate);
  }
}






































package org.mozilla.gecko.sync.repositories;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionBeginDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionFetchRecordsDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionFinishDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionGuidsSinceDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionStoreDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionWipeDelegate;
import org.mozilla.gecko.sync.repositories.domain.Record;

import android.util.Log;














public abstract class RepositorySession {

  public enum SessionStatus {
    UNSTARTED,
    ACTIVE,
    ABORTED,
    DONE
  }

  private static final String LOG_TAG = "RepositorySession";
  protected SessionStatus status = SessionStatus.UNSTARTED;
  protected Repository repository;
  protected RepositorySessionStoreDelegate delegate;

  


  protected ExecutorService delegateQueue  = Executors.newSingleThreadExecutor();

  




  protected ExecutorService storeWorkQueue = Executors.newSingleThreadExecutor();

  
  public long lastSyncTimestamp;

  public static long now() {
    return System.currentTimeMillis();
  }

  public RepositorySession(Repository repository) {
    this.repository = repository;
  }

  public abstract void guidsSince(long timestamp, RepositorySessionGuidsSinceDelegate delegate);
  public abstract void fetchSince(long timestamp, RepositorySessionFetchRecordsDelegate delegate);
  public abstract void fetch(String[] guids, RepositorySessionFetchRecordsDelegate delegate);
  public abstract void fetchAll(RepositorySessionFetchRecordsDelegate delegate);

  






  public boolean dataAvailable() {
    return true;
  }

  












  public void setStoreDelegate(RepositorySessionStoreDelegate delegate) {
    Log.d(LOG_TAG, "Setting store delegate to " + delegate);
    this.delegate = delegate;
  }
  public abstract void store(Record record) throws NoStoreDelegateException;

  public void storeDone() {
    Log.d(LOG_TAG, "Scheduling onStoreCompleted for after storing is done.");
    Runnable command = new Runnable() {
      @Override
      public void run() {
        delegate.onStoreCompleted();
      }
    };
    storeWorkQueue.execute(command);
  }

  public abstract void wipe(RepositorySessionWipeDelegate delegate);

  public void unbundle(RepositorySessionBundle bundle) {
    this.lastSyncTimestamp = 0;
    if (bundle == null) {
      return;
    }
    if (bundle.containsKey("timestamp")) {
      try {
        this.lastSyncTimestamp = bundle.getLong("timestamp");
      } catch (Exception e) {
        
      }
    }
  }

  private static void error(String msg) {
    System.err.println("ERROR: " + msg);
    Log.e(LOG_TAG, msg);
  }

  




  protected void sharedBegin() throws InvalidSessionTransitionException {
    if (this.status == SessionStatus.UNSTARTED) {
      this.status = SessionStatus.ACTIVE;
    } else {
      error("Tried to begin() an already active or finished session");
      throw new InvalidSessionTransitionException(null);
    }
  }

  public void begin(RepositorySessionBeginDelegate delegate) {
    try {
      sharedBegin();
      delegate.deferredBeginDelegate(delegateQueue).onBeginSucceeded(this);
    } catch (Exception e) {
      delegate.deferredBeginDelegate(delegateQueue).onBeginFailed(e);
    }
  }

  protected RepositorySessionBundle getBundle() {
    return this.getBundle(null);
  }

  











  protected RepositorySessionBundle getBundle(RepositorySessionBundle optional) {
    System.out.println("RepositorySession.getBundle(optional).");
    
    RepositorySessionBundle bundle = (optional == null) ? new RepositorySessionBundle() : optional;
    bundle.put("timestamp", this.lastSyncTimestamp);
    System.out.println("Setting bundle timestamp to " + this.lastSyncTimestamp);
    return bundle;
  }

  





  public void abort(RepositorySessionFinishDelegate delegate) {
    this.status = SessionStatus.DONE;    
    delegate.deferredFinishDelegate(delegateQueue).onFinishSucceeded(this, this.getBundle(null));
  }

  public void finish(final RepositorySessionFinishDelegate delegate) {
    if (this.status == SessionStatus.ACTIVE) {
      this.status = SessionStatus.DONE;
      delegate.deferredFinishDelegate(delegateQueue).onFinishSucceeded(this, this.getBundle(null));
    } else {
      Log.e(LOG_TAG, "Tried to finish() an unstarted or already finished session");
      Exception e = new InvalidSessionTransitionException(null);
      delegate.deferredFinishDelegate(delegateQueue).onFinishFailed(e);
    }
    Log.i(LOG_TAG, "Shutting down work queues.");
 
 
  }

  public boolean isActive() {
    return status == SessionStatus.ACTIVE;
  }

  public void abort() {
    
    status = SessionStatus.ABORTED;
    storeWorkQueue.shutdown();
    delegateQueue.shutdown();
  }
}

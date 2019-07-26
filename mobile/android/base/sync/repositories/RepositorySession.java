



package org.mozilla.gecko.sync.repositories;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionBeginDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionFetchRecordsDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionFinishDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionGuidsSinceDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionStoreDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionWipeDelegate;
import org.mozilla.gecko.sync.repositories.domain.Record;




















public abstract class RepositorySession {

  public enum SessionStatus {
    UNSTARTED,
    ACTIVE,
    ABORTED,
    DONE
  }

  private static final String LOG_TAG = "RepositorySession";

  protected static void trace(String message) {
    Logger.trace(LOG_TAG, message);
  }

  private SessionStatus status = SessionStatus.UNSTARTED;
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
  public abstract void fetch(String[] guids, RepositorySessionFetchRecordsDelegate delegate) throws InactiveSessionException;
  public abstract void fetchAll(RepositorySessionFetchRecordsDelegate delegate);

  






  public boolean dataAvailable() {
    return true;
  }

  


  public boolean shouldSkip() {
    return false;
  }

  












  public void setStoreDelegate(RepositorySessionStoreDelegate delegate) {
    Logger.debug(LOG_TAG, "Setting store delegate to " + delegate);
    this.delegate = delegate;
  }
  public abstract void store(Record record) throws NoStoreDelegateException;

  public void storeDone() {
    
    
    
    storeDone(now());
  }

  public void storeDone(final long end) {
    Logger.debug(LOG_TAG, "Scheduling onStoreCompleted for after storing is done: " + end);
    Runnable command = new Runnable() {
      @Override
      public void run() {
        delegate.onStoreCompleted(end);
      }
    };
    storeWorkQueue.execute(command);
  }

  public abstract void wipe(RepositorySessionWipeDelegate delegate);

  public void unbundle(RepositorySessionBundle bundle) {
    this.lastSyncTimestamp = bundle == null ? 0 : bundle.getTimestamp();
  }

  




  protected void sharedBegin() throws InvalidSessionTransitionException {
    Logger.debug(LOG_TAG, "Shared begin.");
    if (delegateQueue.isShutdown()) {
      throw new InvalidSessionTransitionException(null);
    }
    if (storeWorkQueue.isShutdown()) {
      throw new InvalidSessionTransitionException(null);
    }
    this.transitionFrom(SessionStatus.UNSTARTED, SessionStatus.ACTIVE);
  }

  






  public void begin(RepositorySessionBeginDelegate delegate) throws InvalidSessionTransitionException {
    sharedBegin();
    delegate.deferredBeginDelegate(delegateQueue).onBeginSucceeded(this);
  }

  protected RepositorySessionBundle getBundle() {
    return this.getBundle(null);
  }

  








  protected RepositorySessionBundle getBundle(RepositorySessionBundle optional) {
    
    RepositorySessionBundle bundle = (optional == null) ? new RepositorySessionBundle(this.lastSyncTimestamp) : optional;
    Logger.debug(LOG_TAG, "Setting bundle timestamp to " + this.lastSyncTimestamp + ".");

    return bundle;
  }

  



  public void abort(RepositorySessionFinishDelegate delegate) {
    this.abort();
    delegate.deferredFinishDelegate(delegateQueue).onFinishSucceeded(this, this.getBundle(null));
  }

  



  public void abort() {
    
    this.setStatus(SessionStatus.ABORTED);
    try {
      storeWorkQueue.shutdownNow();
    } catch (Exception e) {
      Logger.error(LOG_TAG, "Caught exception shutting down store work queue.", e);
    }
    try {
      delegateQueue.shutdown();
    } catch (Exception e) {
      Logger.error(LOG_TAG, "Caught exception shutting down delegate queue.", e);
    }
  }

  






  public void finish(final RepositorySessionFinishDelegate delegate) throws InactiveSessionException {
    try {
      this.transitionFrom(SessionStatus.ACTIVE, SessionStatus.DONE);
      delegate.deferredFinishDelegate(delegateQueue).onFinishSucceeded(this, this.getBundle(null));
    } catch (InvalidSessionTransitionException e) {
      Logger.error(LOG_TAG, "Tried to finish() an unstarted or already finished session");
      throw new InactiveSessionException(e);
    }

    Logger.trace(LOG_TAG, "Shutting down work queues.");
    storeWorkQueue.shutdown();
    delegateQueue.shutdown();
  }

  



  protected synchronized void executeDelegateCommand(Runnable command)
      throws InactiveSessionException {
    if (!isActive() || delegateQueue.isShutdown()) {
      throw new InactiveSessionException(null);
    }
    delegateQueue.execute(command);
  }

  public synchronized void ensureActive() throws InactiveSessionException {
    if (!isActive()) {
      throw new InactiveSessionException(null);
    }
  }

  public synchronized boolean isActive() {
    return status == SessionStatus.ACTIVE;
  }

  public synchronized SessionStatus getStatus() {
    return status;
  }

  public synchronized void setStatus(SessionStatus status) {
    this.status = status;
  }

  public synchronized void transitionFrom(SessionStatus from, SessionStatus to) throws InvalidSessionTransitionException {
    if (from == null || this.status == from) {
      Logger.trace(LOG_TAG, "Successfully transitioning from " + this.status + " to " + to);

      this.status = to;
      return;
    }
    Logger.warn(LOG_TAG, "Wanted to transition from " + from + " but in state " + this.status);
    throw new InvalidSessionTransitionException(null);
  }

  


































  protected Record reconcileRecords(final Record remoteRecord,
                                    final Record localRecord,
                                    final long lastRemoteRetrieval,
                                    final long lastLocalRetrieval) {
    Logger.debug(LOG_TAG, "Reconciling remote " + remoteRecord.guid + " against local " + localRecord.guid);

    if (localRecord.equalPayloads(remoteRecord)) {
      if (remoteRecord.lastModified > localRecord.lastModified) {
        Logger.debug(LOG_TAG, "Records are equal. No record application needed.");
        return null;
      }

      
      return null;
    }

    
    
    
    
    
    boolean localIsMoreRecent = localRecord.lastModified > remoteRecord.lastModified;
    Logger.debug(LOG_TAG, "Local record is more recent? " + localIsMoreRecent);
    Record donor = localIsMoreRecent ? localRecord : remoteRecord;

    
    
    
    Record out = donor.copyWithIDs(remoteRecord.guid, localRecord.androidID);

    
    
    
    if (!localIsMoreRecent) {
      trackGUID(out.guid);
    }
    return out;
  }

  







  protected void trackGUID(String guid) {
  }

  protected synchronized void untrackGUIDs(Collection<String> guids) {
  }

  protected void untrackGUID(String guid) {
  }

  
  public Iterator<String> getTrackedRecordIDs() {
    return new ArrayList<String>().iterator();
  }
}






































package org.mozilla.gecko.sync.synchronizer;

import org.mozilla.gecko.sync.SynchronizerConfiguration;
import org.mozilla.gecko.sync.repositories.Repository;
import org.mozilla.gecko.sync.repositories.RepositorySessionBundle;

import android.content.Context;
import android.util.Log;









public class Synchronizer {

  





  public class SynchronizerDelegateSessionDelegate implements
      SynchronizerSessionDelegate {

    private static final String LOG_TAG = "SynchronizerDelegateSessionDelegate";
    private SynchronizerDelegate synchronizerDelegate;
    private SynchronizerSession  session;

    public SynchronizerDelegateSessionDelegate(SynchronizerDelegate delegate) {
      this.synchronizerDelegate = delegate;
    }

    @Override
    public void onInitialized(SynchronizerSession session) {
      this.session = session;
      session.synchronize();
    }

    @Override
    public void onSynchronized(SynchronizerSession synchronizerSession) {
      Log.d(LOG_TAG, "Got onSynchronized.");
      Log.d(LOG_TAG, "Notifying SynchronizerDelegate.");
      this.synchronizerDelegate.onSynchronized(synchronizerSession.getSynchronizer());
    }

    @Override
    public void onSynchronizeSkipped(SynchronizerSession synchronizerSession) {
      Log.d(LOG_TAG, "Got onSynchronizeSkipped.");
      Log.d(LOG_TAG, "Notifying SynchronizerDelegate as if on success.");
      this.synchronizerDelegate.onSynchronized(synchronizerSession.getSynchronizer());
    }

    @Override
    public void onSynchronizeFailed(SynchronizerSession session,
                                    Exception lastException, String reason) {
      this.synchronizerDelegate.onSynchronizeFailed(session.getSynchronizer(), lastException, reason);
    }

    @Override
    public void onSynchronizeAborted(SynchronizerSession synchronizerSession) {
      this.synchronizerDelegate.onSynchronizeAborted(session.getSynchronizer());
    }

    @Override
    public void onFetchError(Exception e) {
      session.abort();
      synchronizerDelegate.onSynchronizeFailed(session.getSynchronizer(), e, "Got fetch error.");
    }

    @Override
    public void onStoreError(Exception e) {
      session.abort();
      synchronizerDelegate.onSynchronizeFailed(session.getSynchronizer(), e, "Got store error.");
    }

    @Override
    public void onSessionError(Exception e) {
      session.abort();
      synchronizerDelegate.onSynchronizeFailed(session.getSynchronizer(), e, "Got session error.");
    }
  }

  public Repository repositoryA;
  public Repository repositoryB;
  public RepositorySessionBundle bundleA;
  public RepositorySessionBundle bundleB;

  public void synchronize(Context context, SynchronizerDelegate delegate) {
    SynchronizerDelegateSessionDelegate sessionDelegate = new SynchronizerDelegateSessionDelegate(delegate);
    SynchronizerSession session = new SynchronizerSession(this, sessionDelegate);
    session.init(context, bundleA, bundleB);
  }

  public SynchronizerConfiguration save() {
    String syncID = null;      
    return new SynchronizerConfiguration(syncID, bundleA, bundleB);
  }

  
  public void load(SynchronizerConfiguration config) {
    bundleA = config.remoteBundle;
    bundleB = config.localBundle;
    
  }
}

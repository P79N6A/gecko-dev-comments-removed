



package org.mozilla.gecko.sync.synchronizer;

import org.mozilla.gecko.sync.SynchronizerConfiguration;
import org.mozilla.gecko.sync.repositories.Repository;
import org.mozilla.gecko.sync.repositories.RepositorySessionBundle;

import android.content.Context;
import android.util.Log;

















public class Synchronizer implements SynchronizerSessionDelegate {
  public static final String LOG_TAG = "SyncDelSDelegate";

  protected String configSyncID; 

  protected SynchronizerDelegate synchronizerDelegate;

  @Override
  public void onInitialized(SynchronizerSession session) {
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

  public Repository repositoryA;
  public Repository repositoryB;
  public RepositorySessionBundle bundleA;
  public RepositorySessionBundle bundleB;

  


  public SynchronizerSession getSynchronizerSession() {
    return new SynchronizerSession(this, this);
  }

  


  public void synchronize(Context context, SynchronizerDelegate delegate) {
    this.synchronizerDelegate = delegate;
    SynchronizerSession session = getSynchronizerSession();
    session.init(context, bundleA, bundleB);
  }

  public SynchronizerConfiguration save() {
    return new SynchronizerConfiguration(configSyncID, bundleA, bundleB);
  }

  






  public void load(SynchronizerConfiguration config) {
    bundleA = config.remoteBundle;
    bundleB = config.localBundle;
    configSyncID  = config.syncID;
  }
}

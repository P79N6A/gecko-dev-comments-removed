



package org.mozilla.gecko.sync.synchronizer;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.SynchronizerConfiguration;
import org.mozilla.gecko.sync.repositories.Repository;
import org.mozilla.gecko.sync.repositories.RepositorySessionBundle;

import android.content.Context;

















public class Synchronizer implements SynchronizerSessionDelegate {
  public static final String LOG_TAG = "SyncDelSDelegate";

  protected String configSyncID; 

  protected SynchronizerDelegate synchronizerDelegate;

  protected SynchronizerSession session = null;

  public SynchronizerSession getSynchronizerSession() {
    return session;
  }

  @Override
  public void onInitialized(SynchronizerSession session) {
    session.synchronize();
  }

  @Override
  public void onSynchronized(SynchronizerSession synchronizerSession) {
    Logger.debug(LOG_TAG, "Got onSynchronized.");
    Logger.debug(LOG_TAG, "Notifying SynchronizerDelegate.");
    this.synchronizerDelegate.onSynchronized(synchronizerSession.getSynchronizer());
  }

  @Override
  public void onSynchronizeSkipped(SynchronizerSession synchronizerSession) {
    Logger.debug(LOG_TAG, "Got onSynchronizeSkipped.");
    Logger.debug(LOG_TAG, "Notifying SynchronizerDelegate as if on success.");
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

  


  protected SynchronizerSession newSynchronizerSession() {
    return new SynchronizerSession(this, this);
  }

  


  public void synchronize(Context context, SynchronizerDelegate delegate) {
    this.synchronizerDelegate = delegate;
    this.session = newSynchronizerSession();
    this.session.init(context, bundleA, bundleB);
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

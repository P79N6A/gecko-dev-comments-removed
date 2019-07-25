




































package org.mozilla.gecko.sync.synchronizer;

public interface SynchronizerSessionDelegate {
  public void onInitialized(SynchronizerSession session);

  public void onSynchronized(SynchronizerSession session);
  public void onSynchronizeFailed(SynchronizerSession session, Exception lastException, String reason);
  public void onSynchronizeAborted(SynchronizerSession synchronizerSession);
  public void onSynchronizeSkipped(SynchronizerSession synchronizerSession);

  
  public void onFetchError(Exception e);
  public void onStoreError(Exception e);
  public void onSessionError(Exception e);

}





package org.mozilla.gecko.sync.synchronizer;

public interface SynchronizerDelegate {
  public void onSynchronized(Synchronizer synchronizer);
  public void onSynchronizeFailed(Synchronizer synchronizer, Exception lastException, String reason);
}

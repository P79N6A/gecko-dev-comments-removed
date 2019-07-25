




































package org.mozilla.gecko.sync.repositories.delegates;

import java.util.concurrent.ExecutorService;

public interface RepositorySessionWipeDelegate {
  public void onWipeFailed(Exception ex);
  public void onWipeSucceeded();
  public RepositorySessionWipeDelegate deferredWipeDelegate(ExecutorService executor);
}

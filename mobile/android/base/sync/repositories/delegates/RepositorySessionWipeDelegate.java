




































package org.mozilla.gecko.sync.repositories.delegates;

public interface RepositorySessionWipeDelegate {
  public void onWipeFailed(Exception ex);
  public void onWipeSucceeded();
  public RepositorySessionWipeDelegate deferredWipeDelegate();
}

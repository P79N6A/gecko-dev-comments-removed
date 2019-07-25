




































package org.mozilla.gecko.sync.repositories.delegates;

public interface RepositorySessionGuidsSinceDelegate {
  public void onGuidsSinceFailed(Exception ex);
  public void onGuidsSinceSucceeded(String[] guids);
}

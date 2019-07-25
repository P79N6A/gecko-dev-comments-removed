




































package org.mozilla.gecko.sync.repositories.delegates;

import java.util.concurrent.ExecutorService;

import org.mozilla.gecko.sync.repositories.RepositorySession;









public interface RepositorySessionBeginDelegate {
  public void onBeginFailed(Exception ex);
  public void onBeginSucceeded(RepositorySession session);
  public RepositorySessionBeginDelegate deferredBeginDelegate(ExecutorService executor);
}

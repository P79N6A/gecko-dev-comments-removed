





































package org.mozilla.gecko.sync.repositories.delegates;

import org.mozilla.gecko.sync.repositories.RepositorySession;



public interface RepositorySessionCreationDelegate {
  public void onSessionCreateFailed(Exception ex);
  public void onSessionCreated(RepositorySession session);
  public RepositorySessionCreationDelegate deferredCreationDelegate();
}
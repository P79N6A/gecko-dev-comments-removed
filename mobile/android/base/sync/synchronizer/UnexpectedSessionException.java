




































package org.mozilla.gecko.sync.synchronizer;

import org.mozilla.gecko.sync.SyncException;
import org.mozilla.gecko.sync.repositories.RepositorySession;










public class UnexpectedSessionException extends SyncException {
  private static final long serialVersionUID = 949010933527484721L;
  public RepositorySession session;

  public UnexpectedSessionException(RepositorySession session) {
    this.session = session;
  }
}

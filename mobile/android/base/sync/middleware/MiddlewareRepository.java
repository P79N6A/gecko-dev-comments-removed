




































package org.mozilla.gecko.sync.middleware;

import org.mozilla.gecko.sync.repositories.Repository;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionCreationDelegate;

public abstract class MiddlewareRepository extends Repository {

  public abstract class SessionCreationDelegate implements
      RepositorySessionCreationDelegate {

    
    
    @Override
    public RepositorySessionCreationDelegate deferredCreationDelegate() {
      return this;
    }

  }
}

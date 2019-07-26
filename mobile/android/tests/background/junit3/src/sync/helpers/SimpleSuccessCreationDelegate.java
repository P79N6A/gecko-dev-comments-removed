


package org.mozilla.gecko.background.sync.helpers;

import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionCreationDelegate;

public abstract class SimpleSuccessCreationDelegate extends DefaultDelegate implements RepositorySessionCreationDelegate {
  @Override
  public void onSessionCreateFailed(Exception ex) {
    performNotify("Session creation failed", ex);
  }

  @Override
  public RepositorySessionCreationDelegate deferredCreationDelegate() {
    return this;
  }
}

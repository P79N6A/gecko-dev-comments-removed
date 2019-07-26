


package org.mozilla.gecko.background.sync.helpers;

import java.util.concurrent.ExecutorService;

import org.mozilla.gecko.sync.repositories.RepositorySession;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionBeginDelegate;

public class DefaultBeginDelegate extends DefaultDelegate implements RepositorySessionBeginDelegate {
  @Override
  public void onBeginFailed(Exception ex) {
    performNotify("Begin failed", ex);
  }

  @Override
  public void onBeginSucceeded(RepositorySession session) {
    performNotify("Default begin delegate hit.", null);
  }

  @Override
  public RepositorySessionBeginDelegate deferredBeginDelegate(ExecutorService executor) {
    DefaultBeginDelegate copy;
    try {
      copy = (DefaultBeginDelegate) this.clone();
      copy.executor = executor;
      return copy;
    } catch (CloneNotSupportedException e) {
      return this;
    }
  }
}

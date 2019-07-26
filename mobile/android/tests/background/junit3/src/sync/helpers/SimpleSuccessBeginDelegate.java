


package org.mozilla.gecko.background.sync.helpers;

import java.util.concurrent.ExecutorService;

import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionBeginDelegate;

public abstract class SimpleSuccessBeginDelegate extends DefaultDelegate implements RepositorySessionBeginDelegate {
  @Override
  public void onBeginFailed(Exception ex) {
    performNotify("Begin failed", ex);
  }

  @Override
  public RepositorySessionBeginDelegate deferredBeginDelegate(ExecutorService executor) {
    return this;
  }
}

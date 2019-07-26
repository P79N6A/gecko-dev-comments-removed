


package org.mozilla.gecko.background.sync.helpers;

import java.util.concurrent.ExecutorService;

import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionFinishDelegate;

public abstract class SimpleSuccessFinishDelegate extends DefaultDelegate implements RepositorySessionFinishDelegate {
  @Override
  public void onFinishFailed(Exception ex) {
    performNotify("Finish failed", ex);
  }

  @Override
  public RepositorySessionFinishDelegate deferredFinishDelegate(ExecutorService executor) {
    return this;
  }
}

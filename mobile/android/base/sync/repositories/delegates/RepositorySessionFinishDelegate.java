




































package org.mozilla.gecko.sync.repositories.delegates;

import java.util.concurrent.ExecutorService;

import org.mozilla.gecko.sync.repositories.RepositorySession;
import org.mozilla.gecko.sync.repositories.RepositorySessionBundle;

public interface RepositorySessionFinishDelegate {
  public void onFinishFailed(Exception ex);
  public void onFinishSucceeded(RepositorySession session, RepositorySessionBundle bundle);
  public RepositorySessionFinishDelegate deferredFinishDelegate(ExecutorService executor);
}

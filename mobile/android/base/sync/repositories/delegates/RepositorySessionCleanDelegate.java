




































package org.mozilla.gecko.sync.repositories.delegates;

import org.mozilla.gecko.sync.repositories.Repository;

public interface RepositorySessionCleanDelegate {
  public void onCleaned(Repository repo);
  public void onCleanFailed(Repository repo, Exception ex);
}

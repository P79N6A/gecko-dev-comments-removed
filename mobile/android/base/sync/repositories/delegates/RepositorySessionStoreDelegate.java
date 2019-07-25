




































package org.mozilla.gecko.sync.repositories.delegates;

import org.mozilla.gecko.sync.repositories.domain.Record;








public interface RepositorySessionStoreDelegate {
  public void onStoreFailed(Exception ex);
  public void onStoreSucceeded(Record record);
  public RepositorySessionStoreDelegate deferredStoreDelegate();
}

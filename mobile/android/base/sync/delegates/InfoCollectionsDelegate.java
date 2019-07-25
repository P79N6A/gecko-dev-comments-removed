




































package org.mozilla.gecko.sync.delegates;

import org.mozilla.gecko.sync.InfoCollections;
import org.mozilla.gecko.sync.net.SyncStorageResponse;

public interface InfoCollectionsDelegate {
  public void handleSuccess(InfoCollections global);
  public void handleFailure(SyncStorageResponse response);
  public void handleError(Exception e);
}

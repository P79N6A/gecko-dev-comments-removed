



package org.mozilla.gecko.sync;

import android.content.SyncResult;

public class NullClusterURLException extends SyncException {
  private static final long serialVersionUID = 4277845518548393161L;

  @Override
  public void updateStats(GlobalSession globalSession, SyncResult syncResult) {
    syncResult.stats.numAuthExceptions++;
  }
}

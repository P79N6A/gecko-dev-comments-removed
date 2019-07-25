




































package org.mozilla.gecko.sync;

import android.content.SyncResult;

public class SyncConfigurationException extends SyncException {
  private static final long serialVersionUID = 1107080177269358381L;

  @Override
  public void updateStats(GlobalSession globalSession, SyncResult syncResult) {
    syncResult.stats.numAuthExceptions++;
  }
}





package org.mozilla.gecko.sync;

import android.content.SyncResult;

public class NodeAuthenticationException extends SyncException {
  private static final long serialVersionUID = 8156745873212364352L;

  @Override
  public void updateStats(GlobalSession globalSession, SyncResult syncResult) {
    syncResult.stats.numAuthExceptions++;
  }
}

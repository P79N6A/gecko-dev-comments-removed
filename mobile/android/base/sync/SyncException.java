



package org.mozilla.gecko.sync;

import android.content.SyncResult;

public abstract class SyncException extends Exception {
  private static final long serialVersionUID = -6928990004393234738L;

  public SyncException() {
    super();
  }

  public SyncException(final Throwable e) {
    super(e);
  }

  








  public void updateStats(GlobalSession globalSession, SyncResult syncResult) {
    
    
    syncResult.databaseError = true;
  }
}

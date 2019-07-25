




































package org.mozilla.gecko.sync;

import org.mozilla.gecko.sync.stage.GlobalSyncStage.Stage;

import android.content.SyncResult;

public class AlreadySyncingException extends SyncException {
  Stage inState;
  public AlreadySyncingException(Stage currentState) {
    inState = currentState;
  }

  private static final long serialVersionUID = -5647548462539009893L;

  @Override
  public void updateStats(GlobalSession globalSession, SyncResult syncResult) {
  }
}

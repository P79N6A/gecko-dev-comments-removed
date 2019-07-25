




































package org.mozilla.gecko.sync.delegates;

import org.mozilla.gecko.sync.GlobalSession;
import org.mozilla.gecko.sync.stage.GlobalSyncStage.Stage;

public interface GlobalSessionCallback {

  



  void requestBackoff(long backoff);

  void handleAborted(GlobalSession globalSession, String reason);
  void handleError(GlobalSession globalSession, Exception ex);
  void handleSuccess(GlobalSession globalSession);
  void handleStageCompleted(Stage currentState, GlobalSession globalSession);

  boolean shouldBackOff();
}





package org.mozilla.gecko.sync.delegates;

import java.net.URI;

import org.mozilla.gecko.sync.GlobalSession;
import org.mozilla.gecko.sync.stage.GlobalSyncStage.Stage;

public interface BaseGlobalSessionCallback {
  



  void requestBackoff(long backoff);

  


  void informUnauthorizedResponse(GlobalSession globalSession, URI oldClusterURL);


  


  void informUpgradeRequiredResponse(GlobalSession session);

  void handleAborted(GlobalSession globalSession, String reason);
  void handleError(GlobalSession globalSession, Exception ex);
  void handleSuccess(GlobalSession globalSession);
  void handleStageCompleted(Stage currentState, GlobalSession globalSession);

  boolean shouldBackOff();
}

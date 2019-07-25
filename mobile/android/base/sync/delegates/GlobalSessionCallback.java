



package org.mozilla.gecko.sync.delegates;

import java.net.URI;

import org.mozilla.gecko.sync.GlobalSession;
import org.mozilla.gecko.sync.stage.GlobalSyncStage.Stage;

public interface GlobalSessionCallback {

  



  void requestBackoff(long backoff);

  


  boolean wantNodeAssignment();

  


  void informUnauthorizedResponse(GlobalSession globalSession, URI oldClusterURL);

  










  void informNodeAssigned(GlobalSession globalSession, URI oldClusterURL, URI newClusterURL);

  








  void informNodeAuthenticationFailed(GlobalSession globalSession, URI failedClusterURL);

  void handleAborted(GlobalSession globalSession, String reason);
  void handleError(GlobalSession globalSession, Exception ex);
  void handleSuccess(GlobalSession globalSession);
  void handleStageCompleted(Stage currentState, GlobalSession globalSession);

  boolean shouldBackOff();
}

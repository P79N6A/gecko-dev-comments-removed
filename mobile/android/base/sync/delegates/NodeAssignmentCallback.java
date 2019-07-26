



package org.mozilla.gecko.sync.delegates;

import java.net.URI;

import org.mozilla.gecko.sync.GlobalSession;

public interface NodeAssignmentCallback {
  


  public boolean wantNodeAssignment();

  










  public void informNodeAssigned(GlobalSession globalSession, URI oldClusterURL, URI newClusterURL);

  








  public void informNodeAuthenticationFailed(GlobalSession globalSession, URI failedClusterURL);

  public String nodeWeaveURL();
}

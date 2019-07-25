



package org.mozilla.gecko.sync.stage;

import org.mozilla.gecko.sync.GlobalSession;




public abstract class AbstractNonRepositorySyncStage implements GlobalSyncStage {
  protected final GlobalSession session;

  public AbstractNonRepositorySyncStage(GlobalSession session) {
    this.session = session;
  }

  @Override
  public void resetLocal() {
    
  }

  @Override
  public void wipeLocal() {
    
  }
}

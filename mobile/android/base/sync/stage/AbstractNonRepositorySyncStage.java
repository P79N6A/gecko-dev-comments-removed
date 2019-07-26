



package org.mozilla.gecko.sync.stage;





public abstract class AbstractNonRepositorySyncStage extends AbstractSessionManagingSyncStage {
  @Override
  protected void resetLocal() {
    
  }

  @Override
  protected void wipeLocal() {
    
  }

  public Integer getStorageVersion() {
    return null; 
  }
}

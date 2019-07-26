



package org.mozilla.gecko.sync.stage;



public class CompletedStage extends AbstractNonRepositorySyncStage {
  @Override
  public void execute() throws NoSuchStageException {
    
    
    session.completeSync();
  }
}

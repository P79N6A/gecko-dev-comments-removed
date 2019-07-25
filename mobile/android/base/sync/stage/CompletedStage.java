



package org.mozilla.gecko.sync.stage;

import org.mozilla.gecko.sync.GlobalSession;


public class CompletedStage extends AbstractNonRepositorySyncStage {

  public CompletedStage(GlobalSession session) {
    super(session);
  }

  @Override
  public void execute() throws NoSuchStageException {
    
    
    session.completeSync();
  }
}

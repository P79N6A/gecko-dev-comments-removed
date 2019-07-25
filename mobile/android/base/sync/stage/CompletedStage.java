




































package org.mozilla.gecko.sync.stage;

import org.mozilla.gecko.sync.GlobalSession;

public class CompletedStage implements GlobalSyncStage {

  @Override
  public void execute(GlobalSession session) throws NoSuchStageException {
    
    
    session.completeSync();
  }

}

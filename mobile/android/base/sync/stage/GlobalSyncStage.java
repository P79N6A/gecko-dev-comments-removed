




































package org.mozilla.gecko.sync.stage;

import org.mozilla.gecko.sync.GlobalSession;

public interface GlobalSyncStage {
  public static enum Stage {
    idle,                       
    checkPreconditions,         
    ensureClusterURL,           
    fetchInfoCollections,       
    fetchMetaGlobal,
    ensureKeysStage,
    







    syncBookmarks,
    syncHistory,
    completed,
  }
  public void execute(GlobalSession session) throws NoSuchStageException;
}

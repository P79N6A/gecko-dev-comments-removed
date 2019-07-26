



package org.mozilla.gecko.sync.stage;

import java.util.Collection;
import java.util.Collections;
import java.util.EnumSet;
import java.util.HashMap;
import java.util.Map;

import org.mozilla.gecko.sync.GlobalSession;


public interface GlobalSyncStage {
  public static enum Stage {
    idle,                       
    checkPreconditions,         
    ensureClusterURL,           
    fetchInfoCollections,       
    fetchMetaGlobal,
    ensureKeysStage,
    



    syncClientsEngine(SyncClientsEngineStage.STAGE_NAME),
    




    syncTabs("tabs"),
    syncPasswords("passwords"),
    syncBookmarks("bookmarks"),
    syncHistory("history"),
    syncFormHistory("forms"),

    uploadMetaGlobal,
    completed;

    
    private static final Map<String, Stage> named = new HashMap<String, Stage>();
    static {
      for (Stage s : EnumSet.allOf(Stage.class)) {
        if (s.getRepositoryName() != null) {
          named.put(s.getRepositoryName(), s);
        }
      }
    }

    public static Stage byName(final String name) {
      if (name == null) {
        return null;
      }
      return named.get(name);
    }

    


    public static Collection<Stage> getNamedStages() {
      return Collections.unmodifiableCollection(named.values());
    }

    
    private final String repositoryName;
    public String getRepositoryName() {
      return repositoryName;
    }

    private Stage() {
      this.repositoryName = null;
    }

    private Stage(final String name) {
      this.repositoryName = name;
    }
  }

  public void execute(GlobalSession session) throws NoSuchStageException;
  public void resetLocal(GlobalSession session);
  public void wipeLocal(GlobalSession session) throws Exception;

  





  public Integer getStorageVersion();
}





package org.mozilla.gecko.sync.repositories.android;

import java.util.HashSet;
import java.util.Map;
import java.util.Set;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.sync.repositories.NullCursorException;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionStoreDelegate;

































public class BookmarksDeletionManager {
  private static final String LOG_TAG = "BookmarkDelete";

  private final AndroidBrowserBookmarksDataAccessor dataAccessor;
  private RepositorySessionStoreDelegate delegate;

  private final int flushThreshold;

  private final HashSet<String> folders    = new HashSet<String>();
  private final HashSet<String> nonFolders = new HashSet<String>();
  private int nonFolderCount = 0;

  
  private HashSet<String> nonFolderParents = new HashSet<String>();
  private HashSet<String> folderParents    = new HashSet<String>();

  










  public BookmarksDeletionManager(AndroidBrowserBookmarksDataAccessor dataAccessor, int flushThreshold) {
    this.dataAccessor = dataAccessor;
    this.flushThreshold = flushThreshold;
  }

  





  public void setDelegate(RepositorySessionStoreDelegate delegate) {
    this.delegate = delegate;
  }

  public void deleteRecord(String guid, boolean isFolder, String parentGUID) {
    if (guid == null) {
      Logger.warn(LOG_TAG, "Cannot queue deletion of record with no GUID.");
      return;
    }
    Logger.debug(LOG_TAG, "Queuing deletion of " + guid);

    if (isFolder) {
      folders.add(guid);
      if (!folders.contains(parentGUID)) {
        
        folderParents.add(parentGUID);
      }

      nonFolderParents.remove(guid);
      folderParents.remove(guid);
      return;
    }

    if (!folders.contains(parentGUID)) {
      
      nonFolderParents.add(parentGUID);
    }

    if (nonFolders.add(guid)) {
      if (++nonFolderCount >= flushThreshold) {
        deleteNonFolders();
      }
    }
  }

  


  public void incrementalFlush() {
    
    deleteNonFolders();
  }

  








  public Set<String> flushAll(long orphanDestination, long now) throws NullCursorException {
    Logger.debug(LOG_TAG, "Doing complete flush of deleted items. Moving orphans to " + orphanDestination);
    deleteNonFolders();

    
    
    nonFolderParents.removeAll(folders);

    Logger.debug(LOG_TAG, "Bumping modified times for " + nonFolderParents.size() +
                          " parents of deleted non-folders.");
    dataAccessor.bumpModifiedByGUID(nonFolderParents, now);

    if (folders.size() > 0) {
      final String[] folderGUIDs = folders.toArray(new String[folders.size()]);
      final String[] folderIDs = getIDs(folderGUIDs);   
      int moved = dataAccessor.moveChildren(folderIDs, orphanDestination);
      if (moved > 0) {
        dataAccessor.bumpModified(orphanDestination, now);
      }

      
      
      final String folderWhere = RepoUtils.computeSQLInClause(folders.size(), BrowserContract.Bookmarks.GUID);
      dataAccessor.delete(folderWhere, folderGUIDs);
      invokeCallbacks(delegate, folderGUIDs);

      folderParents.removeAll(folders);
      Logger.debug(LOG_TAG, "Bumping modified times for " + folderParents.size() +
                            " parents of deleted folders.");
      dataAccessor.bumpModifiedByGUID(folderParents, now);

      
      folders.clear();
    }

    HashSet<String> ret = nonFolderParents;
    ret.addAll(folderParents);

    nonFolderParents = new HashSet<String>();
    folderParents    = new HashSet<String>();
    return ret;
  }

  private String[] getIDs(String[] guids) throws NullCursorException {
    
    String[] ids = new String[guids.length];
    Map<String, Long> guidsToIDs = dataAccessor.idsForGUIDs(guids);
    for (int i = 0; i < guids.length; ++i) {
      String guid = guids[i];
      Long id =  guidsToIDs.get(guid);
      if (id == null) {
        throw new IllegalArgumentException("Can't get ID for unknown record " + guid);
      }
      ids[i] = id.toString();
    }
    return ids;
  }

  


  private void deleteNonFolders() {
    if (nonFolderCount == 0) {
      Logger.debug(LOG_TAG, "No non-folders to delete.");
      return;
    }

    Logger.debug(LOG_TAG, "Applying deletion of " + nonFolderCount + " non-folders.");
    final String[] nonFolderGUIDs = nonFolders.toArray(new String[nonFolderCount]);
    final String nonFolderWhere = RepoUtils.computeSQLInClause(nonFolderCount, BrowserContract.Bookmarks.GUID);
    dataAccessor.delete(nonFolderWhere, nonFolderGUIDs);

    invokeCallbacks(delegate, nonFolderGUIDs);

    
    
    nonFolders.clear();
    nonFolderCount = 0;
  }

  private void invokeCallbacks(RepositorySessionStoreDelegate delegate,
                               String[] nonFolderGUIDs) {
    if (delegate == null) {
      return;
    }
    Logger.trace(LOG_TAG, "Invoking store callback for " + nonFolderGUIDs.length + " GUIDs.");
    for (String guid : nonFolderGUIDs) {
      delegate.onRecordStoreSucceeded(guid);
    }
  }

  


  public void clear() {
    nonFolders.clear();
    nonFolderCount = 0;
    folders.clear();
    nonFolderParents.clear();
    folderParents.clear();
  }
}

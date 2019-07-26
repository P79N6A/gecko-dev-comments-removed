



package org.mozilla.gecko.sync.repositories.android;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashSet;
import java.util.Map;
import java.util.Set;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.repositories.domain.BookmarkRecord;























public class BookmarksInsertionManager {
  public static final String LOG_TAG = "BookmarkInsert";
  public static boolean DEBUG = false;

  protected final int flushThreshold;
  protected final BookmarkInserter inserter;

  


  private final Set<String> insertedFolders = new HashSet<String>();

  




  private final Set<BookmarkRecord> nonFoldersToWrite = new LinkedHashSet<BookmarkRecord>();

  



  private final Map<String, Set<BookmarkRecord>> recordsWaitingForParent = new HashMap<String, Set<BookmarkRecord>>();

  











  public BookmarksInsertionManager(int flushThreshold, Collection<String> insertedFolders, BookmarkInserter inserter) {
    this.flushThreshold = flushThreshold;
    this.insertedFolders.addAll(insertedFolders);
    this.inserter = inserter;
  }

  protected void addRecordWithUnwrittenParent(BookmarkRecord record) {
    Set<BookmarkRecord> destination = recordsWaitingForParent.get(record.parentID);
    if (destination == null) {
      destination = new LinkedHashSet<BookmarkRecord>();
      recordsWaitingForParent.put(record.parentID, destination);
    }
    destination.add(record);
  }

  






  protected void recursivelyEnqueueRecordAndChildren(BookmarkRecord record) {
    if (record.isFolder()) {
      if (!inserter.insertFolder(record)) {
        Logger.warn(LOG_TAG, "Folder with known parent with guid " + record.parentID + " failed to insert!");
        return;
      }
      Logger.debug(LOG_TAG, "Folder with known parent with guid " + record.parentID + " inserted; adding to inserted folders.");
      insertedFolders.add(record.guid);
    } else {
      Logger.debug(LOG_TAG, "Non-folder has known parent with guid " + record.parentID + "; adding to insertion queue.");
      nonFoldersToWrite.add(record);
    }

    
    Set<BookmarkRecord> waiting = recordsWaitingForParent.remove(record.guid);
    if (waiting == null) {
      return;
    }
    for (BookmarkRecord waiter : waiting) {
      recursivelyEnqueueRecordAndChildren(waiter);
    }
  }

  





  protected void enqueueFolder(BookmarkRecord record) {
    Logger.debug(LOG_TAG, "Inserting folder with guid " + record.guid);

    if (!insertedFolders.contains(record.parentID)) {
      Logger.debug(LOG_TAG, "Folder has unknown parent with guid " + record.parentID + "; keeping until we see the parent.");
      addRecordWithUnwrittenParent(record);
      return;
    }

    
    recursivelyEnqueueRecordAndChildren(record);
    flushNonFoldersIfNecessary();
  }

  





  protected void enqueueNonFolder(BookmarkRecord record) {
    Logger.debug(LOG_TAG, "Inserting non-folder with guid " + record.guid);

    if (!insertedFolders.contains(record.parentID)) {
      Logger.debug(LOG_TAG, "Non-folder has unknown parent with guid " + record.parentID + "; keeping until we see the parent.");
      addRecordWithUnwrittenParent(record);
      return;
    }

    
    Logger.debug(LOG_TAG, "Non-folder has known parent with guid " + record.parentID + "; adding to insertion queue.");
    nonFoldersToWrite.add(record);
    flushNonFoldersIfNecessary();
  }

  





  public void enqueueRecord(BookmarkRecord record) {
    if (record.isFolder()) {
      enqueueFolder(record);
    } else {
      enqueueNonFolder(record);
    }
    if (DEBUG) {
      dumpState();
    }
  }

  


  protected void flushNonFolders() {
    inserter.bulkInsertNonFolders(nonFoldersToWrite); 
    nonFoldersToWrite.clear();
  }

  



  protected void flushNonFoldersIfNecessary() {
    int num = nonFoldersToWrite.size();
    if (num < flushThreshold) {
      Logger.debug(LOG_TAG, "Incremental flush called with " + num + " < " + flushThreshold + " non-folders; not flushing.");
      return;
    }
    Logger.debug(LOG_TAG, "Incremental flush called with " + num + " non-folders; flushing.");
    flushNonFolders();
  }

  



  public void finishUp() {
    
    
    int numFolders = 0;
    int numNonFolders = 0;
    for (Set<BookmarkRecord> records : recordsWaitingForParent.values()) {
      for (BookmarkRecord record : records) {
        if (!record.isFolder()) {
          numNonFolders += 1;
          nonFoldersToWrite.add(record);
          continue;
        }

        numFolders += 1;
        if (!inserter.insertFolder(record)) {
          Logger.warn(LOG_TAG, "Folder with known parent with guid " + record.parentID + " failed to insert!");
          continue;
        }

        Logger.debug(LOG_TAG, "Folder with known parent with guid " + record.parentID + " inserted; adding to inserted folders.");
        insertedFolders.add(record.guid);
      }
    }
    recordsWaitingForParent.clear();
    flushNonFolders();

    Logger.debug(LOG_TAG, "finishUp inserted " +
        numFolders + " folders without known parents and " +
        numNonFolders + " non-folders without known parents.");
    if (DEBUG) {
      dumpState();
    }
  }

  public void clear() {
    this.insertedFolders.clear();
    this.nonFoldersToWrite.clear();
    this.recordsWaitingForParent.clear();
  }

  
  public boolean isClear() {
    return nonFoldersToWrite.isEmpty() && recordsWaitingForParent.isEmpty();
  }

  
  public void dumpState() {
    ArrayList<String> readies = new ArrayList<String>();
    for (BookmarkRecord record : nonFoldersToWrite) {
      readies.add(record.guid);
    }
    String ready = Utils.toCommaSeparatedString(new ArrayList<String>(readies));

    ArrayList<String> waits = new ArrayList<String>();
    for (Set<BookmarkRecord> recs : recordsWaitingForParent.values()) {
      for (BookmarkRecord rec : recs) {
        waits.add(rec.guid);
      }
    }
    String waiting = Utils.toCommaSeparatedString(waits);
    String known = Utils.toCommaSeparatedString(insertedFolders);

    Logger.debug(LOG_TAG, "Q=(" + ready + "), W = (" + waiting + "), P=(" + known + ")");
  }

  public interface BookmarkInserter {
    









    public boolean insertFolder(BookmarkRecord record);

    










    public void bulkInsertNonFolders(Collection<BookmarkRecord> records);
  }
}

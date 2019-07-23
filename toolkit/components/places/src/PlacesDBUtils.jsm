






































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

let EXPORTED_SYMBOLS = [ "PlacesDBUtils" ];




const IS_CONTRACTID = "@mozilla.org/widget/idleservice;1";
const OS_CONTRACTID = "@mozilla.org/observer-service;1";
const HS_CONTRACTID = "@mozilla.org/browser/nav-history-service;1";
const BS_CONTRACTID = "@mozilla.org/browser/nav-bookmarks-service;1";
const TS_CONTRACTID = "@mozilla.org/timer;1";
const SB_CONTRACTID = "@mozilla.org/intl/stringbundle;1";
const TIM_CONTRACTID = "@mozilla.org/updates/timer-manager;1";

const PLACES_STRING_BUNDLE_URI = "chrome://places/locale/places.properties";

const FINISHED_MAINTENANCE_NOTIFICATION_TOPIC = "places-maintenance-finished";







const IDLE_TIMEOUT = 10 * 60 * 1000;



const IDLE_LOOKUP_REPEAT = 10 * 60 * 1000;


const MAINTENANCE_REPEAT =  24 * 60 * 60;




function nsPlacesDBUtils() {
  
  

  XPCOMUtils.defineLazyServiceGetter(this, "_bms", BS_CONTRACTID,
                                     "nsINavBookmarksService");

  XPCOMUtils.defineLazyServiceGetter(this, "_hs", HS_CONTRACTID,
                                     "nsINavHistoryService");

  XPCOMUtils.defineLazyServiceGetter(this, "_os", OS_CONTRACTID,
                                     "nsIObserverService");

  XPCOMUtils.defineLazyServiceGetter(this, "_idlesvc", IS_CONTRACTID,
                                     "nsIIdleService");

  XPCOMUtils.defineLazyGetter(this, "_dbConn", function() {
    return Cc[HS_CONTRACTID].getService(Ci.nsPIPlacesDatabase).DBConnection;
  });

  XPCOMUtils.defineLazyGetter(this, "_bundle", function() {
    return Cc[SB_CONTRACTID].
           getService(Ci.nsIStringBundleService).
           createBundle(PLACES_STRING_BUNDLE_URI);
  });

  
  try {
    let tim = Cc[TIM_CONTRACTID].getService(Ci.nsIUpdateTimerManager);
    tim.registerTimer("places-maintenance-timer", this, MAINTENANCE_REPEAT);
  } catch (ex) {
    
  }
}

nsPlacesDBUtils.prototype = {
  _idleLookupTimer: null,
  _statementsRunningCount: 0,

  
  

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsITimerCallback,
  ]),

  
  

  notify: function PDBU_notify(aTimer) {
    switch (aTimer) {
      case this._idleLookUpTimer:
        let idleTime = 0;
        try {
          idleTime = this._idlesvc.idleTime;
        } catch (ex) {}

        
        if (idleTime > IDLE_TIMEOUT) {
          
          this._idleLookUpTimer.cancel();
          this._idleLookUpTimer = null;

          
          this.maintenanceOnIdle();
        }
        break;
      default:
        
        this._idleLookUpTimer = Cc[TS_CONTRACTID].createInstance(Ci.nsITimer);
        this._idleLookUpTimer.initWithCallback(this, IDLE_LOOKUP_REPEAT,
                                            Ci.nsITimer.TYPE_REPEATING_SLACK);
        break;
    }
  },

  
  

  handleError: function PDBU_handleError(aError) {
    Cu.reportError("Async statement execution returned with '" +
                   aError.result + "', '" + aError.message + "'");
  },

  handleCompletion: function PDBU_handleCompletion(aReason) {
    
    if (--this._statementsRunningCount > 0)
      return;

    
    
    
    this._hs.runInBatchMode({runBatched: function(aUserData){}}, null);
    this._bms.runInBatchMode({runBatched: function(aUserData){}}, null);
    
    this._os.notifyObservers(null, FINISHED_MAINTENANCE_NOTIFICATION_TOPIC, null);
  },

  
  

  maintenanceOnIdle: function PDBU_maintenanceOnIdle() {
    
    let cleanupStatements = [];

    
    
    let deleteUnusedAnnoAttributes = this._dbConn.createStatement(
      "DELETE FROM moz_anno_attributes WHERE id IN ( " +
        "SELECT id FROM moz_anno_attributes n " +
        "WHERE NOT EXISTS " +
            "(SELECT id FROM moz_annos WHERE anno_attribute_id = n.id LIMIT 1) " +
          "AND NOT EXISTS " +
            "(SELECT id FROM moz_items_annos WHERE anno_attribute_id = n.id LIMIT 1) " +
      ")");
    cleanupStatements.push(deleteUnusedAnnoAttributes);

    
    
    let deleteInvalidAttributeAnnos = this._dbConn.createStatement(
      "DELETE FROM moz_annos WHERE id IN ( " +
        "SELECT id FROM moz_annos a " +
        "WHERE NOT EXISTS " +
          "(SELECT id FROM moz_anno_attributes " +
            "WHERE id = a.anno_attribute_id LIMIT 1) " +
      ")");
    cleanupStatements.push(deleteInvalidAttributeAnnos);

    
    let deleteOrphanAnnos = this._dbConn.createStatement(
      "DELETE FROM moz_annos WHERE id IN ( " +
        "SELECT id FROM moz_annos a " +
        "WHERE NOT EXISTS " +
          "(SELECT id FROM moz_places_temp WHERE id = a.place_id LIMIT 1) " +
        "AND NOT EXISTS " +
          "(SELECT id FROM moz_places WHERE id = a.place_id LIMIT 1) " +
      ")");
    cleanupStatements.push(deleteOrphanAnnos);

    
    
    
    
    let selectPlacesRoot = this._dbConn.createStatement(
      "SELECT id FROM moz_bookmarks WHERE id = :places_root");
    selectPlacesRoot.params["places_root"] = this._bms.placesRoot;
    if (!selectPlacesRoot.executeStep()) {
      
      let createPlacesRoot = this._dbConn.createStatement(
        "INSERT INTO moz_bookmarks (id, type, fk, parent, position, title) " +
        "VALUES (:places_root, 2, NULL, 0, 0, :title)");
      createPlacesRoot.params["places_root"] = this._bms.placesRoot;
      createPlacesRoot.params["title"] = "";
      cleanupStatements.push(createPlacesRoot);

      
      let fixPlacesRootChildren = this._dbConn.createStatement(
        "UPDATE moz_bookmarks SET parent = :places_root WHERE id IN " +
          "(SELECT folder_id FROM moz_bookmarks_roots " +
            "WHERE folder_id <> :places_root)");
      fixPlacesRootChildren.params["places_root"] = this._bms.placesRoot;
      cleanupStatements.push(fixPlacesRootChildren);
    }
    selectPlacesRoot.finalize();

    
    
    
    let updateRootTitleSql = "UPDATE moz_bookmarks SET title = :title " +
                             "WHERE id = :root_id AND title <> :title";
    
    let fixPlacesRootTitle = this._dbConn.createStatement(updateRootTitleSql);
    fixPlacesRootTitle.params["root_id"] = this._bms.placesRoot;
    fixPlacesRootTitle.params["title"] = "";
    cleanupStatements.push(fixPlacesRootTitle);
    
    let fixBookmarksMenuTitle = this._dbConn.createStatement(updateRootTitleSql);
    fixBookmarksMenuTitle.params["root_id"] = this._bms.bookmarksMenuFolder;
    fixBookmarksMenuTitle.params["title"] =
      this._bundle.GetStringFromName("BookmarksMenuFolderTitle");
    cleanupStatements.push(fixBookmarksMenuTitle);
    
    let fixBookmarksToolbarTitle = this._dbConn.createStatement(updateRootTitleSql);
    fixBookmarksToolbarTitle.params["root_id"] = this._bms.toolbarFolder;
    fixBookmarksToolbarTitle.params["title"] =
      this._bundle.GetStringFromName("BookmarksToolbarFolderTitle");
    cleanupStatements.push(fixBookmarksToolbarTitle);
    
    let fixUnsortedBookmarksTitle = this._dbConn.createStatement(updateRootTitleSql);
    fixUnsortedBookmarksTitle.params["root_id"] = this._bms.unfiledBookmarksFolder;
    fixUnsortedBookmarksTitle.params["title"] =
      this._bundle.GetStringFromName("UnsortedBookmarksFolderTitle");
    cleanupStatements.push(fixUnsortedBookmarksTitle);
    
    let fixTagsRootTitle = this._dbConn.createStatement(updateRootTitleSql);
    fixTagsRootTitle.params["root_id"] = this._bms.tagsFolder;
    fixTagsRootTitle.params["title"] =
      this._bundle.GetStringFromName("TagsFolderTitle");
    cleanupStatements.push(fixTagsRootTitle);

    
    
    
    let deleteNoPlaceItems = this._dbConn.createStatement(
      "DELETE FROM moz_bookmarks WHERE id NOT IN ( " +
        "SELECT folder_id FROM moz_bookmarks_roots " + 
      ") AND id IN (" +
        "SELECT b.id FROM moz_bookmarks b " +
        "WHERE fk NOT NULL AND b.type = :bookmark_type " +
          "AND NOT EXISTS (SELECT url FROM moz_places_temp WHERE id = b.fk LIMIT 1) " +
          "AND NOT EXISTS (SELECT url FROM moz_places WHERE id = b.fk LIMIT 1) " +
      ")");
    deleteNoPlaceItems.params["bookmark_type"] = this._bms.TYPE_BOOKMARK;
    cleanupStatements.push(deleteNoPlaceItems);

    
    let deleteBogusTagChildren = this._dbConn.createStatement(
      "DELETE FROM moz_bookmarks WHERE id NOT IN ( " +
        "SELECT folder_id FROM moz_bookmarks_roots " + 
      ") AND id IN (" +
        "SELECT b.id FROM moz_bookmarks b " +
        "WHERE b.parent IN " +
          "(SELECT id FROM moz_bookmarks WHERE parent = :tags_folder) " +
          "AND b.type <> :bookmark_type " +
      ")");
    deleteBogusTagChildren.params["tags_folder"] = this._bms.tagsFolder;
    deleteBogusTagChildren.params["bookmark_type"] = this._bms.TYPE_BOOKMARK;
    cleanupStatements.push(deleteBogusTagChildren);

    
    let deleteEmptyTags = this._dbConn.createStatement(
      "DELETE FROM moz_bookmarks WHERE id NOT IN ( " +
        "SELECT folder_id FROM moz_bookmarks_roots " + 
      ") AND id IN (" +
        "SELECT b.id FROM moz_bookmarks b " +
        "WHERE b.id IN " +
          "(SELECT id FROM moz_bookmarks WHERE parent = :tags_folder) " +
          "AND NOT EXISTS " +
            "(SELECT id from moz_bookmarks WHERE parent = b.id LIMIT 1) " +
      ")");
    deleteEmptyTags.params["tags_folder"] = this._bms.tagsFolder;
    cleanupStatements.push(deleteEmptyTags);

    
    let fixOrphanItems = this._dbConn.createStatement(
      "UPDATE moz_bookmarks SET parent = :unsorted_folder WHERE id NOT IN ( " +
        "SELECT folder_id FROM moz_bookmarks_roots " +  
      ") AND id IN (" +
        "SELECT b.id FROM moz_bookmarks b " +
        "WHERE b.parent <> 0 " + 
        "AND NOT EXISTS " +
          "(SELECT id FROM moz_bookmarks WHERE id = b.parent LIMIT 1) " +
      ")");
    fixOrphanItems.params["unsorted_folder"] = this._bms.unfiledBookmarksFolder;
    cleanupStatements.push(fixOrphanItems);

    
    let fixInvalidKeywords = this._dbConn.createStatement(
      "UPDATE moz_bookmarks SET keyword_id = NULL WHERE id NOT IN ( " +
        "SELECT folder_id FROM moz_bookmarks_roots " + 
      ") AND id IN ( " +
        "SELECT id FROM moz_bookmarks b " +
        "WHERE keyword_id NOT NULL " +
          "AND NOT EXISTS " +
            "(SELECT id FROM moz_keywords WHERE id = b.keyword_id LIMIT 1) " +
      ")");
    cleanupStatements.push(fixInvalidKeywords);

    
    
    
    
    let fixBookmarksAsFolders = this._dbConn.createStatement(
      "UPDATE moz_bookmarks SET type = :bookmark_type WHERE id NOT IN ( " +
        "SELECT folder_id FROM moz_bookmarks_roots " + 
      ") AND id IN ( " +
        "SELECT id FROM moz_bookmarks b " +
        "WHERE type IN (:folder_type, :separator_type, :dynamic_type) " +
          "AND fk NOTNULL " +
      ")");
    fixBookmarksAsFolders.params["bookmark_type"] = this._bms.TYPE_BOOKMARK;
    fixBookmarksAsFolders.params["folder_type"] = this._bms.TYPE_FOLDER;
    fixBookmarksAsFolders.params["separator_type"] = this._bms.TYPE_SEPARATOR;
    fixBookmarksAsFolders.params["dynamic_type"] = this._bms.TYPE_DYNAMIC_CONTAINER;
    cleanupStatements.push(fixBookmarksAsFolders);

    
    
    
    let fixFoldersAsBookmarks = this._dbConn.createStatement(
      "UPDATE moz_bookmarks SET type = :folder_type WHERE id NOT IN ( " +
        "SELECT folder_id FROM moz_bookmarks_roots " + 
      ") AND id IN ( " +
        "SELECT id FROM moz_bookmarks b " +
        "WHERE type = :bookmark_type " +
          "AND fk IS NULL " +
      ")");
    fixFoldersAsBookmarks.params["bookmark_type"] = this._bms.TYPE_BOOKMARK;
    fixFoldersAsBookmarks.params["folder_type"] = this._bms.TYPE_FOLDER;
    cleanupStatements.push(fixFoldersAsBookmarks);

    
    
    
    let fixFoldersAsDynamic = this._dbConn.createStatement(
      "UPDATE moz_bookmarks SET type = :folder_type WHERE id NOT IN ( " +
        "SELECT folder_id FROM moz_bookmarks_roots " + 
      ") AND id IN ( " +
        "SELECT id FROM moz_bookmarks b " +
        "WHERE type = :dynamic_type " +
          "AND folder_type IS NULL " +
      ")");
    fixFoldersAsDynamic.params["dynamic_type"] = this._bms.TYPE_DYNAMIC_CONTAINER;
    fixFoldersAsDynamic.params["folder_type"] = this._bms.TYPE_FOLDER;
    cleanupStatements.push(fixFoldersAsDynamic);

    
    
    
    let fixInvalidParents = this._dbConn.createStatement(
      "UPDATE moz_bookmarks SET parent = :unsorted_folder WHERE id NOT IN ( " +
        "SELECT folder_id FROM moz_bookmarks_roots " +  
      ") AND id IN ( " +
        "SELECT id FROM moz_bookmarks b " +
        "WHERE EXISTS " +
          "(SELECT id FROM moz_bookmarks WHERE id = b.parent " +
            "AND type IN (:bookmark_type, :separator_type, :dynamic_type) " +
            "LIMIT 1) " +
      ")");
    fixInvalidParents.params["unsorted_folder"] = this._bms.unfiledBookmarksFolder;
    fixInvalidParents.params["bookmark_type"] = this._bms.TYPE_BOOKMARK;
    fixInvalidParents.params["separator_type"] = this._bms.TYPE_SEPARATOR;
    fixInvalidParents.params["dynamic_type"] = this._bms.TYPE_DYNAMIC_CONTAINER;
    cleanupStatements.push(fixInvalidParents);































    
    
    
    
    let removeLivemarkStaticItems = this._dbConn.createStatement(
      "DELETE FROM moz_bookmarks WHERE type = :bookmark_type AND fk IN ( " +
        "SELECT id FROM moz_places WHERE url = :lmloading OR url = :lmfailed " +
      ")");
    removeLivemarkStaticItems.params["bookmark_type"] = this._bms.TYPE_BOOKMARK;
    removeLivemarkStaticItems.params["lmloading"] = "about:livemark-loading";
    removeLivemarkStaticItems.params["lmfailed"] = "about:livemark-failed";
    cleanupStatements.push(removeLivemarkStaticItems);

    
    
    let deleteOrphanIcons = this._dbConn.createStatement(
      "DELETE FROM moz_favicons WHERE id IN (" +
        "SELECT id FROM moz_favicons f " +
        "WHERE NOT EXISTS " +
          "(SELECT id FROM moz_places_temp WHERE favicon_id = f.id LIMIT 1) " +
          "AND NOT EXISTS" +
          "(SELECT id FROM moz_places WHERE favicon_id = f.id LIMIT 1) " +
      ")");
    cleanupStatements.push(deleteOrphanIcons);

    
    
    let deleteOrphanVisits = this._dbConn.createStatement(
      "DELETE FROM moz_historyvisits WHERE id IN (" +
        "SELECT id FROM moz_historyvisits v " +
        "WHERE NOT EXISTS " +
          "(SELECT id FROM moz_places_temp WHERE id = v.place_id LIMIT 1) " +
          "AND NOT EXISTS " +
          "(SELECT id FROM moz_places WHERE id = v.place_id LIMIT 1) " +
      ")");
    cleanupStatements.push(deleteOrphanVisits);

    
    
    let deleteOrphanInputHistory = this._dbConn.createStatement(
      "DELETE FROM moz_inputhistory WHERE place_id IN (" +
        "SELECT place_id FROM moz_inputhistory i " +
        "WHERE NOT EXISTS " +
          "(SELECT id FROM moz_places_temp WHERE id = i.place_id LIMIT 1) " +
          "AND NOT EXISTS " +
          "(SELECT id FROM moz_places WHERE id = i.place_id LIMIT 1) " +
      ")");
    cleanupStatements.push(deleteOrphanInputHistory);

    
    
    let deleteInvalidAttributeItemsAnnos = this._dbConn.createStatement(
      "DELETE FROM moz_items_annos WHERE id IN ( " +
        "SELECT id FROM moz_items_annos t " +
        "WHERE NOT EXISTS " +
          "(SELECT id FROM moz_anno_attributes " +
            "WHERE id = t.anno_attribute_id LIMIT 1) " +
      ")");
    cleanupStatements.push(deleteInvalidAttributeItemsAnnos);

    
    let deleteOrphanItemsAnnos = this._dbConn.createStatement(
      "DELETE FROM moz_items_annos WHERE id IN ( " +
        "SELECT id FROM moz_items_annos t " +
        "WHERE NOT EXISTS " +
          "(SELECT id FROM moz_bookmarks WHERE id = t.item_id LIMIT 1) " +
      ")");
    cleanupStatements.push(deleteOrphanItemsAnnos);

    
    
    let deleteUnusedKeywords = this._dbConn.createStatement(
      "DELETE FROM moz_keywords WHERE id IN ( " +
        "SELECT id FROM moz_keywords k " +
        "WHERE NOT EXISTS " +
          "(SELECT id FROM moz_bookmarks WHERE keyword_id = k.id LIMIT 1) " +
      ")");
    cleanupStatements.push(deleteUnusedKeywords);

    
    
    let fixInvalidFaviconIds = this._dbConn.createStatement(
      "UPDATE moz_places SET favicon_id = NULL WHERE id IN ( " +
        "SELECT id FROM moz_places h " +
        "WHERE favicon_id NOT NULL " +
          "AND NOT EXISTS " +
            "(SELECT id FROM moz_favicons WHERE id = h.favicon_id LIMIT 1) " +
      ")");
    cleanupStatements.push(fixInvalidFaviconIds);


























    

    
    this._statementsRunningCount = cleanupStatements.length;
    
    cleanupStatements.forEach(function (aStatement) {
        aStatement.executeAsync(this);
        aStatement.finalize();
      }, this);
  },

  




  checkAndFixDatabase: function PDBU_checkAndFixDatabase() {
    let log = [];
    let self = this;
    let sep = "- - -";

    function integrity() {
      let integrityCheckStmt =
        self._dbConn.createStatement("PRAGMA integrity_check");
      log.push("INTEGRITY");
      let logIndex = log.length;
      while (integrityCheckStmt.executeStep()) {
        log.push(integrityCheckStmt.getString(0));
      }
      integrityCheckStmt.finalize();
      log.push(sep);
      return log[logIndex] == "ok";
    }

    function vacuum() {
      log.push("VACUUM");
      let dirSvc = Cc["@mozilla.org/file/directory_service;1"].
                   getService(Ci.nsIProperties);
      let placesDBFile = dirSvc.get("ProfD", Ci.nsILocalFile);
      placesDBFile.append("places.sqlite");
      log.push("places.sqlite: " + placesDBFile.fileSize + " byte");
      self._dbConn.executeSimpleSQL("VACUUM");
      log.push(sep);
    }

    function backup() {
      log.push("BACKUP");
      let dirSvc = Cc["@mozilla.org/file/directory_service;1"].
                   getService(Ci.nsIProperties);
      let profD = dirSvc.get("ProfD", Ci.nsILocalFile);
      let placesDBFile = profD.clone();
      placesDBFile.append("places.sqlite");
      let backupDBFile = profD.clone();
      backupDBFile.append("places.sqlite.corrupt");
      backupDBFile.createUnique(backupDBFile.NORMAL_FILE_TYPE, 0666);
      let backupName = backupDBFile.leafName;
      backupDBFile.remove(false);
      placesDBFile.copyTo(profD, backupName);
      log.push(backupName);
      log.push(sep);
    }

    function reindex() {
      log.push("REINDEX");
      self._dbConn.executeSimpleSQL("REINDEX");
      log.push(sep);
    }

    function cleanup() {
      log.push("CLEANUP");
      self.maintenanceOnIdle()
      log.push(sep);
    }

    function stats() {
      log.push("STATS");
      let dirSvc = Cc["@mozilla.org/file/directory_service;1"].
                   getService(Ci.nsIProperties);
      let placesDBFile = dirSvc.get("ProfD", Ci.nsILocalFile);
      placesDBFile.append("places.sqlite");
      log.push("places.sqlite: " + placesDBFile.fileSize + " byte");
      let stmt = self._dbConn.createStatement(
        "SELECT name FROM sqlite_master WHERE type = :DBType");
      stmt.params["DBType"] = "table";
      while (stmt.executeStep()) {
        let tableName = stmt.getString(0);
        let countStmt = self._dbConn.createStatement(
        "SELECT count(*) FROM " + tableName);
        countStmt.executeStep();
        log.push(tableName + ": " + countStmt.getInt32(0));
        countStmt.finalize();
      }
      stmt.finalize();
      log.push(sep);
    }

    
    let integrityIsGood = integrity();

    
    
    if (!integrityIsGood) {
      
      backup();
      
      reindex();
      
      integrityIsGood = integrity();
    }

    
    
    if (integrityIsGood) {
      cleanup();
      vacuum();
      stats();
    }

    return log.join('\n');
  }
};

__defineGetter__("PlacesDBUtils", function() {
  delete this.PlacesDBUtils;
  return this.PlacesDBUtils = new nsPlacesDBUtils;
});

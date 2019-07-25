






































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PlacesUtils.jsm");

let EXPORTED_SYMBOLS = [ "PlacesDBUtils" ];




const FINISHED_MAINTENANCE_NOTIFICATION_TOPIC = "places-maintenance-finished";




XPCOMUtils.defineLazyGetter(this, "DBConn", function() {
  return PlacesUtils.history.QueryInterface(Ci.nsPIPlacesDatabase).DBConnection;
});




function nsPlacesDBUtils() {
}

nsPlacesDBUtils.prototype = {
  _statementsRunningCount: 0,

  
  

  handleError: function PDBU_handleError(aError) {
    Cu.reportError("Async statement execution returned with '" +
                   aError.result + "', '" + aError.message + "'");
  },

  handleCompletion: function PDBU_handleCompletion(aReason) {
    
    if (--this._statementsRunningCount > 0)
      return;

    
    
    
    PlacesUtils.history.runInBatchMode({runBatched: function(aUserData){}}, null);
    PlacesUtils.bookmarks.runInBatchMode({runBatched: function(aUserData){}}, null);
    
    Services.obs.notifyObservers(null, FINISHED_MAINTENANCE_NOTIFICATION_TOPIC, null);
  },

  
  

  maintenanceOnIdle: function PDBU_maintenanceOnIdle() {
    
    let cleanupStatements = [];

    
    
    let deleteUnusedAnnoAttributes = DBConn.createStatement(
      "DELETE FROM moz_anno_attributes WHERE id IN ( " +
        "SELECT id FROM moz_anno_attributes n " +
        "WHERE NOT EXISTS " +
            "(SELECT id FROM moz_annos WHERE anno_attribute_id = n.id LIMIT 1) " +
          "AND NOT EXISTS " +
            "(SELECT id FROM moz_items_annos WHERE anno_attribute_id = n.id LIMIT 1) " +
      ")");
    cleanupStatements.push(deleteUnusedAnnoAttributes);

    
    
    let deleteInvalidAttributeAnnos = DBConn.createStatement(
      "DELETE FROM moz_annos WHERE id IN ( " +
        "SELECT id FROM moz_annos a " +
        "WHERE NOT EXISTS " +
          "(SELECT id FROM moz_anno_attributes " +
            "WHERE id = a.anno_attribute_id LIMIT 1) " +
      ")");
    cleanupStatements.push(deleteInvalidAttributeAnnos);

    
    let deleteOrphanAnnos = DBConn.createStatement(
      "DELETE FROM moz_annos WHERE id IN ( " +
        "SELECT id FROM moz_annos a " +
        "WHERE NOT EXISTS " +
          "(SELECT id FROM moz_places WHERE id = a.place_id LIMIT 1) " +
      ")");
    cleanupStatements.push(deleteOrphanAnnos);

    
    
    
    
    let selectPlacesRoot = DBConn.createStatement(
      "SELECT id FROM moz_bookmarks WHERE id = :places_root");
    selectPlacesRoot.params["places_root"] = PlacesUtils.placesRootId;
    if (!selectPlacesRoot.executeStep()) {
      
      let createPlacesRoot = DBConn.createStatement(
        "INSERT INTO moz_bookmarks (id, type, fk, parent, position, title) " +
        "VALUES (:places_root, 2, NULL, 0, 0, :title)");
      createPlacesRoot.params["places_root"] = PlacesUtils.placesRootId;
      createPlacesRoot.params["title"] = "";
      cleanupStatements.push(createPlacesRoot);

      
      let fixPlacesRootChildren = DBConn.createStatement(
        "UPDATE moz_bookmarks SET parent = :places_root WHERE id IN " +
          "(SELECT folder_id FROM moz_bookmarks_roots " +
            "WHERE folder_id <> :places_root)");
      fixPlacesRootChildren.params["places_root"] = PlacesUtils.placesRootId;
      cleanupStatements.push(fixPlacesRootChildren);
    }
    selectPlacesRoot.finalize();

    
    
    
    let updateRootTitleSql = "UPDATE moz_bookmarks SET title = :title " +
                             "WHERE id = :root_id AND title <> :title";
    
    let fixPlacesRootTitle = DBConn.createStatement(updateRootTitleSql);
    fixPlacesRootTitle.params["root_id"] = PlacesUtils.placesRootId;
    fixPlacesRootTitle.params["title"] = "";
    cleanupStatements.push(fixPlacesRootTitle);
    
    let fixBookmarksMenuTitle = DBConn.createStatement(updateRootTitleSql);
    fixBookmarksMenuTitle.params["root_id"] = PlacesUtils.bookmarksMenuFolderId;
    fixBookmarksMenuTitle.params["title"] =
      PlacesUtils.getString("BookmarksMenuFolderTitle");
    cleanupStatements.push(fixBookmarksMenuTitle);
    
    let fixBookmarksToolbarTitle = DBConn.createStatement(updateRootTitleSql);
    fixBookmarksToolbarTitle.params["root_id"] = PlacesUtils.toolbarFolderId;
    fixBookmarksToolbarTitle.params["title"] =
      PlacesUtils.getString("BookmarksToolbarFolderTitle");
    cleanupStatements.push(fixBookmarksToolbarTitle);
    
    let fixUnsortedBookmarksTitle = DBConn.createStatement(updateRootTitleSql);
    fixUnsortedBookmarksTitle.params["root_id"] = PlacesUtils.unfiledBookmarksFolderId;
    fixUnsortedBookmarksTitle.params["title"] =
      PlacesUtils.getString("UnsortedBookmarksFolderTitle");
    cleanupStatements.push(fixUnsortedBookmarksTitle);
    
    let fixTagsRootTitle = DBConn.createStatement(updateRootTitleSql);
    fixTagsRootTitle.params["root_id"] = PlacesUtils.tagsFolderId;
    fixTagsRootTitle.params["title"] =
      PlacesUtils.getString("TagsFolderTitle");
    cleanupStatements.push(fixTagsRootTitle);

    
    
    
    let deleteNoPlaceItems = DBConn.createStatement(
      "DELETE FROM moz_bookmarks WHERE id NOT IN ( " +
        "SELECT folder_id FROM moz_bookmarks_roots " + 
      ") AND id IN (" +
        "SELECT b.id FROM moz_bookmarks b " +
        "WHERE fk NOT NULL AND b.type = :bookmark_type " +
          "AND NOT EXISTS (SELECT url FROM moz_places WHERE id = b.fk LIMIT 1) " +
      ")");
    deleteNoPlaceItems.params["bookmark_type"] = PlacesUtils.bookmarks.TYPE_BOOKMARK;
    cleanupStatements.push(deleteNoPlaceItems);

    
    let deleteBogusTagChildren = DBConn.createStatement(
      "DELETE FROM moz_bookmarks WHERE id NOT IN ( " +
        "SELECT folder_id FROM moz_bookmarks_roots " + 
      ") AND id IN (" +
        "SELECT b.id FROM moz_bookmarks b " +
        "WHERE b.parent IN " +
          "(SELECT id FROM moz_bookmarks WHERE parent = :tags_folder) " +
          "AND b.type <> :bookmark_type " +
      ")");
    deleteBogusTagChildren.params["tags_folder"] = PlacesUtils.tagsFolderId;
    deleteBogusTagChildren.params["bookmark_type"] = PlacesUtils.bookmarks.TYPE_BOOKMARK;
    cleanupStatements.push(deleteBogusTagChildren);

    
    let deleteEmptyTags = DBConn.createStatement(
      "DELETE FROM moz_bookmarks WHERE id NOT IN ( " +
        "SELECT folder_id FROM moz_bookmarks_roots " + 
      ") AND id IN (" +
        "SELECT b.id FROM moz_bookmarks b " +
        "WHERE b.id IN " +
          "(SELECT id FROM moz_bookmarks WHERE parent = :tags_folder) " +
          "AND NOT EXISTS " +
            "(SELECT id from moz_bookmarks WHERE parent = b.id LIMIT 1) " +
      ")");
    deleteEmptyTags.params["tags_folder"] = PlacesUtils.tagsFolderId;
    cleanupStatements.push(deleteEmptyTags);

    
    let fixOrphanItems = DBConn.createStatement(
      "UPDATE moz_bookmarks SET parent = :unsorted_folder WHERE id NOT IN ( " +
        "SELECT folder_id FROM moz_bookmarks_roots " +  
      ") AND id IN (" +
        "SELECT b.id FROM moz_bookmarks b " +
        "WHERE b.parent <> 0 " + 
        "AND NOT EXISTS " +
          "(SELECT id FROM moz_bookmarks WHERE id = b.parent LIMIT 1) " +
      ")");
    fixOrphanItems.params["unsorted_folder"] = PlacesUtils.unfiledBookmarksFolderId;
    cleanupStatements.push(fixOrphanItems);

    
    let fixInvalidKeywords = DBConn.createStatement(
      "UPDATE moz_bookmarks SET keyword_id = NULL WHERE id NOT IN ( " +
        "SELECT folder_id FROM moz_bookmarks_roots " + 
      ") AND id IN ( " +
        "SELECT id FROM moz_bookmarks b " +
        "WHERE keyword_id NOT NULL " +
          "AND NOT EXISTS " +
            "(SELECT id FROM moz_keywords WHERE id = b.keyword_id LIMIT 1) " +
      ")");
    cleanupStatements.push(fixInvalidKeywords);

    
    
    
    
    let fixBookmarksAsFolders = DBConn.createStatement(
      "UPDATE moz_bookmarks SET type = :bookmark_type WHERE id NOT IN ( " +
        "SELECT folder_id FROM moz_bookmarks_roots " + 
      ") AND id IN ( " +
        "SELECT id FROM moz_bookmarks b " +
        "WHERE type IN (:folder_type, :separator_type, :dynamic_type) " +
          "AND fk NOTNULL " +
      ")");
    fixBookmarksAsFolders.params["bookmark_type"] = PlacesUtils.bookmarks.TYPE_BOOKMARK;
    fixBookmarksAsFolders.params["folder_type"] = PlacesUtils.bookmarks.TYPE_FOLDER;
    fixBookmarksAsFolders.params["separator_type"] = PlacesUtils.bookmarks.TYPE_SEPARATOR;
    fixBookmarksAsFolders.params["dynamic_type"] = PlacesUtils.bookmarks.TYPE_DYNAMIC_CONTAINER;
    cleanupStatements.push(fixBookmarksAsFolders);

    
    
    
    let fixFoldersAsBookmarks = DBConn.createStatement(
      "UPDATE moz_bookmarks SET type = :folder_type WHERE id NOT IN ( " +
        "SELECT folder_id FROM moz_bookmarks_roots " + 
      ") AND id IN ( " +
        "SELECT id FROM moz_bookmarks b " +
        "WHERE type = :bookmark_type " +
          "AND fk IS NULL " +
      ")");
    fixFoldersAsBookmarks.params["bookmark_type"] = PlacesUtils.bookmarks.TYPE_BOOKMARK;
    fixFoldersAsBookmarks.params["folder_type"] = PlacesUtils.bookmarks.TYPE_FOLDER;
    cleanupStatements.push(fixFoldersAsBookmarks);

    
    
    
    let fixFoldersAsDynamic = DBConn.createStatement(
      "UPDATE moz_bookmarks SET type = :folder_type WHERE id NOT IN ( " +
        "SELECT folder_id FROM moz_bookmarks_roots " + 
      ") AND id IN ( " +
        "SELECT id FROM moz_bookmarks b " +
        "WHERE type = :dynamic_type " +
          "AND folder_type IS NULL " +
      ")");
    fixFoldersAsDynamic.params["dynamic_type"] = PlacesUtils.bookmarks.TYPE_DYNAMIC_CONTAINER;
    fixFoldersAsDynamic.params["folder_type"] = PlacesUtils.bookmarks.TYPE_FOLDER;
    cleanupStatements.push(fixFoldersAsDynamic);

    
    
    
    let fixInvalidParents = DBConn.createStatement(
      "UPDATE moz_bookmarks SET parent = :unsorted_folder WHERE id NOT IN ( " +
        "SELECT folder_id FROM moz_bookmarks_roots " +  
      ") AND id IN ( " +
        "SELECT id FROM moz_bookmarks b " +
        "WHERE EXISTS " +
          "(SELECT id FROM moz_bookmarks WHERE id = b.parent " +
            "AND type IN (:bookmark_type, :separator_type, :dynamic_type) " +
            "LIMIT 1) " +
      ")");
    fixInvalidParents.params["unsorted_folder"] = PlacesUtils.unfiledBookmarksFolderId;
    fixInvalidParents.params["bookmark_type"] = PlacesUtils.bookmarks.TYPE_BOOKMARK;
    fixInvalidParents.params["separator_type"] = PlacesUtils.bookmarks.TYPE_SEPARATOR;
    fixInvalidParents.params["dynamic_type"] = PlacesUtils.bookmarks.TYPE_DYNAMIC_CONTAINER;
    cleanupStatements.push(fixInvalidParents);































    
    
    
    let removeLivemarkStaticItems = DBConn.createStatement(
      "DELETE FROM moz_bookmarks WHERE type = :bookmark_type AND fk IN ( " +
        "SELECT id FROM moz_places WHERE url = :lmloading OR url = :lmfailed " +
      ")");
    removeLivemarkStaticItems.params["bookmark_type"] = PlacesUtils.bookmarks.TYPE_BOOKMARK;
    removeLivemarkStaticItems.params["lmloading"] = "about:livemark-loading";
    removeLivemarkStaticItems.params["lmfailed"] = "about:livemark-failed";
    cleanupStatements.push(removeLivemarkStaticItems);

    
    
    
    let fixEmptyNamedTags = DBConn.createStatement(
      "UPDATE moz_bookmarks SET title = :empty_title " +
      "WHERE length(title) = 0 AND type = :folder_type " +
        "AND parent = :tags_folder"
    );
    fixEmptyNamedTags.params["empty_title"] = "(notitle)";
    fixEmptyNamedTags.params["folder_type"] = PlacesUtils.bookmarks.TYPE_FOLDER;
    fixEmptyNamedTags.params["tags_folder"] = PlacesUtils.tagsFolderId;
    cleanupStatements.push(fixEmptyNamedTags);

    
    
    let deleteOrphanIcons = DBConn.createStatement(
      "DELETE FROM moz_favicons WHERE id IN (" +
        "SELECT id FROM moz_favicons f " +
        "WHERE NOT EXISTS " +
          "(SELECT id FROM moz_places WHERE favicon_id = f.id LIMIT 1) " +
      ")");
    cleanupStatements.push(deleteOrphanIcons);

    
    
    let deleteOrphanVisits = DBConn.createStatement(
      "DELETE FROM moz_historyvisits WHERE id IN (" +
        "SELECT id FROM moz_historyvisits v " +
        "WHERE NOT EXISTS " +
          "(SELECT id FROM moz_places WHERE id = v.place_id LIMIT 1) " +
      ")");
    cleanupStatements.push(deleteOrphanVisits);

    
    
    let deleteOrphanInputHistory = DBConn.createStatement(
      "DELETE FROM moz_inputhistory WHERE place_id IN (" +
        "SELECT place_id FROM moz_inputhistory i " +
        "WHERE NOT EXISTS " +
          "(SELECT id FROM moz_places WHERE id = i.place_id LIMIT 1) " +
      ")");
    cleanupStatements.push(deleteOrphanInputHistory);

    
    
    let deleteInvalidAttributeItemsAnnos = DBConn.createStatement(
      "DELETE FROM moz_items_annos WHERE id IN ( " +
        "SELECT id FROM moz_items_annos t " +
        "WHERE NOT EXISTS " +
          "(SELECT id FROM moz_anno_attributes " +
            "WHERE id = t.anno_attribute_id LIMIT 1) " +
      ")");
    cleanupStatements.push(deleteInvalidAttributeItemsAnnos);

    
    let deleteOrphanItemsAnnos = DBConn.createStatement(
      "DELETE FROM moz_items_annos WHERE id IN ( " +
        "SELECT id FROM moz_items_annos t " +
        "WHERE NOT EXISTS " +
          "(SELECT id FROM moz_bookmarks WHERE id = t.item_id LIMIT 1) " +
      ")");
    cleanupStatements.push(deleteOrphanItemsAnnos);

    
    
    let deleteUnusedKeywords = DBConn.createStatement(
      "DELETE FROM moz_keywords WHERE id IN ( " +
        "SELECT id FROM moz_keywords k " +
        "WHERE NOT EXISTS " +
          "(SELECT id FROM moz_bookmarks WHERE keyword_id = k.id LIMIT 1) " +
      ")");
    cleanupStatements.push(deleteUnusedKeywords);

    
    
    let fixInvalidFaviconIds = DBConn.createStatement(
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

  




  checkAndFixDatabase: function PDBU_checkAndFixDatabase(aLogCallback) {
    let log = [];
    let self = this;
    let sep = "- - -";

    function integrity() {
      let integrityCheckStmt =
        DBConn.createStatement("PRAGMA integrity_check");
      log.push("INTEGRITY");
      let logIndex = log.length;
      while (integrityCheckStmt.executeStep()) {
        log.push(integrityCheckStmt.getString(0));
      }
      integrityCheckStmt.finalize();
      log.push(sep);
      return log[logIndex] == "ok";
    }

    function vacuum(aVacuumCallback) {
      log.push("VACUUM");
      let dirSvc = Cc["@mozilla.org/file/directory_service;1"].
                   getService(Ci.nsIProperties);
      let placesDBFile = dirSvc.get("ProfD", Ci.nsILocalFile);
      placesDBFile.append("places.sqlite");
      log.push("places.sqlite: " + placesDBFile.fileSize + " byte");
      log.push(sep);
      let stmt = DBConn.createStatement("VACUUM");
      stmt.executeAsync({
        handleResult: function() {},
        handleError: function() {
          Cu.reportError("Maintenance VACUUM failed");
        },
        handleCompletion: function(aReason) {
          aVacuumCallback();
        }
      });
      stmt.finalize();
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
      DBConn.executeSimpleSQL("REINDEX");
      log.push(sep);
    }

    function cleanup() {
      log.push("CLEANUP");
      self.maintenanceOnIdle()
      log.push(sep);
    }

    function expire() {
      log.push("EXPIRE");
      
      
      let expiration = Cc["@mozilla.org/places/expiration;1"].
                       getService(Ci.nsIObserver);
      
      let prefs = Cc["@mozilla.org/preferences-service;1"].
                  getService(Ci.nsIPrefBranch);
      let limitURIs = prefs.getIntPref(
        "places.history.expiration.transient_current_max_pages");
      if (limitURIs >= 0)
        log.push("Current unique URIs limit: " + limitURIs);
      
      expiration.observe(null, "places-debug-start-expiration",
                         -1 );
      log.push(sep);
    }

    function stats() {
      log.push("STATS");
      let dirSvc = Cc["@mozilla.org/file/directory_service;1"].
                   getService(Ci.nsIProperties);
      let placesDBFile = dirSvc.get("ProfD", Ci.nsILocalFile);
      placesDBFile.append("places.sqlite");
      log.push("places.sqlite: " + placesDBFile.fileSize + " byte");
      let stmt = DBConn.createStatement(
        "SELECT name FROM sqlite_master WHERE type = :DBType");
      stmt.params["DBType"] = "table";
      while (stmt.executeStep()) {
        let tableName = stmt.getString(0);
        let countStmt = DBConn.createStatement(
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
      expire();
      vacuum(function () {
        stats();
        if (aLogCallback) {
          aLogCallback(log);
        }
        else {
          try {
            let console = Cc["@mozilla.org/consoleservice;1"].
                          getService(Ci.nsIConsoleService);
            console.logStringMessage(log.join('\n'));
          }
          catch(ex) {}
        }
      });
    }
  }

};

__defineGetter__("PlacesDBUtils", function() {
  delete this.PlacesDBUtils;
  return this.PlacesDBUtils = new nsPlacesDBUtils;
});







const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PlacesUtils.jsm");

this.EXPORTED_SYMBOLS = [ "PlacesDBUtils" ];




const FINISHED_MAINTENANCE_TOPIC = "places-maintenance-finished";

const BYTES_PER_MEBIBYTE = 1048576;




XPCOMUtils.defineLazyGetter(this, "DBConn", function() {
  return PlacesUtils.history.QueryInterface(Ci.nsPIPlacesDatabase).DBConnection;
});




this.PlacesDBUtils = {
  







  _executeTasks: function PDBU__executeTasks(aTasks)
  {
    if (PlacesDBUtils._isShuttingDown) {
      aTasks.log("- We are shutting down. Will not schedule the tasks.");
      aTasks.clear();
    }

    let task = aTasks.pop();
    if (task) {
      task.call(PlacesDBUtils, aTasks);
    }
    else {
      
      
      if (aTasks._telemetryStart) {
        Services.telemetry.getHistogramById("PLACES_IDLE_MAINTENANCE_TIME_MS")
                          .add(Date.now() - aTasks._telemetryStart);
        aTasks._telemetryStart = 0;
      }

      if (aTasks.callback) {
        let scope = aTasks.scope || Cu.getGlobalForObject(aTasks.callback);
        aTasks.callback.call(scope, aTasks.messages);
      }

      
      Services.prefs.setIntPref("places.database.lastMaintenance", parseInt(Date.now() / 1000));
      Services.obs.notifyObservers(null, FINISHED_MAINTENANCE_TOPIC, null);
    }
  },

  _isShuttingDown : false,
  shutdown: function PDBU_shutdown() {
    PlacesDBUtils._isShuttingDown = true;
  },

  








  maintenanceOnIdle: function PDBU_maintenanceOnIdle(aCallback, aScope)
  {
    let tasks = new Tasks([
      this.checkIntegrity
    , this.checkCoherence
    , this._refreshUI
    ]);
    tasks._telemetryStart = Date.now();
    tasks.callback = aCallback;
    tasks.scope = aScope;
    this._executeTasks(tasks);
  },

  









  checkAndFixDatabase: function PDBU_checkAndFixDatabase(aCallback, aScope)
  {
    let tasks = new Tasks([
      this.checkIntegrity
    , this.checkCoherence
    , this.expire
    , this.vacuum
    , this.stats
    , this._refreshUI
    ]);
    tasks.callback = aCallback;
    tasks.scope = aScope;
    this._executeTasks(tasks);
  },

  





  _refreshUI: function PDBU__refreshUI(aTasks)
  {
    let tasks = new Tasks(aTasks);

    
    PlacesUtils.history.runInBatchMode({
      runBatched: function (aUserData) {}
    }, null);
    PlacesDBUtils._executeTasks(tasks);
  },

  _handleError: function PDBU__handleError(aError)
  {
    Cu.reportError("Async statement execution returned with '" +
                   aError.result + "', '" + aError.message + "'");
  },

  





  reindex: function PDBU_reindex(aTasks)
  {
    let tasks = new Tasks(aTasks);
    tasks.log("> Reindex");

    let stmt = DBConn.createAsyncStatement("REINDEX");
    stmt.executeAsync({
      handleError: PlacesDBUtils._handleError,
      handleResult: function () {},

      handleCompletion: function (aReason)
      {
        if (aReason == Ci.mozIStorageStatementCallback.REASON_FINISHED) {
          tasks.log("+ The database has been reindexed");
        }
        else {
          tasks.log("- Unable to reindex database");
        }

        PlacesDBUtils._executeTasks(tasks);
      }
    });
    stmt.finalize();
  },

  





  _checkIntegritySkipReindex: function PDBU__checkIntegritySkipReindex(aTasks)
    this.checkIntegrity(aTasks, true),

  







  checkIntegrity: function PDBU_checkIntegrity(aTasks, aSkipReindex)
  {
    let tasks = new Tasks(aTasks);
    tasks.log("> Integrity check");

    
    let stmt = DBConn.createAsyncStatement("PRAGMA integrity_check(1)");
    stmt.executeAsync({
      handleError: PlacesDBUtils._handleError,

      _corrupt: false,
      handleResult: function (aResultSet)
      {
        let row = aResultSet.getNextRow();
        this._corrupt = row.getResultByIndex(0) != "ok";
      },

      handleCompletion: function (aReason)
      {
        if (aReason == Ci.mozIStorageStatementCallback.REASON_FINISHED) {
          if (this._corrupt) {
            tasks.log("- The database is corrupt");
            if (aSkipReindex) {
              tasks.log("- Unable to fix corruption, database will be replaced on next startup");
              Services.prefs.setBoolPref("places.database.replaceOnStartup", true);
              tasks.clear();
            }
            else {
              
              
              tasks.push(PlacesDBUtils._checkIntegritySkipReindex);
              tasks.push(PlacesDBUtils.reindex);
            }
          }
          else {
            tasks.log("+ The database is sane");
          }
        }
        else {
          tasks.log("- Unable to check database status");
          tasks.clear();
        }

        PlacesDBUtils._executeTasks(tasks);
      }
    });
    stmt.finalize();
  },

  





  checkCoherence: function PDBU_checkCoherence(aTasks)
  {
    let tasks = new Tasks(aTasks);
    tasks.log("> Coherence check");

    let stmts = PlacesDBUtils._getBoundCoherenceStatements();
    DBConn.executeAsync(stmts, stmts.length, {
      handleError: PlacesDBUtils._handleError,
      handleResult: function () {},

      handleCompletion: function (aReason)
      {
        if (aReason == Ci.mozIStorageStatementCallback.REASON_FINISHED) {
          tasks.log("+ The database is coherent");
        }
        else {
          tasks.log("- Unable to check database coherence");
          tasks.clear();
        }

        PlacesDBUtils._executeTasks(tasks);
      }
    });
    stmts.forEach(function (aStmt) aStmt.finalize());
  },

  _getBoundCoherenceStatements: function PDBU__getBoundCoherenceStatements()
  {
    let cleanupStatements = [];

    
    
    
    
    let deleteObsoleteAnnos = DBConn.createAsyncStatement(
      "DELETE FROM moz_annos "                      +
      "WHERE anno_attribute_id IN ( "               +
      "  SELECT id FROM moz_anno_attributes "       +
      "  WHERE name BETWEEN 'weave/' AND 'weave0' " +
      ")");
    cleanupStatements.push(deleteObsoleteAnnos);

    
    let deleteObsoleteItemsAnnos = DBConn.createAsyncStatement(
      "DELETE FROM moz_items_annos "                +
      "WHERE anno_attribute_id IN ( "               +
      "  SELECT id FROM moz_anno_attributes "       +
      "  WHERE name = 'sync/children' "             +
      "     OR name = 'placesInternal/GUID' "       +
      "     OR name BETWEEN 'weave/' AND 'weave0' " +
      ")");
    cleanupStatements.push(deleteObsoleteItemsAnnos);

    
    let deleteUnusedAnnoAttributes = DBConn.createAsyncStatement(
      "DELETE FROM moz_anno_attributes WHERE id IN ( " +
        "SELECT id FROM moz_anno_attributes n " +
        "WHERE NOT EXISTS " +
            "(SELECT id FROM moz_annos WHERE anno_attribute_id = n.id LIMIT 1) " +
          "AND NOT EXISTS " +
            "(SELECT id FROM moz_items_annos WHERE anno_attribute_id = n.id LIMIT 1) " +
      ")");
    cleanupStatements.push(deleteUnusedAnnoAttributes);

    
    
    let deleteInvalidAttributeAnnos = DBConn.createAsyncStatement(
      "DELETE FROM moz_annos WHERE id IN ( " +
        "SELECT id FROM moz_annos a " +
        "WHERE NOT EXISTS " +
          "(SELECT id FROM moz_anno_attributes " +
            "WHERE id = a.anno_attribute_id LIMIT 1) " +
      ")");
    cleanupStatements.push(deleteInvalidAttributeAnnos);

    
    let deleteOrphanAnnos = DBConn.createAsyncStatement(
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
      
      let createPlacesRoot = DBConn.createAsyncStatement(
        "INSERT INTO moz_bookmarks (id, type, fk, parent, position, title, "
      +                            "guid) "
      + "VALUES (:places_root, 2, NULL, 0, 0, :title, GENERATE_GUID())");
      createPlacesRoot.params["places_root"] = PlacesUtils.placesRootId;
      createPlacesRoot.params["title"] = "";
      cleanupStatements.push(createPlacesRoot);

      
      let fixPlacesRootChildren = DBConn.createAsyncStatement(
        "UPDATE moz_bookmarks SET parent = :places_root WHERE id IN " +
          "(SELECT folder_id FROM moz_bookmarks_roots " +
            "WHERE folder_id <> :places_root)");
      fixPlacesRootChildren.params["places_root"] = PlacesUtils.placesRootId;
      cleanupStatements.push(fixPlacesRootChildren);
    }
    selectPlacesRoot.finalize();

    
    
    
    let updateRootTitleSql = "UPDATE moz_bookmarks SET title = :title " +
                             "WHERE id = :root_id AND title <> :title";
    
    let fixPlacesRootTitle = DBConn.createAsyncStatement(updateRootTitleSql);
    fixPlacesRootTitle.params["root_id"] = PlacesUtils.placesRootId;
    fixPlacesRootTitle.params["title"] = "";
    cleanupStatements.push(fixPlacesRootTitle);
    
    let fixBookmarksMenuTitle = DBConn.createAsyncStatement(updateRootTitleSql);
    fixBookmarksMenuTitle.params["root_id"] = PlacesUtils.bookmarksMenuFolderId;
    fixBookmarksMenuTitle.params["title"] =
      PlacesUtils.getString("BookmarksMenuFolderTitle");
    cleanupStatements.push(fixBookmarksMenuTitle);
    
    let fixBookmarksToolbarTitle = DBConn.createAsyncStatement(updateRootTitleSql);
    fixBookmarksToolbarTitle.params["root_id"] = PlacesUtils.toolbarFolderId;
    fixBookmarksToolbarTitle.params["title"] =
      PlacesUtils.getString("BookmarksToolbarFolderTitle");
    cleanupStatements.push(fixBookmarksToolbarTitle);
    
    let fixUnsortedBookmarksTitle = DBConn.createAsyncStatement(updateRootTitleSql);
    fixUnsortedBookmarksTitle.params["root_id"] = PlacesUtils.unfiledBookmarksFolderId;
    fixUnsortedBookmarksTitle.params["title"] =
      PlacesUtils.getString("UnsortedBookmarksFolderTitle");
    cleanupStatements.push(fixUnsortedBookmarksTitle);
    
    let fixTagsRootTitle = DBConn.createAsyncStatement(updateRootTitleSql);
    fixTagsRootTitle.params["root_id"] = PlacesUtils.tagsFolderId;
    fixTagsRootTitle.params["title"] =
      PlacesUtils.getString("TagsFolderTitle");
    cleanupStatements.push(fixTagsRootTitle);

    
    
    
    let deleteNoPlaceItems = DBConn.createAsyncStatement(
      "DELETE FROM moz_bookmarks WHERE id NOT IN ( " +
        "SELECT folder_id FROM moz_bookmarks_roots " + 
      ") AND id IN (" +
        "SELECT b.id FROM moz_bookmarks b " +
        "WHERE fk NOT NULL AND b.type = :bookmark_type " +
          "AND NOT EXISTS (SELECT url FROM moz_places WHERE id = b.fk LIMIT 1) " +
      ")");
    deleteNoPlaceItems.params["bookmark_type"] = PlacesUtils.bookmarks.TYPE_BOOKMARK;
    cleanupStatements.push(deleteNoPlaceItems);

    
    let deleteBogusTagChildren = DBConn.createAsyncStatement(
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

    
    let deleteEmptyTags = DBConn.createAsyncStatement(
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

    
    let fixOrphanItems = DBConn.createAsyncStatement(
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

    
    let fixInvalidKeywords = DBConn.createAsyncStatement(
      "UPDATE moz_bookmarks SET keyword_id = NULL WHERE id NOT IN ( " +
        "SELECT folder_id FROM moz_bookmarks_roots " + 
      ") AND id IN ( " +
        "SELECT id FROM moz_bookmarks b " +
        "WHERE keyword_id NOT NULL " +
          "AND NOT EXISTS " +
            "(SELECT id FROM moz_keywords WHERE id = b.keyword_id LIMIT 1) " +
      ")");
    cleanupStatements.push(fixInvalidKeywords);

    
    
    
    
    let fixBookmarksAsFolders = DBConn.createAsyncStatement(
      "UPDATE moz_bookmarks SET type = :bookmark_type WHERE id NOT IN ( " +
        "SELECT folder_id FROM moz_bookmarks_roots " + 
      ") AND id IN ( " +
        "SELECT id FROM moz_bookmarks b " +
        "WHERE type IN (:folder_type, :separator_type) " +
          "AND fk NOTNULL " +
      ")");
    fixBookmarksAsFolders.params["bookmark_type"] = PlacesUtils.bookmarks.TYPE_BOOKMARK;
    fixBookmarksAsFolders.params["folder_type"] = PlacesUtils.bookmarks.TYPE_FOLDER;
    fixBookmarksAsFolders.params["separator_type"] = PlacesUtils.bookmarks.TYPE_SEPARATOR;
    cleanupStatements.push(fixBookmarksAsFolders);

    
    
    
    let fixFoldersAsBookmarks = DBConn.createAsyncStatement(
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

    
    
    
    let fixInvalidParents = DBConn.createAsyncStatement(
      "UPDATE moz_bookmarks SET parent = :unsorted_folder WHERE id NOT IN ( " +
        "SELECT folder_id FROM moz_bookmarks_roots " +  
      ") AND id IN ( " +
        "SELECT id FROM moz_bookmarks b " +
        "WHERE EXISTS " +
          "(SELECT id FROM moz_bookmarks WHERE id = b.parent " +
            "AND type IN (:bookmark_type, :separator_type) " +
            "LIMIT 1) " +
      ")");
    fixInvalidParents.params["unsorted_folder"] = PlacesUtils.unfiledBookmarksFolderId;
    fixInvalidParents.params["bookmark_type"] = PlacesUtils.bookmarks.TYPE_BOOKMARK;
    fixInvalidParents.params["separator_type"] = PlacesUtils.bookmarks.TYPE_SEPARATOR;
    cleanupStatements.push(fixInvalidParents);

    
    
    
    
    
    
    cleanupStatements.push(DBConn.createAsyncStatement(
      "CREATE TEMP TABLE IF NOT EXISTS moz_bm_reindex_temp ( " +
      "  id INTEGER PRIMARY_KEY " +
      ", parent INTEGER " +
      ", position INTEGER " +
      ") "
    ));
    cleanupStatements.push(DBConn.createAsyncStatement(
      "INSERT INTO moz_bm_reindex_temp " +
      "SELECT id, parent, 0 " +
      "FROM moz_bookmarks b " +
      "WHERE parent IN ( " +
        "SELECT parent " +
        "FROM moz_bookmarks " +
        "GROUP BY parent " +
        "HAVING (SUM(DISTINCT position + 1) - (count(*) * (count(*) + 1) / 2)) <> 0 " +
      ") " +
      "ORDER BY parent ASC, position ASC, ROWID ASC "
    ));
    cleanupStatements.push(DBConn.createAsyncStatement(
      "CREATE INDEX IF NOT EXISTS moz_bm_reindex_temp_index " +
      "ON moz_bm_reindex_temp(parent)"
    ));
    cleanupStatements.push(DBConn.createAsyncStatement(
      "UPDATE moz_bm_reindex_temp SET position = ( " +
        "ROWID - (SELECT MIN(t.ROWID) FROM moz_bm_reindex_temp t " +
                 "WHERE t.parent = moz_bm_reindex_temp.parent) " +
      ") "
    ));
    cleanupStatements.push(DBConn.createAsyncStatement(
      "CREATE TEMP TRIGGER IF NOT EXISTS moz_bm_reindex_temp_trigger " +
      "BEFORE DELETE ON moz_bm_reindex_temp " +
      "FOR EACH ROW " +
      "BEGIN " +
        "UPDATE moz_bookmarks SET position = OLD.position WHERE id = OLD.id; " +
      "END "
    ));
    cleanupStatements.push(DBConn.createAsyncStatement(
      "DELETE FROM moz_bm_reindex_temp "
    ));
    cleanupStatements.push(DBConn.createAsyncStatement(
      "DROP INDEX moz_bm_reindex_temp_index "
    ));
    cleanupStatements.push(DBConn.createAsyncStatement(
      "DROP TRIGGER moz_bm_reindex_temp_trigger "
    ));
    cleanupStatements.push(DBConn.createAsyncStatement(
      "DROP TABLE moz_bm_reindex_temp "
    ));

    
    
    
    let fixEmptyNamedTags = DBConn.createAsyncStatement(
      "UPDATE moz_bookmarks SET title = :empty_title " +
      "WHERE length(title) = 0 AND type = :folder_type " +
        "AND parent = :tags_folder"
    );
    fixEmptyNamedTags.params["empty_title"] = "(notitle)";
    fixEmptyNamedTags.params["folder_type"] = PlacesUtils.bookmarks.TYPE_FOLDER;
    fixEmptyNamedTags.params["tags_folder"] = PlacesUtils.tagsFolderId;
    cleanupStatements.push(fixEmptyNamedTags);

    
    
    let deleteOrphanIcons = DBConn.createAsyncStatement(
      "DELETE FROM moz_favicons WHERE id IN (" +
        "SELECT id FROM moz_favicons f " +
        "WHERE NOT EXISTS " +
          "(SELECT id FROM moz_places WHERE favicon_id = f.id LIMIT 1) " +
      ")");
    cleanupStatements.push(deleteOrphanIcons);

    
    
    let deleteOrphanVisits = DBConn.createAsyncStatement(
      "DELETE FROM moz_historyvisits WHERE id IN (" +
        "SELECT id FROM moz_historyvisits v " +
        "WHERE NOT EXISTS " +
          "(SELECT id FROM moz_places WHERE id = v.place_id LIMIT 1) " +
      ")");
    cleanupStatements.push(deleteOrphanVisits);

    
    
    let deleteOrphanInputHistory = DBConn.createAsyncStatement(
      "DELETE FROM moz_inputhistory WHERE place_id IN (" +
        "SELECT place_id FROM moz_inputhistory i " +
        "WHERE NOT EXISTS " +
          "(SELECT id FROM moz_places WHERE id = i.place_id LIMIT 1) " +
      ")");
    cleanupStatements.push(deleteOrphanInputHistory);

    
    
    let deleteInvalidAttributeItemsAnnos = DBConn.createAsyncStatement(
      "DELETE FROM moz_items_annos WHERE id IN ( " +
        "SELECT id FROM moz_items_annos t " +
        "WHERE NOT EXISTS " +
          "(SELECT id FROM moz_anno_attributes " +
            "WHERE id = t.anno_attribute_id LIMIT 1) " +
      ")");
    cleanupStatements.push(deleteInvalidAttributeItemsAnnos);

    
    let deleteOrphanItemsAnnos = DBConn.createAsyncStatement(
      "DELETE FROM moz_items_annos WHERE id IN ( " +
        "SELECT id FROM moz_items_annos t " +
        "WHERE NOT EXISTS " +
          "(SELECT id FROM moz_bookmarks WHERE id = t.item_id LIMIT 1) " +
      ")");
    cleanupStatements.push(deleteOrphanItemsAnnos);

    
    
    let deleteUnusedKeywords = DBConn.createAsyncStatement(
      "DELETE FROM moz_keywords WHERE id IN ( " +
        "SELECT id FROM moz_keywords k " +
        "WHERE NOT EXISTS " +
          "(SELECT id FROM moz_bookmarks WHERE keyword_id = k.id LIMIT 1) " +
      ")");
    cleanupStatements.push(deleteUnusedKeywords);

    
    
    let fixInvalidFaviconIds = DBConn.createAsyncStatement(
      "UPDATE moz_places SET favicon_id = NULL WHERE id IN ( " +
        "SELECT id FROM moz_places h " +
        "WHERE favicon_id NOT NULL " +
          "AND NOT EXISTS " +
            "(SELECT id FROM moz_favicons WHERE id = h.favicon_id LIMIT 1) " +
      ")");
    cleanupStatements.push(fixInvalidFaviconIds);

    
    let fixVisitStats = DBConn.createAsyncStatement(
      "UPDATE moz_places " +
      "SET visit_count = (SELECT count(*) FROM moz_historyvisits " +
                         "WHERE place_id = moz_places.id AND visit_type NOT IN (0,4,7,8)), " +
          "last_visit_date = (SELECT MAX(visit_date) FROM moz_historyvisits " +
                             "WHERE place_id = moz_places.id) " +
      "WHERE id IN ( " +
        "SELECT h.id FROM moz_places h " +
        "WHERE visit_count <> (SELECT count(*) FROM moz_historyvisits v " +
                              "WHERE v.place_id = h.id AND visit_type NOT IN (0,4,7,8)) " +
           "OR last_visit_date <> (SELECT MAX(visit_date) FROM moz_historyvisits v " +
                                  "WHERE v.place_id = h.id) " +
      ")");
    cleanupStatements.push(fixVisitStats);

    
    let fixRedirectsHidden = DBConn.createAsyncStatement(
      "UPDATE moz_places " +
      "SET hidden = 1 " +
      "WHERE id IN ( " +
        "SELECT h.id FROM moz_places h " +
        "JOIN moz_historyvisits src ON src.place_id = h.id " +
        "JOIN moz_historyvisits dst ON dst.from_visit = src.id AND dst.visit_type IN (5,6) " +
        "LEFT JOIN moz_bookmarks on fk = h.id AND fk ISNULL " +
        "GROUP BY src.place_id HAVING count(*) = visit_count " +
      ")");
    cleanupStatements.push(fixRedirectsHidden);

    

    return cleanupStatements;
  },

  





  vacuum: function PDBU_vacuum(aTasks)
  {
    let tasks = new Tasks(aTasks);
    tasks.log("> Vacuum");

    let DBFile = Services.dirsvc.get("ProfD", Ci.nsILocalFile);
    DBFile.append("places.sqlite");
    tasks.log("Initial database size is " +
              parseInt(DBFile.fileSize / 1024) + " KiB");

    let stmt = DBConn.createAsyncStatement("VACUUM");
    stmt.executeAsync({
      handleError: PlacesDBUtils._handleError,
      handleResult: function () {},

      handleCompletion: function (aReason)
      {
        if (aReason == Ci.mozIStorageStatementCallback.REASON_FINISHED) {
          tasks.log("+ The database has been vacuumed");
          let vacuumedDBFile = Services.dirsvc.get("ProfD", Ci.nsILocalFile);
          vacuumedDBFile.append("places.sqlite");
          tasks.log("Final database size is " +
                    parseInt(vacuumedDBFile.fileSize / 1024) + " KiB");
        }
        else {
          tasks.log("- Unable to vacuum database");
          tasks.clear();
        }

        PlacesDBUtils._executeTasks(tasks);
      }
    });
    stmt.finalize();
  },

  





  expire: function PDBU_expire(aTasks)
  {
    let tasks = new Tasks(aTasks);
    tasks.log("> Orphans expiration");

    let expiration = Cc["@mozilla.org/places/expiration;1"].
                     getService(Ci.nsIObserver);

    Services.obs.addObserver(function (aSubject, aTopic, aData) {
      Services.obs.removeObserver(arguments.callee, aTopic);
      tasks.log("+ Database cleaned up");
      PlacesDBUtils._executeTasks(tasks);
    }, PlacesUtils.TOPIC_EXPIRATION_FINISHED, false);

    
    expiration.observe(null, "places-debug-start-expiration", 0);
  },

  





  stats: function PDBU_stats(aTasks)
  {
    let tasks = new Tasks(aTasks);
    tasks.log("> Statistics");

    let DBFile = Services.dirsvc.get("ProfD", Ci.nsILocalFile);
    DBFile.append("places.sqlite");
    tasks.log("Database size is " + parseInt(DBFile.fileSize / 1024) + " KiB");

    [ "user_version"
    , "page_size"
    , "cache_size"
    , "journal_mode"
    , "synchronous"
    ].forEach(function (aPragma) {
      let stmt = DBConn.createStatement("PRAGMA " + aPragma);
      stmt.executeStep();
      tasks.log(aPragma + " is " + stmt.getString(0));
      stmt.finalize();
    });

    
    try {
      let limitURIs = Services.prefs.getIntPref(
        "places.history.expiration.transient_current_max_pages");
      tasks.log("History can store a maximum of " + limitURIs + " unique pages");
    } catch(ex) {}

    let stmt = DBConn.createStatement(
      "SELECT name FROM sqlite_master WHERE type = :type");
    stmt.params.type = "table";
    while (stmt.executeStep()) {
      let tableName = stmt.getString(0);
      let countStmt = DBConn.createStatement(
        "SELECT count(*) FROM " + tableName);
      countStmt.executeStep();
      tasks.log("Table " + tableName + " has " + countStmt.getInt32(0) + " records");
      countStmt.finalize();
    }
    stmt.reset();

    stmt.params.type = "index";
    while (stmt.executeStep()) {
      tasks.log("Index " + stmt.getString(0));
    }
    stmt.reset();

    stmt.params.type = "trigger";
    while (stmt.executeStep()) {
      tasks.log("Trigger " + stmt.getString(0));
    }
    stmt.finalize();

    PlacesDBUtils._executeTasks(tasks);
  },

  















  telemetry: function PDBU_telemetry(aTasks, aHealthReportCallback=null)
  {
    let tasks = new Tasks(aTasks);

    let isTelemetry = !aHealthReportCallback;

    
    
    let probeValues = {};

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    let probes = [
      { histogram: "PLACES_PAGES_COUNT",
        healthreport: true,
        query:     "SELECT count(*) FROM moz_places" },

      { histogram: "PLACES_BOOKMARKS_COUNT",
        healthreport: true,
        query:     "SELECT count(*) FROM moz_bookmarks b "
                 + "JOIN moz_bookmarks t ON t.id = b.parent "
                 + "AND t.parent <> :tags_folder "
                 + "WHERE b.type = :type_bookmark " },

      { histogram: "PLACES_TAGS_COUNT",
        query:     "SELECT count(*) FROM moz_bookmarks "
                 + "WHERE parent = :tags_folder " },

      { histogram: "PLACES_FOLDERS_COUNT",
        query:     "SELECT count(*) FROM moz_bookmarks "
                 + "WHERE TYPE = :type_folder "
                 + "AND parent NOT IN (0, :places_root, :tags_folder) " },

      { histogram: "PLACES_KEYWORDS_COUNT",
        query:     "SELECT count(*) FROM moz_keywords " },

      { histogram: "PLACES_SORTED_BOOKMARKS_PERC",
        query:     "SELECT IFNULL(ROUND(( "
                 +   "SELECT count(*) FROM moz_bookmarks b "
                 +   "JOIN moz_bookmarks t ON t.id = b.parent "
                 +   "AND t.parent <> :tags_folder AND t.parent > :places_root "
                 +   "WHERE b.type  = :type_bookmark "
                 +   ") * 100 / ( "
                 +   "SELECT count(*) FROM moz_bookmarks b "
                 +   "JOIN moz_bookmarks t ON t.id = b.parent "
                 +   "AND t.parent <> :tags_folder "
                 +   "WHERE b.type = :type_bookmark "
                 + ")), 0) " },

      { histogram: "PLACES_TAGGED_BOOKMARKS_PERC",
        query:     "SELECT IFNULL(ROUND(( "
                 +   "SELECT count(*) FROM moz_bookmarks b "
                 +   "JOIN moz_bookmarks t ON t.id = b.parent "
                 +   "AND t.parent = :tags_folder "
                 +   ") * 100 / ( "
                 +   "SELECT count(*) FROM moz_bookmarks b "
                 +   "JOIN moz_bookmarks t ON t.id = b.parent "
                 +   "AND t.parent <> :tags_folder "
                 +   "WHERE b.type = :type_bookmark "
                 + ")), 0) " },

      { histogram: "PLACES_DATABASE_FILESIZE_MB",
        callback: function () {
          let DBFile = Services.dirsvc.get("ProfD", Ci.nsILocalFile);
          DBFile.append("places.sqlite");
          return parseInt(DBFile.fileSize / BYTES_PER_MEBIBYTE);
        }
      },

      { histogram: "PLACES_DATABASE_JOURNALSIZE_MB",
        callback: function () {
          let DBFile = Services.dirsvc.get("ProfD", Ci.nsILocalFile);
          DBFile.append("places.sqlite-wal");
          return parseInt(DBFile.fileSize / BYTES_PER_MEBIBYTE);
        }
      },

      { histogram: "PLACES_DATABASE_PAGESIZE_B",
        query:     "PRAGMA page_size /* PlacesDBUtils.jsm PAGESIZE_B */" },

      { histogram: "PLACES_DATABASE_SIZE_PER_PAGE_B",
        query:     "PRAGMA page_count",
        callback: function (aDbPageCount) {
          
          
          let dbPageSize = probeValues.PLACES_DATABASE_PAGESIZE_B;
          let placesPageCount = probeValues.PLACES_PAGES_COUNT;
          return Math.round((dbPageSize * aDbPageCount) / placesPageCount);
        }
      },

      { histogram: "PLACES_ANNOS_BOOKMARKS_COUNT",
        query:     "SELECT count(*) FROM moz_items_annos" },

      
      
      
      { histogram: "PLACES_ANNOS_BOOKMARKS_SIZE_KB",
        query:     "SELECT SUM(LENGTH(content))/1024 FROM moz_items_annos" },

      { histogram: "PLACES_ANNOS_PAGES_COUNT",
        query:     "SELECT count(*) FROM moz_annos" },

      { histogram: "PLACES_ANNOS_PAGES_SIZE_KB",
        query:     "SELECT SUM(LENGTH(content))/1024 FROM moz_annos" },
    ];

    let params = {
      tags_folder: PlacesUtils.tagsFolderId,
      type_folder: PlacesUtils.bookmarks.TYPE_FOLDER,
      type_bookmark: PlacesUtils.bookmarks.TYPE_BOOKMARK,
      places_root: PlacesUtils.placesRootId
    };

    let outstandingProbes = 0;

    function reportResult(aProbe, aValue) {
      outstandingProbes--;

      let value = aValue;
      try {
        if ("callback" in aProbe) {
          value = aProbe.callback(value);
        }
        probeValues[aProbe.histogram] = value;
        Services.telemetry.getHistogramById(aProbe.histogram).add(value);
      } catch (ex) {
        Components.utils.reportError("Error adding value " + value +
                                     " to histogram " + aProbe.histogram +
                                     ": " + ex);
      }

      if (!outstandingProbes && aHealthReportCallback) {
        try {
          aHealthReportCallback(probeValues);
        } catch (ex) {
          Components.utils.reportError(ex);
        }
      }
    }

    for (let i = 0; i < probes.length; i++) {
      let probe = probes[i];

      if (!isTelemetry && !probe.healthreport) {
        continue;
      }

      outstandingProbes++;

      if (!("query" in probe)) {
        reportResult(probe);
        continue;
      }

      let stmt = DBConn.createAsyncStatement(probe.query);
      for (param in params) {
        if (probe.query.indexOf(":" + param) > 0) {
          stmt.params[param] = params[param];
        }
      }

      try {
        stmt.executeAsync({
          handleError: PlacesDBUtils._handleError,
          handleResult: function (aResultSet) {
            let row = aResultSet.getNextRow();
            reportResult(probe, row.getResultByIndex(0));
          },
          handleCompletion: function () {}
        });
      } finally{
        stmt.finalize();
      }
    }

    PlacesDBUtils._executeTasks(tasks);
  },

  









  runTasks: function PDBU_runTasks(aTasks, aCallback) {
    let tasks = new Tasks(aTasks);
    tasks.callback = aCallback;
    PlacesDBUtils._executeTasks(tasks);
  }
};







function Tasks(aTasks)
{
  if (aTasks) {
    if (Array.isArray(aTasks)) {
      this._list = aTasks.slice(0, aTasks.length);
    }
    
    
    else if (typeof(aTasks) == "object" &&
             (Tasks instanceof Tasks || "list" in aTasks)) {
      this._list = aTasks.list;
      this._log = aTasks.messages;
      this.callback = aTasks.callback;
      this.scope = aTasks.scope;
      this._telemetryStart = aTasks._telemetryStart;
    }
  }
}

Tasks.prototype = {
  _list: [],
  _log: [],
  callback: null,
  scope: null,
  _telemetryStart: 0,

  





  push: function T_push(aNewElt)
  {
    this._list.unshift(aNewElt);
  },

  




  pop: function T_pop() this._list.shift(),

  


  clear: function T_clear()
  {
    this._list.length = 0;
  },

  


  get list() this._list.slice(0, this._list.length),

  





  log: function T_log(aMsg)
  {
    this._log.push(aMsg);
  },

  


  get messages() this._log.slice(0, this._log.length),
}

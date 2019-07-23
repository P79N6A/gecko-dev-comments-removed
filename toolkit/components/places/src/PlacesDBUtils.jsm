






































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



const IDLE_LOOKUP_REPEAT = 30 * 60 * 1000;


const MAINTENANCE_REPEAT =  24 * 60 * 60;




function nsPlacesDBUtils() {
  
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
    Ci.nsIObserver,
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

















































    
    
    
    let deleteNoPlaceItems = this._dbConn.createStatement(
      "DELETE FROM moz_bookmarks WHERE id IN (" +
        "SELECT b.id FROM moz_bookmarks b " +
        "WHERE fk NOT NULL AND b.type = :bookmark_type " +
          "AND NOT EXISTS (SELECT url FROM moz_places_temp WHERE id = b.fk LIMIT 1) " +
          "AND NOT EXISTS (SELECT url FROM moz_places WHERE id = b.fk LIMIT 1) " +
      ")");
    deleteNoPlaceItems.params["bookmark_type"] = this._bms.TYPE_BOOKMARK;
    cleanupStatements.push(deleteNoPlaceItems);

    
    let deleteBogusTagChildren = this._dbConn.createStatement(
      "DELETE FROM moz_bookmarks WHERE id IN (" +
        "SELECT b.id FROM moz_bookmarks b " +
        "WHERE b.parent IN " +
          "(SELECT id FROM moz_bookmarks WHERE parent = :tags_folder) " +
          "AND b.type <> :bookmark_type " +
      ")");
    deleteBogusTagChildren.params["tags_folder"] = this._bms.tagsFolder;
    deleteBogusTagChildren.params["bookmark_type"] = this._bms.TYPE_BOOKMARK;
    cleanupStatements.push(deleteBogusTagChildren);

    
    let deleteEmptyTags = this._dbConn.createStatement(
      "DELETE FROM moz_bookmarks WHERE id IN (" +
        "SELECT b.id FROM moz_bookmarks b " +
        "WHERE b.id IN " +
          "(SELECT id FROM moz_bookmarks WHERE parent = :tags_folder) " +
          "AND NOT EXISTS " +
            "(SELECT id from moz_bookmarks WHERE parent = b.id LIMIT 1) " +
      ")");
    deleteEmptyTags.params["tags_folder"] = this._bms.tagsFolder;
    cleanupStatements.push(deleteEmptyTags);

    
    let fixOrphanItems = this._dbConn.createStatement(
      "UPDATE moz_bookmarks SET parent = :unsorted_folder WHERE id IN (" +
        "SELECT b.id FROM moz_bookmarks b " +
        "WHERE b.parent <> 0 " + 
        "AND NOT EXISTS " +
          "(SELECT id FROM moz_bookmarks WHERE id = b.parent LIMIT 1) " +
      ")");
    fixOrphanItems.params["unsorted_folder"] = this._bms.unfiledBookmarksFolder;
    cleanupStatements.push(fixOrphanItems);

    
    let fixInvalidKeywords = this._dbConn.createStatement(
      "UPDATE moz_bookmarks SET keyword_id = NULL WHERE id IN ( " +
        "SELECT id FROM moz_bookmarks b " +
        "WHERE keyword_id NOT NULL " +
          "AND NOT EXISTS " +
            "(SELECT id FROM moz_keywords WHERE id = b.keyword_id LIMIT 1) " +
      ")");
    cleanupStatements.push(fixInvalidKeywords);

    
    
    
    
    let fixBookmarksAsFolders = this._dbConn.createStatement(
      "UPDATE moz_bookmarks SET type = :bookmark_type WHERE id IN ( " +
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
      "UPDATE moz_bookmarks SET type = :folder_type WHERE id IN ( " +
        "SELECT id FROM moz_bookmarks b " +
        "WHERE type = :bookmark_type " +
          "AND fk IS NULL " +
      ")");
    fixFoldersAsBookmarks.params["bookmark_type"] = this._bms.TYPE_BOOKMARK;
    fixFoldersAsBookmarks.params["folder_type"] = this._bms.TYPE_FOLDER;
    cleanupStatements.push(fixFoldersAsBookmarks);

    
    
    
    let fixFoldersAsDynamic = this._dbConn.createStatement(
      "UPDATE moz_bookmarks SET type = :folder_type WHERE id IN ( " +
        "SELECT id FROM moz_bookmarks b " +
        "WHERE type = :dynamic_type " +
          "AND folder_type IS NULL " +
      ")");
    fixFoldersAsDynamic.params["dynamic_type"] = this._bms.TYPE_DYNAMIC_CONTAINER;
    fixFoldersAsDynamic.params["folder_type"] = this._bms.TYPE_FOLDER;
    cleanupStatements.push(fixFoldersAsDynamic);

    
    
    
    let fixInvalidParents = this._dbConn.createStatement(
      "UPDATE moz_bookmarks SET parent = :unsorted_folder WHERE id IN ( " +
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
      "DELETE FROM moz_bookmarks WHERE fk IN ( " +
        "SELECT id FROM moz_places WHERE url = :lmloading OR url = :lmfailed " +
      ")");
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
};




nsPlacesDBUtils.prototype.__defineGetter__("_bms", function() {
  delete nsPlacesDBUtils._bms;
  return nsPlacesDBUtils._bms = Cc[BS_CONTRACTID].getService(Ci.nsINavBookmarksService);
});

nsPlacesDBUtils.prototype.__defineGetter__("_hs", function() {
  delete nsPlacesDBUtils._hs;
  return nsPlacesDBUtils._hs = Cc[HS_CONTRACTID].getService(Ci.nsINavHistoryService);
});

nsPlacesDBUtils.prototype.__defineGetter__("_os", function() {
  delete nsPlacesDBUtils._os;
  return nsPlacesDBUtils._os = Cc[OS_CONTRACTID].getService(Ci.nsIObserverService);
});

nsPlacesDBUtils.prototype.__defineGetter__("_idlesvc", function() {
  delete nsPlacesDBUtils._idlesvc;
  return nsPlacesDBUtils._idlesvc = Cc[IS_CONTRACTID].getService(Ci.nsIIdleService);
});

nsPlacesDBUtils.prototype.__defineGetter__("_dbConn", function() {
  delete nsPlacesDBUtils._dbConn;
  return nsPlacesDBUtils._dbConn = Cc[HS_CONTRACTID].
                        getService(Ci.nsPIPlacesDatabase).DBConnection;
});

nsPlacesDBUtils.prototype.__defineGetter__("_bundle", function() {
  delete nsPlacesDBUtils._bundle;
  return nsPlacesDBUtils._bundle = Cc[SB_CONTRACTID].
                                   getService(Ci.nsIStringBundleService).
                                   createBundle(PLACES_STRING_BUNDLE_URI);
});

__defineGetter__("PlacesDBUtils", function() {
  delete this.PlacesDBUtils;
  return this.PlacesDBUtils = new nsPlacesDBUtils;
});

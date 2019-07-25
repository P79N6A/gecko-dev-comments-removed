









































const Cc = Components.classes;
const Ci = Components.interfaces;


const TOPIC_GATHER_TELEMETRY = "gather-telemetry";


const MAINTENANCE_INTERVAL_SECONDS = 7 * 86400;




Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/PlacesUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesDBUtils",
                                  "resource://gre/modules/PlacesDBUtils.jsm");





function PlacesCategoriesStarter()
{
  Services.obs.addObserver(this, TOPIC_GATHER_TELEMETRY, false);
  Services.obs.addObserver(this, PlacesUtils.TOPIC_SHUTDOWN, false);

  
  let notify = (function () {
    if (!this._notifiedBookmarksSvcReady) {
      
      
      Cc["@mozilla.org/categorymanager;1"]
        .getService(Ci.nsICategoryManager)
        .deleteCategoryEntry("bookmarks-observer", this, false);
      Services.obs.notifyObservers(null, "bookmarks-service-ready", null);
    }
  }).bind(this);
  [ "onItemAdded", "onItemRemoved", "onItemChanged", "onBeginUpdateBatch",
    "onEndUpdateBatch", "onBeforeItemRemoved", "onItemVisited",
    "onItemMoved" ].forEach(function(aMethod) {
      this[aMethod] = notify;
    }, this);
}

PlacesCategoriesStarter.prototype = {
  
  

  observe: function PCS_observe(aSubject, aTopic, aData)
  {
    switch (aTopic) {
      case PlacesUtils.TOPIC_SHUTDOWN:
        Services.obs.removeObserver(this, PlacesUtils.TOPIC_SHUTDOWN);
        Services.obs.removeObserver(this, TOPIC_GATHER_TELEMETRY);
        break;
      case TOPIC_GATHER_TELEMETRY:
        PlacesDBUtils.telemetry();
        break;
      case "idle-daily":
        
        let lastMaintenance = 0;
        try {
          lastMaintenance =
            Services.prefs.getIntPref("places.database.lastMaintenance");
        } catch (ex) {}
        let nowSeconds = parseInt(Date.now() / 1000);
        if (lastMaintenance < nowSeconds - MAINTENANCE_INTERVAL_SECONDS) {
          PlacesDBUtils.maintenanceOnIdle();
        }
        break;
      default:
        throw new Error("Trying to handle an unknown category.");
    }
  },

  
  

  classID: Components.ID("803938d5-e26d-4453-bf46-ad4b26e41114"),

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIObserver
  , Ci.nsINavBookmarkObserver
  ])
};




let components = [PlacesCategoriesStarter];
var NSGetFactory = XPCOMUtils.generateNSGetFactory(components);

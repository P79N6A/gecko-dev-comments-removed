









































const Cc = Components.classes;
const Ci = Components.interfaces;


const MAINTENANCE_INTERVAL_SECONDS = 7 * 86400;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");





function PlacesCategoriesStarter()
{
}

PlacesCategoriesStarter.prototype = {
  
  

  observe: function PCS_observe(aSubject, aTopic, aData)
  {
    switch (aTopic) {
      case "idle-daily":
        
        let lastMaintenance = 0;
        try {
          lastMaintenance = Services.prefs.getIntPref("places.database.lastMaintenance");
        } catch (ex) {}
        let nowSeconds = parseInt(Date.now() / 1000);
        if (lastMaintenance < nowSeconds - MAINTENANCE_INTERVAL_SECONDS) {
          Components.utils.import("resource://gre/modules/PlacesDBUtils.jsm");
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
  ])
};




let components = [PlacesCategoriesStarter];
var NSGetFactory = XPCOMUtils.generateNSGetFactory(components);

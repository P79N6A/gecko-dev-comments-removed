









































const Cc = Components.classes;
const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");





function PlacesCategoriesStarter()
{
}

PlacesCategoriesStarter.prototype = {
  
  

  observe: function PCS_observe(aSubject, aTopic, aData)
  {
    switch (aTopic) {
      case "idle-daily":
        
        Components.utils.import("resource://gre/modules/PlacesDBUtils.jsm");
        PlacesDBUtils.maintenanceOnIdle();
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

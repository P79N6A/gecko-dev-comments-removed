






































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

var EXPORTED_SYMBOLS = [ "PlacesBackground" ];




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const kQuitApplication = "quit-application";
const kPlacesBackgroundShutdown = "places-background-shutdown";




function nsPlacesBackground()
{
  let tm = Cc["@mozilla.org/thread-manager;1"].
           getService(Ci.nsIThreadManager);
  this._thread = tm.newThread(0);

  let os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  os.addObserver(this, kQuitApplication, false);
}

nsPlacesBackground.prototype = {
  
  

  dispatch: function PlacesBackground_dispatch(aEvent, aFlags)
  {
    this._thread.dispatch(aEvent, aFlags);
  },

  isOnCurrentThread: function PlacesBackground_isOnCurrentThread()
  {
    return this._thread.isOnCurrentThread();
  },

  
  

  observe: function PlacesBackground_observe(aSubject, aTopic, aData)
  {
    if (aTopic == kQuitApplication) {
      
      let os = Cc["@mozilla.org/observer-service;1"].
               getService(Ci.nsIObserverService);
      os.notifyObservers(null, kPlacesBackgroundShutdown, null);

      
      this._thread.shutdown();
      this._thread = null;
    }
  },

  
  

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIEventTarget,
    Ci.nsIObserver,
  ])
};


__defineGetter__("PlacesBackground", function() {
  delete this.PlacesBackground;
  return this.PlacesBackground = new nsPlacesBackground;
});

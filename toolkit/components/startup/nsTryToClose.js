




































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

function TryToClose() {}

TryToClose.prototype = {
  observe: function (aSubject, aTopic, aData) {
    switch (aTopic) {
    case "app-startup":
      var obsService = Cc["@mozilla.org/observer-service;1"].
                       getService(Ci.nsIObserverService);
      obsService.addObserver(this, "quit-application-requested", true);
      break;
    case "quit-application-requested":
      var windowMediator = Cc['@mozilla.org/appshell/window-mediator;1'].
                           getService(Ci.nsIWindowMediator);
      var enumerator = windowMediator.getEnumerator(null);
      while (enumerator.hasMoreElements()) {
        var domWindow = enumerator.getNext();
        if (("tryToClose" in domWindow) && !domWindow.tryToClose()) {
          aSubject.QueryInterface(Ci.nsISupportsPRBool);
          aSubject.data = true;
          break;
        }
      }
      break;
    }
  },

  classID: Components.ID("{b69155f4-a8bf-453d-8653-91d1456e1d3d}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),
};

var NSGetFactory = XPCOMUtils.generateNSGetFactory([TryToClose]);

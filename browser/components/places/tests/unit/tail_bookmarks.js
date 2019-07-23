








































var os = Cc['@mozilla.org/observer-service;1'].
         getService(Ci.nsIObserverService);
os.notifyObservers(null, "quit-application-granted", null);
os.notifyObservers(null, "quit-application", null);

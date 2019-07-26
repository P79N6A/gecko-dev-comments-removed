



const nsIQuotaManager = Components.interfaces.nsIQuotaManager;

let gURI = Components.classes["@mozilla.org/network/io-service;1"].getService(Components.interfaces.nsIIOService).newURI("http://localhost", null, null);

function onUsageCallback(uri, usage, fileUsage) {}

function onLoad()
{
  var quotaManager = Components.classes["@mozilla.org/dom/quota/manager;1"]
                               .getService(nsIQuotaManager);
  var quotaRequest = quotaManager.getUsageForURI(gURI, onUsageCallback);
  quotaRequest.cancel();
  Components.classes["@mozilla.org/observer-service;1"]
            .getService(Components.interfaces.nsIObserverService)
            .notifyObservers(window, "bug839193-loaded", null);
}

function onUnload()
{
  Components.classes["@mozilla.org/observer-service;1"]
            .getService(Components.interfaces.nsIObserverService)
            .notifyObservers(window, "bug839193-unloaded", null);
}

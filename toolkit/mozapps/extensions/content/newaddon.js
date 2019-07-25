




































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/AddonManager.jsm");

var gAddon = null;


var EnableListener = {
  onEnabling: function(aAddon) {
    if (aAddon.id == gAddon.id)
      window.close();
  }
}
AddonManager.addAddonListener(EnableListener);

function initialize() {
  
  
  let spec = document.location.href;
  let pos = spec.indexOf("?");
  let query = "";
  if (pos >= 0)
    query = spec.substring(pos + 1);

  let bundle = Services.strings.createBundle("chrome://mozapps/locale/extensions/newaddon.properties");

  
  let id = query.substring(3);
  if (!id) {
    window.close();
    return;
  }

  AddonManager.getAddonByID(id, function(aAddon) {
    
    
    
    if (!aAddon || !aAddon.userDisabled ||
        !(aAddon.permissions & AddonManager.PERM_CAN_ENABLE)) {
      window.close();
      return;
    }

    gAddon = aAddon;

    document.getElementById("addon-info").setAttribute("type", aAddon.type);
    
    let icon = document.getElementById("icon");
    if (aAddon.icon64URL)
      icon.src = aAddon.icon64URL;
    else if (aAddon.iconURL)
      icon.src = aAddon.iconURL;

    let name = bundle.formatStringFromName("name", [aAddon.name, aAddon.version],
                                           2);
    document.getElementById("name").value = name

    if (aAddon.creator) {
      let creator = bundle.formatStringFromName("author", [aAddon.creator], 1);
      document.getElementById("author").value = creator;
    } else {
      document.getElementById("author").hidden = true;
    }

    let uri = "getResourceURI" in aAddon ? aAddon.getResourceURI() : null;
    let locationLabel = document.getElementById("location");
    if (uri instanceof Ci.nsIFileURL) {
      let location = bundle.formatStringFromName("location", [uri.file.path], 1);
      locationLabel.value = location;
      locationLabel.setAttribute("tooltiptext", location);
    } else {
      document.getElementById("location").hidden = true;
    }

    var event = document.createEvent("Events");
    event.initEvent("AddonDisplayed", true, true);
    document.dispatchEvent(event);
  });
}

function unload() {
  AddonManager.removeAddonListener(EnableListener);
}

function continueClicked() {
  AddonManager.removeAddonListener(EnableListener);

  if (document.getElementById("allow").checked) {
    gAddon.userDisabled = false;

    if (gAddon.pendingOperations & AddonManager.PENDING_ENABLE) {
      document.getElementById("allow").disabled = true;
      document.getElementById("buttonDeck").selectedPanel = document.getElementById("restartPanel");
      return;
    }
  }

  window.close();
}

function restartClicked() {
  let cancelQuit = Cc["@mozilla.org/supports-PRBool;1"].
                   createInstance(Ci.nsISupportsPRBool);
  Services.obs.notifyObservers(cancelQuit, "quit-application-requested",
                               "restart");
  if (cancelQuit.data)
    return; 

  window.close();

  let appStartup = Components.classes["@mozilla.org/toolkit/app-startup;1"].
                   getService(Components.interfaces.nsIAppStartup);
  appStartup.quit(Ci.nsIAppStartup.eAttemptQuit |  Ci.nsIAppStartup.eRestart);
}

function cancelClicked() {
  gAddon.userDisabled = true;
  AddonManager.addAddonListener(EnableListener);

  document.getElementById("allow").disabled = false;
  document.getElementById("buttonDeck").selectedPanel = document.getElementById("continuePanel");
}

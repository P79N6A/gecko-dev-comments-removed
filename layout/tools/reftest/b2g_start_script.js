



function setDefaultPrefs() {
    
    var prefs = Cc["@mozilla.org/preferences-service;1"].
                getService(Ci.nsIPrefService);
    var branch = prefs.getDefaultBranch("");

#include reftest-preferences.js
}

function setPermissions() {
  if (__marionetteParams.length < 2) {
    return;
  }

  let serverAddr = __marionetteParams[0];
  let serverPort = __marionetteParams[1];
  let perms = Cc["@mozilla.org/permissionmanager;1"]
              .getService(Ci.nsIPermissionManager);
  let ioService = Cc["@mozilla.org/network/io-service;1"]
                  .getService(Ci.nsIIOService);
  let uri = ioService.newURI("http://" + serverAddr + ":" + serverPort, null, null);
  perms.add(uri, "allowXULXBL", Ci.nsIPermissionManager.ALLOW_ACTION);
}


let wm = Cc["@mozilla.org/appshell/window-mediator;1"]
            .getService(Ci.nsIWindowMediator);
let win = wm.getMostRecentWindow('');


setDefaultPrefs();
setPermissions();



let reftest = {};
Cu.import("chrome://reftest/content/reftest.jsm", reftest);


reftest.OnRefTestLoad(win);

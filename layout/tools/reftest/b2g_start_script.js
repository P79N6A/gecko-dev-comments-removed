args = __marionetteParams;

function setDefaultPrefs() {
    
    
    
    
    
    var prefs = Components.classes["@mozilla.org/preferences-service;1"].
                getService(Components.interfaces.nsIPrefService);
    var branch = prefs.getDefaultBranch("");
    branch.setBoolPref("dom.use_xbl_scopes_for_remote_xul", true);
    branch.setBoolPref("gfx.color_management.force_srgb", true);
    branch.setBoolPref("browser.dom.window.dump.enabled", true);
    branch.setIntPref("ui.caretBlinkTime", -1);
    branch.setBoolPref("dom.send_after_paint_to_content", true);
    
    branch.setIntPref("dom.max_script_run_time", 0);
    branch.setIntPref("dom.max_chrome_script_run_time", 0);
    branch.setIntPref("hangmonitor.timeout", 0);
    
    branch.setBoolPref("media.autoplay.enabled", true);
    
    branch.setBoolPref("app.update.enabled", false);
}

function setPermissions(webserver, port) {
  var perms = Components.classes["@mozilla.org/permissionmanager;1"]
              .getService(Components.interfaces.nsIPermissionManager);
  var ioService = Components.classes["@mozilla.org/network/io-service;1"]
                  .getService(Components.interfaces.nsIIOService);
  var uri = ioService.newURI("http://" + webserver + ":" + port, null, null);
  perms.add(uri, "allowXULXBL", Components.interfaces.nsIPermissionManager.ALLOW_ACTION);
}


let wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                   .getService(Components.interfaces.nsIWindowMediator);
let win = wm.getMostRecentWindow('');


setDefaultPrefs();
setPermissions(args[0], args[1]);


Components.utils.import("chrome://reftest/content/reftest.jsm");
OnRefTestLoad(win);

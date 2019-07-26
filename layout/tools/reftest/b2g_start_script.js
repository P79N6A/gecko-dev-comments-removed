



let serverAddr = __marionetteParams[0];
let serverPort = __marionetteParams[1];

function setDefaultPrefs() {
    
    
    
    
    
    var prefs = Cc["@mozilla.org/preferences-service;1"].
                getService(Ci.nsIPrefService);
    var branch = prefs.getDefaultBranch("");
    branch.setBoolPref("dom.use_xbl_scopes_for_remote_xul", false);
    branch.setBoolPref("gfx.color_management.force_srgb", true);
    branch.setBoolPref("browser.dom.window.dump.enabled", true);
    branch.setIntPref("ui.caretBlinkTime", -1);
    branch.setBoolPref("dom.send_after_paint_to_content", true);
    
    branch.setIntPref("dom.max_script_run_time", 0);
    branch.setIntPref("dom.max_chrome_script_run_time", 0);
    branch.setIntPref("hangmonitor.timeout", 0);
    
    branch.setBoolPref("media.autoplay.enabled", true);
    
    branch.setBoolPref("app.update.enabled", false);
    
    branch.setBoolPref("extensions.update.enabled", false);
    branch.setBoolPref("extensions.getAddons.cache.enabled", false);
    
    branch.setBoolPref("extensions.blocklist.enabled", false);
    
    branch.setIntPref("urlclassifier.updateinterval", 172800);
    
    branch.setBoolPref("image.high_quality_downscaling.enabled", false);
    
    
    branch.setBoolPref("security.fileuri.strict_origin_policy", false);
    
    branch.setBoolPref("browser.pagethumbnails.capturing_disabled", true);
}

function setPermissions() {
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








function test() {
  waitForExplicitFinish();
  registerCleanupFunction(function() {
    
    Services.prefs.clearUserPref("browser.preferences.inContent");
  });
  
  
  
  Services.prefs.setBoolPref("browser.preferences.inContent", true);
    
  gBrowser.tabContainer.addEventListener("TabOpen", function(aEvent) {
    
    gBrowser.tabContainer.removeEventListener("TabOpen", arguments.callee, true);
    let browser = aEvent.originalTarget.linkedBrowser;
    browser.addEventListener("load", function(aEvent) {
      browser.removeEventListener("load", arguments.callee, true);
      
      is(Services.prefs.getBoolPref("browser.preferences.inContent"), true, "In-content prefs are enabled");
      is(browser.contentWindow.location.href, "about:preferences", "Checking if the preferences tab was opened");
      
      gBrowser.removeCurrentTab();
      Services.prefs.setBoolPref("browser.preferences.inContent", false);
      openPreferences();
      
    }, true);
  }, true);
  
  
  let observer = {
    observe: function(aSubject, aTopic, aData) {
      if (aTopic == "domwindowopened") {
        windowWatcher.unregisterNotification(observer);
        
        let win = aSubject.QueryInterface(Components.interfaces.nsIDOMWindow);
        win.addEventListener("load", function() {
          win.removeEventListener("load", arguments.callee, false);
          is(Services.prefs.getBoolPref("browser.preferences.inContent"), false, "In-content prefs are disabled");
          is(win.location.href, "chrome://browser/content/preferences/preferences.xul", "Checking if the preferences window was opened");
          win.close();
          finish();
        }, false);
      }
    }
  }
  
  var windowWatcher = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                        .getService(Components.interfaces.nsIWindowWatcher);
  windowWatcher.registerNotification(observer);
  
  openPreferences();
}









const URL = "http://mochi.test:8888/browser/browser/base/content/test/general/offlineQuotaNotification.html";

registerCleanupFunction(function() {
  
  let uri = Services.io.newURI(URL, null, null);
  var principal = Services.scriptSecurityManager.getNoAppCodebasePrincipal(uri);
  Services.perms.removeFromPrincipal(principal, "offline-app");
  Services.prefs.clearUserPref("offline-apps.quota.warn");
  Services.prefs.clearUserPref("offline-apps.allow_by_default");
});




function checkPreferences(prefsWin) {
  
  
  prefsWin.addEventListener("paneload", function paneload(evt) {
    prefsWin.removeEventListener("paneload", paneload);
    is(evt.target.id, "paneAdvanced", "advanced pane loaded");
    let tabPanels = evt.target.getElementsByTagName("tabpanels")[0];
    tabPanels.addEventListener("select", function tabselect() {
      tabPanels.removeEventListener("select", tabselect);
      is(tabPanels.selectedPanel.id, "networkPanel", "networkPanel is selected");
      
      prefsWin.close();
      finish();
    });
  });
}

function checkInContentPreferences(win) {
  let sel = win.history.state;
  let doc = win.document;
  let tab = doc.getElementById("advancedPrefs").selectedTab.id;
  is(gBrowser.currentURI.spec, "about:preferences", "about:preferences loaded");
  is(sel, "paneAdvanced", "Advanced pane was selected");
  is(tab, "networkTab", "Network tab is selected");
  
  win.close();
  finish();
}

function test() {
  waitForExplicitFinish();
  gBrowser.selectedBrowser.addEventListener("load", function onload() {
    gBrowser.selectedBrowser.removeEventListener("load", onload, true);
    gBrowser.selectedBrowser.contentWindow.applicationCache.oncached = function() {
      executeSoon(function() {
        
        let notification = PopupNotifications.getNotification('offline-app-usage');
        ok(notification, "have offline-app-usage notification");
        
        
        
        
        if (!Services.prefs.getBoolPref("browser.preferences.inContent")) {
          Services.ww.registerNotification(function wwobserver(aSubject, aTopic, aData) {
            if (aTopic != "domwindowopened")
              return;
            Services.ww.unregisterNotification(wwobserver);
            checkPreferences(aSubject);
          });
        }
        PopupNotifications.panel.firstElementChild.button.click();
        if (Services.prefs.getBoolPref("browser.preferences.inContent")) {
          let newTabBrowser = gBrowser.getBrowserForTab(gBrowser.selectedTab);
          newTabBrowser.addEventListener("Initialized", function PrefInit() {
            newTabBrowser.removeEventListener("Initialized", PrefInit, true);
            checkInContentPreferences(newTabBrowser.contentWindow);
          }, true);
        }
      });
    };
    Services.prefs.setIntPref("offline-apps.quota.warn", 1);

    
    
    PopupNotifications.panel.firstElementChild.button.click();
  }, true);

  Services.prefs.setBoolPref("offline-apps.allow_by_default", false);
  gBrowser.contentWindow.location = URL;
}

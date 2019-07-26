







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

function test() {
  waitForExplicitFinish();
  gBrowser.selectedBrowser.addEventListener("load", function onload() {
    gBrowser.selectedBrowser.removeEventListener("load", onload, true);
    gBrowser.selectedBrowser.contentWindow.applicationCache.oncached = function() {
      executeSoon(function() {
        
        let notification = PopupNotifications.getNotification('offline-app-usage');
        ok(notification, "have offline-app-usage notification");
        
        
        
        
        if (Services.prefs.getBoolPref("browser.preferences.inContent")) {
          
          todo(false, "Bug 881576 - this test needs to be updated for inContent prefs");
        } else {
          Services.ww.registerNotification(function wwobserver(aSubject, aTopic, aData) {
            if (aTopic != "domwindowopened")
              return;
            Services.ww.unregisterNotification(wwobserver);
            checkPreferences(aSubject);
          });
          PopupNotifications.panel.firstElementChild.button.click();
        }
      });
    };
    Services.prefs.setIntPref("offline-apps.quota.warn", 1);

    
    
    PopupNotifications.panel.firstElementChild.button.click();
  }, true);

  Services.prefs.setBoolPref("offline-apps.allow_by_default", false);
  gBrowser.contentWindow.location = URL;
}

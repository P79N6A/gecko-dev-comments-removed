




Components.utils.import("resource://gre/modules/Services.jsm");

function test() {
  waitForExplicitFinish();

  
  
  let oldNetworkProxyType = Services.prefs.getIntPref("network.proxy.type");
  registerCleanupFunction(function() {
    Services.prefs.setIntPref("network.proxy.type", oldNetworkProxyType);
    Services.prefs.clearUserPref("browser.preferences.instantApply");
    Services.prefs.clearUserPref("network.proxy.share_proxy_settings");
    for (let proxyType of ["http", "ssl", "ftp", "socks"]) {
      Services.prefs.clearUserPref("network.proxy." + proxyType);
      Services.prefs.clearUserPref("network.proxy." + proxyType + "_port");
      if (proxyType == "http") {
        continue;
      }
      Services.prefs.clearUserPref("network.proxy.backup." + proxyType);
      Services.prefs.clearUserPref("network.proxy.backup." + proxyType + "_port");
    }
  });

  let connectionURL = "chrome://browser/content/preferences/connection.xul";
  let windowWatcher = Services.ww;

  
  
  Services.prefs.setBoolPref("browser.preferences.instantApply", true);

  
  Services.prefs.setIntPref("network.proxy.type", 1);
  Services.prefs.setBoolPref("network.proxy.share_proxy_settings", true);
  Services.prefs.setCharPref("network.proxy.http", "example.com");
  Services.prefs.setIntPref("network.proxy.http_port", 1200);
  Services.prefs.setCharPref("network.proxy.socks", "example.com");
  Services.prefs.setIntPref("network.proxy.socks_port", 1200);
  Services.prefs.setCharPref("network.proxy.backup.socks", "127.0.0.1");
  Services.prefs.setIntPref("network.proxy.backup.socks_port", 9050);

  
  let observer = {
    observe: function(aSubject, aTopic, aData) {
      if (aTopic == "domwindowopened") {
        
        let win = aSubject.QueryInterface(Components.interfaces.nsIDOMWindow);
        win.addEventListener("load", function winLoadListener() {
          win.removeEventListener("load", winLoadListener, false);
          if (win.location.href == connectionURL) {
            ok(true, "connection window opened");
            win.document.documentElement.acceptDialog();
          }
        }, false);
      } else if (aTopic == "domwindowclosed") {
        
        let win = aSubject.QueryInterface(Components.interfaces.nsIDOMWindow);
        if (win.location.href == connectionURL) {
          windowWatcher.unregisterNotification(observer);
          ok(true, "connection window closed");

          
          is(Services.prefs.getCharPref("network.proxy.backup.socks"), "127.0.0.1", "Shared proxy backup shouldn't be replaced");
          is(Services.prefs.getIntPref("network.proxy.backup.socks_port"), 9050, "Shared proxy port backup shouldn't be replaced");

          gBrowser.removeCurrentTab();
          finish();
        }
      }
    }
  };

  




  open_preferences(function tabOpened(aContentWindow) {
    is(gBrowser.currentURI.spec, "about:preferences", "about:preferences loaded");
    windowWatcher.registerNotification(observer);
    gBrowser.contentWindow.gAdvancedPane.showConnections();
  });
}


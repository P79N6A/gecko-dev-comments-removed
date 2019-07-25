




Components.utils.import("resource://gre/modules/Services.jsm");

function test() {
  waitForExplicitFinish();
  
  
  
  let oldNetworkProxyType = Services.prefs.getIntPref("network.proxy.type");
  registerCleanupFunction(function() {
    Services.prefs.setIntPref("network.proxy.type", oldNetworkProxyType);
    Services.prefs.clearUserPref("network.proxy.no_proxies_on");
    Services.prefs.clearUserPref("browser.preferences.instantApply");
  });
  
  let connectionURI = "chrome://browser/content/preferences/connection.xul";
  let windowWatcher = Cc["@mozilla.org/embedcomp/window-watcher;1"]
                          .getService(Components.interfaces.nsIWindowWatcher);

  
  
  Services.prefs.setBoolPref("browser.preferences.instantApply", true);

  
  let observer = {
    observe: function(aSubject, aTopic, aData) {
      
      if (aTopic == "domwindowopened") {
        
        let win = aSubject.QueryInterface(Components.interfaces.nsIDOMWindow);
        win.addEventListener("load", function winLoadListener() {
          win.removeEventListener("load", winLoadListener, false);
          if (win.location.href == connectionURI) {
            ok(true, "connection window opened");
            runConnectionTests(win);
            win.document.documentElement.acceptDialog();
          }
        }, false);
      } else if (aTopic == "domwindowclosed") {
        
        let win = aSubject.QueryInterface(Components.interfaces.nsIDOMWindow);
        if (win.location.href == connectionURI) {
          windowWatcher.unregisterNotification(observer);
          ok(true, "connection window closed");
          
          
          is(Services.prefs.getCharPref("network.proxy.no_proxies_on"),
             ".a.com,.b.com,.c.com", "no_proxies_on pref has correct value");
          gBrowser.removeCurrentTab();
          finish();
        }
      }

    }
  }

  




  open_preferences(function tabOpened(aContentWindow) {
    is(gBrowser.currentURI.spec, "about:preferences", "about:preferences loaded");
    windowWatcher.registerNotification(observer);
    gBrowser.contentWindow.gAdvancedPane.showConnections();
  });
}


function runConnectionTests(win) {
  let doc = win.document;
  let networkProxyNone = doc.getElementById("networkProxyNone");
  let networkProxyNonePref = doc.getElementById("network.proxy.no_proxies_on");
  let networkProxyTypePref = doc.getElementById("network.proxy.type");

  
  is(networkProxyNone.getAttribute("multiline"), "true",
     "networkProxyNone textbox is multiline");
  is(networkProxyNone.getAttribute("rows"), "2",
     "networkProxyNone textbox has two rows");
  
  
  
  function testSanitize(input, expected, errorMessage) {
    networkProxyNonePref.value = input;
    win.gConnectionsDialog.sanitizeNoProxiesPref();
    is(networkProxyNonePref.value, expected, errorMessage);
  }

  
  networkProxyTypePref.value = 1;
  is(networkProxyNone.disabled, false, "networkProxyNone textbox is enabled");

  testSanitize(".a.com", ".a.com",
               "sanitize doesn't mess up single filter");
  testSanitize(".a.com, .b.com, .c.com", ".a.com, .b.com, .c.com",
               "sanitize doesn't mess up multiple comma/space sep filters");
  testSanitize(".a.com\n.b.com", ".a.com,.b.com",
               "sanitize turns line break into comma");
  testSanitize(".a.com,\n.b.com", ".a.com,.b.com",
               "sanitize doesn't add duplicate comma after comma");
  testSanitize(".a.com\n,.b.com", ".a.com,.b.com",
               "sanitize doesn't add duplicate comma before comma");
  testSanitize(".a.com,\n,.b.com", ".a.com,,.b.com",
               "sanitize doesn't add duplicate comma surrounded by commas");
  testSanitize(".a.com, \n.b.com", ".a.com, .b.com",
               "sanitize doesn't add comma after comma/space");
  testSanitize(".a.com\n .b.com", ".a.com, .b.com",
               "sanitize adds comma before space");
  testSanitize(".a.com\n\n\n;;\n;\n.b.com", ".a.com,.b.com",
               "sanitize only adds one comma per substring of bad chars");
  testSanitize(".a.com,,.b.com", ".a.com,,.b.com",
               "duplicate commas from user are untouched");
  testSanitize(".a.com\n.b.com\n.c.com,\n.d.com,\n.e.com",
               ".a.com,.b.com,.c.com,.d.com,.e.com",
               "sanitize replaces things globally");

  
  networkProxyNonePref.value = ".a.com;.b.com\n.c.com";
}

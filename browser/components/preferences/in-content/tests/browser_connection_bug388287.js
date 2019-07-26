



Components.utils.import("resource://gre/modules/Services.jsm");


function open_preferences(aCallback) {
  gBrowser.selectedTab = gBrowser.addTab("about:preferences");
  let newTabBrowser = gBrowser.getBrowserForTab(gBrowser.selectedTab);
  newTabBrowser.addEventListener("Initialized", function () {
    newTabBrowser.removeEventListener("Initialized", arguments.callee, true);
    aCallback(gBrowser.contentWindow);
  }, true);
}

function test() {
  waitForExplicitFinish();
  let connectionTests = runConnectionTestsGen();
  connectionTests.next();
  const connectionURL = "chrome://browser/content/preferences/connection.xul";
  let closeable = false;
  let finalTest = false;
  let prefWin;

  
  
  let oldNetworkProxyType = Services.prefs.getIntPref("network.proxy.type");
  registerCleanupFunction(function() {
    Services.prefs.setIntPref("network.proxy.type", oldNetworkProxyType);
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
    try {
      Services.ww.unregisterNotification(observer);
    } catch(e) {
      
    }
  });

  
  let observer = {
    observe: function(aSubject, aTopic, aData) {
      if (aTopic == "domwindowopened") {
        
        let win = aSubject.QueryInterface(Components.interfaces.nsIDOMWindow);
        win.addEventListener("load", function winLoadListener() {
          win.removeEventListener("load", winLoadListener);
          if (win.location.href == connectionURL) {
            
            connectionTests.next(win);
          }
        });
      } else if (aTopic == "domwindowclosed") {
        
        let win = aSubject.QueryInterface(Components.interfaces.nsIDOMWindow);
        if (win.location.href == connectionURL) {
          ok(closeable, "Connection dialog closed");

          
          if (finalTest) {
            Services.ww.unregisterNotification(observer);
            gBrowser.removeCurrentTab();
            finish();
            return;
          }

          
          gBrowser.contentWindow.gAdvancedPane.showConnections();
        }
      }
    }
  };

  
  function* runConnectionTestsGen() {
    let doc, connectionWin, proxyTypePref, sharePref, httpPref, httpPortPref, ftpPref, ftpPortPref;

    
    function setDoc(win) {
      doc = win.document;
      connectionWin = win;
      proxyTypePref = doc.getElementById("network.proxy.type");
      sharePref = doc.getElementById("network.proxy.share_proxy_settings");
      httpPref = doc.getElementById("network.proxy.http");
      httpPortPref = doc.getElementById("network.proxy.http_port");
      ftpPref = doc.getElementById("network.proxy.ftp");
      ftpPortPref = doc.getElementById("network.proxy.ftp_port");
    }

    
    setDoc(yield null);

    
    proxyTypePref.value = 1;
    sharePref.value = true;
    httpPref.value = "localhost";
    httpPortPref.value = 0;
    doc.documentElement.acceptDialog();

    
    sharePref.value = false;
    ftpPref.value = "localhost";
    ftpPortPref.value = 80;
    doc.documentElement.acceptDialog();

    
    httpPortPref.value = 80;
    ftpPortPref.value = 0;
    doc.documentElement.acceptDialog();

    
    
    closeable = true;

    
    httpPortPref.value = 80;
    ftpPortPref.value = 80;
    doc.documentElement.acceptDialog();

    
    setDoc(yield null);
    proxyTypePref.value = 1;
    sharePref.value = true;
    ftpPref.value = "localhost";
    httpPref.value = "localhost";
    httpPortPref.value = 80;
    ftpPortPref.value = 0;
    doc.documentElement.acceptDialog();

    
    setDoc(yield null);
    proxyTypePref.value = 1;
    sharePref.value = true;
    httpPref.value = "";
    httpPortPref.value = 0;
    doc.documentElement.acceptDialog();

    
    setDoc(yield null);
    proxyTypePref.value = 0;
    sharePref.value = true;
    httpPref.value = "localhost";
    httpPortPref.value = 0;

    
    finalTest = true;
    doc.documentElement.acceptDialog();
    yield null;
  }

  




  open_preferences(function tabOpened(aContentWindow) {
    Services.ww.registerNotification(observer);
    gBrowser.contentWindow.gAdvancedPane.showConnections();
  });
}

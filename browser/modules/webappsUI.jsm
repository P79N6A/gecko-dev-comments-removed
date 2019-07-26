



let EXPORTED_SYMBOLS = ["webappsUI"];

let Ci = Components.interfaces;
let Cc = Components.classes;
let Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Webapps.jsm");
Cu.import("resource:///modules/WebappsInstaller.jsm");

let webappsUI = {
  init: function webappsUI_init() {
    Services.obs.addObserver(this, "webapps-ask-install", false);
    Services.obs.addObserver(this, "webapps-launch", false);
  },
  
  uninit: function webappsUI_uninit() {
    Services.obs.removeObserver(this, "webapps-ask-install");
    Services.obs.removeObserver(this, "webapps-launch");
  },

  observe: function webappsUI_observe(aSubject, aTopic, aData) {
    let data = JSON.parse(aData);

    switch(aTopic) {
      case "webapps-ask-install":
        let [chromeWin, browser] = this._getBrowserForId(data.oid);
        if (chromeWin)
          this.doInstall(data, browser, chromeWin);
        break;
      case "webapps-launch":
        DOMApplicationRegistry.getManifestFor(data.origin, (function(aManifest) {
          if (!aManifest)
            return;
          let manifest = new DOMApplicationManifest(aManifest, data.origin);
          this.openURL(manifest.fullLaunchPath(), data.origin);
        }).bind(this));
        break;
    }
  },

  openURL: function(aUrl, aOrigin) {
    let browserEnumerator = Services.wm.getEnumerator("navigator:browser");  
    let ss = Cc["@mozilla.org/browser/sessionstore;1"].getService(Ci.nsISessionStore);

    
    let found = false;
    while (!found && browserEnumerator.hasMoreElements()) {
      let browserWin = browserEnumerator.getNext();
      let tabbrowser = browserWin.gBrowser;

      
      let numTabs = tabbrowser.tabs.length;
      for (let index = 0; index < numTabs; index++) {
        let tab = tabbrowser.tabs[index];
        let appURL = ss.getTabValue(tab, "appOrigin");
        if (appURL == aOrigin) {
          
          tabbrowser.selectedTab = tab;
          browserWin.focus();
          found = true;
          break;
        }
      }
    }

    
    if (!found) {
      let recentWindow = Services.wm.getMostRecentWindow("navigator:browser");
      if (recentWindow) {
        
        let browser = recentWindow.gBrowser;
        let tab = browser.addTab(aUrl);
        browser.pinTab(tab);
        browser.selectedTab = tab;
        ss.setTabValue(tab, "appOrigin", aOrigin);
      }
    }
  },

  _getBrowserForId: function(aId) {
    let someWindow = Services.wm.getMostRecentWindow(null);

    if (someWindow) {
      let windowUtils = someWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                                  .getInterface(Ci.nsIDOMWindowUtils);
      let content = windowUtils.getOuterWindowWithId(aId);
      if (content) {
        let browser = content.QueryInterface(Ci.nsIInterfaceRequestor)
                      .getInterface(Ci.nsIWebNavigation)
                      .QueryInterface(Ci.nsIDocShell).chromeEventHandler;
        let win = browser.ownerDocument.defaultView;
        return [win, browser];
      }
    }

    return [null, null];
  },

  doInstall: function(aData, aBrowser, aWindow) {
    let bundle = aWindow.gNavigatorBundle;

    let mainAction = {
      label: bundle.getString("webapps.install"),
      accessKey: bundle.getString("webapps.install.accesskey"),
      callback: function(notification) {
        let app = WebappsInstaller.install(aData);
        if (app) {
          let localDir = null;
          if (app.appcacheDefined && app.appProfile) {
            localDir = app.appProfile.localDir;
          }

          DOMApplicationRegistry.confirmInstall(aData, false, localDir);
        } else {
          DOMApplicationRegistry.denyInstall(aData);
        }
      }
    };

    let requestingURI = aWindow.makeURI(aData.from);
    let manifest = new DOMApplicationManifest(aData.app.manifest, aData.app.origin);

    let host;
    try {
      host = requestingURI.host;
    } catch(e) {
      host = requestingURI.spec;
    }

    let message = bundle.getFormattedString("webapps.requestInstall",
                                            [manifest.name, host], 2);

    aWindow.PopupNotifications.show(aBrowser, "webapps-install", message,
                                    "webapps-notification-icon", mainAction);

  }
}





"use strict";

const { utils: Cu, interfaces: Ci, classes: Cc, results: Cr } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");

function MainProcessSingleton() {}
MainProcessSingleton.prototype = {
  classID: Components.ID("{0636a680-45cb-11e4-916c-0800200c9a66}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  logConsoleMessage: function(message) {
    let logMsg = message.data;
    logMsg.wrappedJSObject = logMsg;
    Services.obs.notifyObservers(logMsg, "console-api-log-event", null);
  },

  
  
  addSearchEngine: function({ target: browser, data: { pageURL, engineURL, iconURL, type } }) {
    pageURL = NetUtil.newURI(pageURL);
    engineURL = NetUtil.newURI(engineURL, null, pageURL);

    if (iconURL) {
      iconURL = NetUtil.newURI(iconURL, null, pageURL);
    }
    else {
      let tabbrowser = browser.getTabBrowser();
      if (browser.mIconURL && (!tabbrowser || tabbrowser.shouldLoadFavIcon(pageURL)))
        iconURL = NetUtil.newURI(browser.mIconURL);
    }

    try {
      
      let isWeb = ["https", "http", "ftp"];

      if (isWeb.indexOf(engineURL.scheme) < 0)
        throw "Unsupported search engine URL: " + engineURL;

      if (iconURL && isWeb.indexOf(iconURL.scheme) < 0)
        throw "Unsupported search icon URL: " + iconURL;
    }
    catch(ex) {
      Cu.reportError("Invalid argument passed to window.sidebar.addSearchEngine: " + ex);

      var searchBundle = Services.strings.createBundle("chrome://global/locale/search/search.properties");
      var brandBundle = Services.strings.createBundle("chrome://branding/locale/brand.properties");
      var brandName = brandBundle.GetStringFromName("brandShortName");
      var title = searchBundle.GetStringFromName("error_invalid_engine_title");
      var msg = searchBundle.formatStringFromName("error_invalid_engine_msg",
                                                  [brandName], 1);
      Services.ww.getNewPrompter(browser.ownerDocument.defaultView).alert(title, msg);
      return;
    }

    Services.search.init(function(status) {
      if (status != Cr.NS_OK)
        return;

      Services.search.addEngine(engineURL.spec, type, iconURL ? iconURL.spec : null, true);
    })
  },

  observe: function(subject, topic, data) {
    switch (topic) {
    case "app-startup": {
      Services.obs.addObserver(this, "xpcom-shutdown", false);

      
      
      Services.mm.loadFrameScript("chrome://global/content/browser-content.js", true);
      Services.ppmm.loadProcessScript("chrome://global/content/process-content.js", true);
      Services.ppmm.addMessageListener("Console:Log", this.logConsoleMessage);
      Services.mm.addMessageListener("Search:AddEngine", this.addSearchEngine);
      break;
    }

    case "xpcom-shutdown":
      Services.ppmm.removeMessageListener("Console:Log", this.logConsoleMessage);
      Services.mm.removeMessageListener("Search:AddEngine", this.addSearchEngine);
      break;
    }
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([MainProcessSingleton]);

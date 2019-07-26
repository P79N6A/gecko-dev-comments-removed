



"use strict";

let EXPORTED_SYMBOLS = ["WebappsHandler"];

let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Webapps.jsm");
Cu.import("resource://gre/modules/WebappsInstaller.jsm");
Cu.import("resource://gre/modules/WebappOSUtils.jsm");

let WebappsHandler = {
  init: function() {
    Services.obs.addObserver(this, "webapps-ask-install", false);
    Services.obs.addObserver(this, "webapps-launch", false);
    Services.obs.addObserver(this, "webapps-uninstall", false);
  },

  observe: function(subject, topic, data) {
    data = JSON.parse(data);
    data.mm = subject;

    switch (topic) {
      case "webapps-ask-install":
        let chromeWin = this._getWindowByOuterId(data.oid);
        if (chromeWin)
          this.doInstall(data, chromeWin);
        break;
      case "webapps-launch":
        WebappOSUtils.launch(data);
        break;
      case "webapps-uninstall":
        WebappOSUtils.uninstall(data);
        break;
    }
  },

  _getWindowByOuterId: function(outerId) {
    let someWindow = Services.wm.getMostRecentWindow(null);
    if (!someWindow) {
      return null;
    }

    let content = someWindow.QueryInterface(Ci.nsIInterfaceRequestor).
                             getInterface(Ci.nsIDOMWindowUtils).
                             getOuterWindowWithId(outerId);
    return content;
  },

  doInstall: function(data, window) {
    let {name} = data.app.manifest;
    let bundle = Services.strings.createBundle("chrome://webapprt/locale/webapp.properties");

    let choice = Services.prompt.confirmEx(
      window,
      bundle.formatStringFromName("webapps.install.title", [name], 1),
      bundle.formatStringFromName("webapps.install.description", [name], 1),
      
      Ci.nsIPromptService.BUTTON_POS_1_DEFAULT |
        Ci.nsIPromptService.BUTTON_TITLE_IS_STRING * Ci.nsIPromptService.BUTTON_POS_0 |
        Ci.nsIPromptService.BUTTON_TITLE_IS_STRING * Ci.nsIPromptService.BUTTON_POS_1,
      bundle.GetStringFromName("webapps.install.install"),
      bundle.GetStringFromName("webapps.install.dontinstall"),
      null,
      null,
      {});

    
    if (choice == 0 && WebappsInstaller.install(data)) {
      DOMApplicationRegistry.confirmInstall(data);
    }
    else {
      DOMApplicationRegistry.denyInstall(data);
    }
  }
};

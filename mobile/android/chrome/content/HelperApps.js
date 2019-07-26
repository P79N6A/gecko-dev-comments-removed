


"use strict";

XPCOMUtils.defineLazyGetter(this, "ContentAreaUtils", function() {
  let ContentAreaUtils = {};
  Services.scriptloader.loadSubScript("chrome://global/content/contentAreaUtils.js", ContentAreaUtils);
  return ContentAreaUtils;
});

function getBridge() {
  return Cc["@mozilla.org/android/bridge;1"].getService(Ci.nsIAndroidBridge);
}

function sendMessageToJava(aMessage) {
  return getBridge().handleGeckoMessage(JSON.stringify(aMessage));
}

var HelperApps =  {
  get defaultHttpHandlers() {
    let protoHandlers = this.getAppsForProtocol("http");

    var results = {};
    for (var i = 0; i < protoHandlers.length; i++) {
      try {
        let protoApp = protoHandlers.queryElementAt(i, Ci.nsIHandlerApp);
        results[protoApp.name] = protoApp;
      } catch(e) {}
    }

    delete this.defaultHttpHandlers;
    return this.defaultHttpHandlers = results;
  },

  get protoSvc() {
    delete this.protoSvc;
    return this.protoSvc = Cc["@mozilla.org/uriloader/external-protocol-service;1"].getService(Ci.nsIExternalProtocolService);
  },

  get urlHandlerService() {
    delete this.urlHandlerService;
    return this.urlHandlerService = Cc["@mozilla.org/uriloader/external-url-handler-service;1"].getService(Ci.nsIExternalURLHandlerService);
  },

  getAppsForProtocol: function getAppsForProtocol(uri) {
    let handlerInfoProto = this.protoSvc.getProtocolHandlerInfoFromOS(uri, {});
    return handlerInfoProto.possibleApplicationHandlers;
  },

  getAppsForUri: function getAppsFor(uri) {
    let found = [];
    let mimeType = ContentAreaUtils.getMIMETypeForURI(uri) || "";
    
    let msg = {
      type: "Intent:GetHandlers",
      mime: mimeType,
      action: "",
      url: uri.spec,
      packageName: "",
      className: ""
    };
    let apps = this._parseApps(JSON.parse(sendMessageToJava(msg)));
    for (let i = 0; i < apps.length; i++) {
      let appName = apps[i].name;
      if (appName.length > 0 && !this.defaultHttpHandlers[appName])
        found.push(apps[i]);
    }
    return found;
  },

  openUriInApp: function openUriInApp(uri) {
    let mimeType = ContentAreaUtils.getMIMETypeForURI(uri) || "";
    let msg = {
      type: "Intent:Open",
      mime: mimeType,
      action: "",
      url: uri.spec,
      packageName: "",
      className: ""
    };
    sendMessageToJava(msg);
  },

  _parseApps: function _parseApps(aJSON) {
    
    
    let appInfo = aJSON.apps;
    const numAttr = 4; 
    let apps = [];
    for (let i = 0; i < appInfo.length; i += numAttr) {
      apps.push({"name" : appInfo[i],
                 "isDefault" : appInfo[i+1],
                 "packageName" : appInfo[i+2],
                 "activityName" : appInfo[i+3]});
    }
    return apps;
  },

  showDoorhanger: function showDoorhanger(aUri, aCallback) {
    let permValue = Services.perms.testPermission(aUri, "native-intent");
    if (permValue != Services.perms.UNKNOWN_ACTION) {
      if (permValue == Services.perms.ALLOW_ACTION) {
        if (aCallback)
          aCallback(aUri);
        else
          this.openUriInApp(aUri);
      } else if (permValue == Services.perms.DENY_ACTION) {
        
      }
      return;
    }

    let apps = this.getAppsForUri(aUri);
    let strings = Strings.browser;

    let message = "";
    if (apps.length == 1)
      message = strings.formatStringFromName("helperapps.openWithApp2", [apps[0].name], 1);
    else
      message = strings.GetStringFromName("helperapps.openWithList2");

    let buttons = [{
      label: strings.GetStringFromName("helperapps.open"),
      callback: function(aChecked) {
        if (aChecked)
          Services.perms.add(aUri, "native-intent", Ci.nsIPermissionManager.ALLOW_ACTION);
        if (aCallback)
          aCallback(aUri);
        else
          this.openUriInApp(aUri);
      }
    }, {
      label: strings.GetStringFromName("helperapps.ignore"),
      callback: function(aChecked) {
        if (aChecked)
          Services.perms.add(aUri, "native-intent", Ci.nsIPermissionManager.DENY_ACTION);
      }
    }];

    let options = { checkbox: Strings.browser.GetStringFromName("helperapps.dontAskAgain") };
    NativeWindow.doorhanger.show(message, "helperapps-open", buttons, BrowserApp.selectedTab.id, options);
  }
};

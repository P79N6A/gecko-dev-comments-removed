



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/AppsUtils.jsm");

function debug(aMsg) {
  
}

XPCOMUtils.defineLazyServiceGetter(this, "appsService",
                                   "@mozilla.org/AppsService;1",
                                   "nsIAppsService");

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageBroadcaster");

XPCOMUtils.defineLazyServiceGetter(this, "UUIDGenerator",
                                   "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");

XPCOMUtils.defineLazyServiceGetter(this, "messenger",
                                   "@mozilla.org/system-message-internal;1",
                                   "nsISystemMessagesInternal");

const kMessages =["Webapps:Connect"];

function InterAppCommService() {
  Services.obs.addObserver(this, "xpcom-shutdown", false);
  Services.obs.addObserver(this, "inter-app-comm-select-app-result", false);

  kMessages.forEach(function(aMsg) {
    ppmm.addMessageListener(aMsg, this);
  }, this);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  this._registeredConnections = {};

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  this._allowedConnections = {};

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  this._promptUICallers = {};
}

InterAppCommService.prototype = {
  registerConnection: function(aKeyword, aHandlerPageURI, aManifestURI,
                               aDescription, aAppStatus, aRules) {
    let manifestURL = aManifestURI.spec;
    let pageURL = aHandlerPageURI.spec;

    debug("registerConnection: aKeyword: " + aKeyword +
          " manifestURL: " + manifestURL + " pageURL: " + pageURL +
          " aDescription: " + aDescription + " aAppStatus: " + aAppStatus +
          " aRules.minimumAccessLevel: " + aRules.minimumAccessLevel +
          " aRules.manifestURLs: " + aRules.manifestURLs +
          " aRules.installOrigins: " + aRules.installOrigins);

    let subAppManifestURLs = this._registeredConnections[aKeyword];
    if (!subAppManifestURLs) {
      subAppManifestURLs = this._registeredConnections[aKeyword] = {};
    }

    subAppManifestURLs[manifestURL] = {
      pageURL: pageURL,
      description: aDescription,
      appStatus: aAppStatus,
      rules: aRules,
      manifestURL: manifestURL
    };
  },

  _matchMinimumAccessLevel: function(aRules, aAppStatus) {
    if (!aRules || !aRules.minimumAccessLevel) {
      debug("rules.minimumAccessLevel is not available. No need to match.");
      return true;
    }

    let minAccessLevel = aRules.minimumAccessLevel;
    switch (minAccessLevel) {
      case "web":
        if (aAppStatus == Ci.nsIPrincipal.APP_STATUS_INSTALLED ||
            aAppStatus == Ci.nsIPrincipal.APP_STATUS_PRIVILEGED ||
            aAppStatus == Ci.nsIPrincipal.APP_STATUS_CERTIFIED) {
          return true;
        }
        break;
      case "privileged":
        if (aAppStatus == Ci.nsIPrincipal.APP_STATUS_PRIVILEGED ||
            aAppStatus == Ci.nsIPrincipal.APP_STATUS_CERTIFIED) {
          return true;
        }
        break;
      case "certified":
        if (aAppStatus == Ci.nsIPrincipal.APP_STATUS_CERTIFIED) {
          return true;
        }
        break;
    }

    debug("rules.minimumAccessLevel is not matched! " +
          "minAccessLevel: " + minAccessLevel + " aAppStatus : " + aAppStatus);
    return false;
  },

  _matchManifestURLs: function(aRules, aManifestURL) {
    if (!aRules || !Array.isArray(aRules.manifestURLs)) {
      debug("rules.manifestURLs is not available. No need to match.");
      return true;
    }

    let manifestURLs = aRules.manifestURLs;
    if (manifestURLs.indexOf(aManifestURL) != -1) {
      return true;
    }

    debug("rules.manifestURLs is not matched! " +
          "manifestURLs: " + manifestURLs + " aManifestURL : " + aManifestURL);
    return false;
  },

  _matchInstallOrigins: function(aRules, aManifestURL) {
    if (!aRules || !Array.isArray(aRules.installOrigins)) {
      debug("rules.installOrigins is not available. No need to match.");
      return true;
    }

    let installOrigin =
      appsService.getAppByManifestURL(aManifestURL).installOrigin;

    let installOrigins = aRules.installOrigins;
    if (installOrigins.indexOf(installOrigin) != -1) {
      return true;
    }

    debug("rules.installOrigins is not matched! aManifestURL: " + aManifestURL +
          " installOrigins: " + installOrigins + " installOrigin : " + installOrigin);
    return false;
  },

  _matchRules: function(aPubAppManifestURL, aPubAppStatus, aPubRules,
                        aSubAppManifestURL, aSubAppStatus, aSubRules) {
    
    
    
    
    if (aPubAppStatus != Ci.nsIPrincipal.APP_STATUS_CERTIFIED ||
        aSubAppStatus != Ci.nsIPrincipal.APP_STATUS_CERTIFIED) {
      debug("Only certified apps are allowed to do connections.");
      return false;
    }

    if (!aPubRules && !aSubRules) {
      debug("Rules of publisher and subscriber are absent. No need to match.");
      return true;
    }

    
    if (!this._matchMinimumAccessLevel(aPubRules, aSubAppStatus) ||
        !this._matchMinimumAccessLevel(aSubRules, aPubAppStatus)) {
      return false;
    }

    
    if (!this._matchManifestURLs(aPubRules, aSubAppManifestURL) ||
        !this._matchManifestURLs(aSubRules, aPubAppManifestURL)) {
      return false;
    }

    
    if (!this._matchInstallOrigins(aPubRules, aSubAppManifestURL) ||
        !this._matchInstallOrigins(aSubRules, aPubAppManifestURL)) {
      return false;
    }

    
    

    debug("All rules are matched.");
    return true;
  },

  _dispatchMessagePorts: function(aKeyword, aAllowedSubAppManifestURLs,
                                  aTarget, aOuterWindowID, aRequestID) {
    debug("_dispatchMessagePorts: aKeyword: " + aKeyword +
          " aAllowedSubAppManifestURLs: " + aAllowedSubAppManifestURLs);

    if (aAllowedSubAppManifestURLs.length == 0) {
      debug("No apps are allowed to connect. Returning.");
      aTarget.sendAsyncMessage("Webapps:Connect:Return:KO",
                               { oid: aOuterWindowID, requestID: aRequestID });
      return;
    }

    let subAppManifestURLs = this._registeredConnections[aKeyword];
    if (!subAppManifestURLs) {
      debug("No apps are subscribed to connect. Returning.");
      aTarget.sendAsyncMessage("Webapps:Connect:Return:KO",
                               { oid: aOuterWindowID, requestID: aRequestID });
      return;
    }

    let messagePortIDs = [];
    aAllowedSubAppManifestURLs.forEach(function(aAllowedSubAppManifestURL) {
      let subscribedInfo = subAppManifestURLs[aAllowedSubAppManifestURL];
      if (!subscribedInfo) {
        debug("The sunscribed info is not available. Skipping: " +
               aAllowedSubAppManifestURL);
        return;
      }

      let messagePortID = UUIDGenerator.generateUUID().toString();

      
      messenger.sendMessage("connection",
        { keyword: aKeyword,
          messagePortID: messagePortID },
        Services.io.newURI(subscribedInfo.pageURL, null, null),
        Services.io.newURI(subscribedInfo.manifestURL, null, null));

      messagePortIDs.push(messagePortID);
    });

    if (messagePortIDs.length == 0) {
      debug("No apps are subscribed to connect. Returning.");
      aTarget.sendAsyncMessage("Webapps:Connect:Return:KO",
                               { oid: aOuterWindowID, requestID: aRequestID });
      return;
    }

    
    debug("messagePortIDs: " + messagePortIDs);
    aTarget.sendAsyncMessage("Webapps:Connect:Return:OK",
                             { keyword: aKeyword,
                               messagePortIDs: messagePortIDs,
                               oid: aOuterWindowID, requestID: aRequestID });
  },

  _connect: function(aMessage, aTarget) {
    let keyword = aMessage.keyword;
    let pubRules = aMessage.rules;
    let pubAppManifestURL = aMessage.manifestURL;
    let outerWindowID = aMessage.outerWindowID;
    let requestID = aMessage.requestID;
    let pubAppStatus = aMessage.appStatus;

    let subAppManifestURLs = this._registeredConnections[keyword];
    if (!subAppManifestURLs) {
      debug("No apps are subscribed for this connection. Returning.")
      this._dispatchMessagePorts(keyword, [], aTarget, outerWindowID, requestID);
      return;
    }

    
    
    
    let allowedSubAppManifestURLs = [];
    let allowedPubAppManifestURLs = this._allowedConnections[keyword];
    if (allowedPubAppManifestURLs &&
        allowedPubAppManifestURLs[pubAppManifestURL]) {
      allowedSubAppManifestURLs = allowedPubAppManifestURLs[pubAppManifestURL];
    }

    
    let appsToSelect = [];
    for (let subAppManifestURL in subAppManifestURLs) {
      if (allowedSubAppManifestURLs.indexOf(subAppManifestURL) != -1) {
        debug("Don't need to select again. Skipping: " + subAppManifestURL);
        continue;
      }

      
      let subscribedInfo = subAppManifestURLs[subAppManifestURL];
      let subAppStatus = subscribedInfo.appStatus;
      let subRules = subscribedInfo.rules;

      let matched =
        this._matchRules(pubAppManifestURL, pubAppStatus, pubRules,
                         subAppManifestURL, subAppStatus, subRules);
      if (!matched) {
        debug("Rules are not matched. Skipping: " + subAppManifestURL);
        continue;
      }

      appsToSelect.push({
        manifestURL: subAppManifestURL,
        description: subscribedInfo.description
      });
    }

    if (appsToSelect.length == 0) {
      debug("No additional apps need to be selected for this connection. " +
            "Just dispatch message ports for the existing connections.");
      this._dispatchMessagePorts(keyword, allowedSubAppManifestURLs,
                                 aTarget, outerWindowID, requestID);
      return;
    }

    
    
    let callerID = UUIDGenerator.generateUUID().toString();
    this._promptUICallers[callerID] = {
      outerWindowID: outerWindowID,
      requestID: requestID,
      target: aTarget
    };

    
    
    
    
    
    
    








    
    
    
    debug("appsToSelect: " + appsToSelect);
    Services.obs.notifyObservers(null, 'inter-app-comm-select-app-result',
      JSON.stringify({ callerID: callerID,
                       manifestURL: pubAppManifestURL,
                       keyword: keyword,
                       selectedApps: appsToSelect }));
  },

  _handleSelectcedApps: function(aData) {
    let callerID = aData.callerID;
    let caller = this._promptUICallers[callerID];
    if (!caller) {
      debug("Error! Cannot find the caller.");
      return;
    }

    delete this._promptUICallers[callerID];

    let outerWindowID = caller.outerWindowID;
    let requestID = caller.requestID;
    let target = caller.target;

    let manifestURL = aData.manifestURL;
    let keyword = aData.keyword;
    let selectedApps = aData.selectedApps;

    if (selectedApps.length == 0) {
      debug("No apps are selected to connect.")
      this._dispatchMessagePorts(keyword, [], target, outerWindowID, requestID);
      return;
    }

    
    let allowedPubAppManifestURLs = this._allowedConnections[keyword];
    if (!allowedPubAppManifestURLs) {
      allowedPubAppManifestURLs = this._allowedConnections[keyword] = {};
    }
    let allowedSubAppManifestURLs = allowedPubAppManifestURLs[manifestURL];
    if (!allowedSubAppManifestURLs) {
      allowedSubAppManifestURLs = allowedPubAppManifestURLs[manifestURL] = [];
    }

    
    selectedApps.forEach(function(aSelectedApp) {
      let allowedSubAppManifestURL = aSelectedApp.manifestURL;
      if (allowedSubAppManifestURLs.indexOf(allowedSubAppManifestURL) == -1) {
        allowedSubAppManifestURLs.push(allowedSubAppManifestURL);
      }
    });

    
    
    this._dispatchMessagePorts(keyword, allowedSubAppManifestURLs,
                               target, outerWindowID, requestID);
  },

  receiveMessage: function(aMessage) {
    debug("receiveMessage: name: " + aMessage.name);
    let message = aMessage.json;
    let target = aMessage.target;

    
    
    if (kMessages.indexOf(aMessage.name) != -1) {
      if (!target.assertContainApp(message.manifestURL)) {
        debug("Got message from a child process carrying illegal manifest URL.");
        return null;
      }
    }

    switch (aMessage.name) {
      case "Webapps:Connect":
        this._connect(message, target);
        break;
    }
  },

  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "xpcom-shutdown":
        Services.obs.removeObserver(this, "xpcom-shutdown");
        Services.obs.removeObserver(this, "inter-app-comm-select-app-result");
        kMessages.forEach(function(aMsg) {
          ppmm.removeMessageListener(aMsg, this);
        }, this);
        ppmm = null;
        break;
      case "inter-app-comm-select-app-result":
        debug("inter-app-comm-select-app-result: " + aData);
        this._handleSelectcedApps(JSON.parse(aData));
        break;
    }
  },

  classID: Components.ID("{3dd15ce6-e7be-11e2-82bc-77967e7a63e6}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIInterAppCommService,
                                         Ci.nsIObserver])
}

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([InterAppCommService]);

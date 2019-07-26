




"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js");
Cu.import("resource:///modules/source-editor.jsm");
Cu.import("resource:///modules/devtools/shared/event-emitter.js");
Cu.import("resource:///modules/devtools/SideMenuWidget.jsm");
Cu.import("resource:///modules/devtools/VariablesView.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetworkHelper",
  "resource://gre/modules/devtools/NetworkHelper.jsm");

const NET_STRINGS_URI = "chrome://browser/locale/devtools/netmonitor.properties";
const LISTENERS = [ "NetworkActivity" ];
const NET_PREFS = { "NetworkMonitor.saveRequestAndResponseBodies": true };




let NetMonitorController = {
  





  startupNetMonitor: function() {
    if (this._isInitialized) {
      return this._startup.promise;
    }
    this._isInitialized = true;

    let deferred = this._startup = Promise.defer();

    NetMonitorView.initialize(() => {
      NetMonitorView._isInitialized = true;
      deferred.resolve();
    });

    return deferred.promise;
  },

  





  shutdownNetMonitor: function() {
    if (this._isDestroyed) {
      return this._shutdown.promise;
    }
    this._isDestroyed = true;
    this._startup = null;

    let deferred = this._shutdown = Promise.defer();

    NetMonitorView.destroy(() => {
      NetMonitorView._isDestroyed = true;
      this.TargetEventsHandler.disconnect();
      this.NetworkEventsHandler.disconnect();

      this.disconnect();
      deferred.resolve();
    });

    return deferred.promise;
  },

  






  connect: function() {
    if (this._connection) {
      return this._connection.promise;
    }

    let deferred = this._connection = Promise.defer();

    let target = this._target;
    let { client, form } = target;
    if (target.chrome) {
      this._startChromeMonitoring(client, form.consoleActor, deferred.resolve);
    } else {
      this._startMonitoringTab(client, form, deferred.resolve);
    }

    return deferred.promise;
  },

  


  disconnect: function() {
    
    
    this._connection = null;
    this.client = null;
    this.tabClient = null;
    this.webConsoleClient = null;
  },

  









  _startMonitoringTab: function(aClient, aTabGrip, aCallback) {
    if (!aClient) {
      Cu.reportError("No client found!");
      return;
    }
    this.client = aClient;

    aClient.attachTab(aTabGrip.actor, (aResponse, aTabClient) => {
      if (!aTabClient) {
        Cu.reportError("No tab client found!");
        return;
      }
      this.tabClient = aTabClient;

      aClient.attachConsole(aTabGrip.consoleActor, LISTENERS, (aResponse, aWebConsoleClient) => {
        if (!aWebConsoleClient) {
          Cu.reportError("Couldn't attach to console: " + aResponse.error);
          return;
        }
        this.webConsoleClient = aWebConsoleClient;
        this.webConsoleClient.setPreferences(NET_PREFS, () => {
          this.TargetEventsHandler.connect();
          this.NetworkEventsHandler.connect();

          if (aCallback) {
            aCallback();
          }
        });
      });
    });
  },

  









  _startChromeMonitoring: function(aClient, aConsoleActor, aCallback) {
    if (!aClient) {
      Cu.reportError("No client found!");
      return;
    }
    this.client = aClient;

    aClient.attachConsole(aConsoleActor, LISTENERS, (aResponse, aWebConsoleClient) => {
      if (!aWebConsoleClient) {
        Cu.reportError("Couldn't attach to console: " + aResponse.error);
        return;
      }
      this.webConsoleClient = aWebConsoleClient;
      this.webConsoleClient.setPreferences(NET_PREFS, () => {
        this.TargetEventsHandler.connect();
        this.NetworkEventsHandler.connect();

        if (aCallback) {
          aCallback();
        }
      });
    });
  },

  _isInitialized: false,
  _isDestroyed: false,
  _startup: null,
  _shutdown: null,
  _connection: null,
  client: null,
  tabClient: null,
  webConsoleClient: null
};




function TargetEventsHandler() {
  this._onTabNavigated = this._onTabNavigated.bind(this);
  this._onTabDetached = this._onTabDetached.bind(this);
}

TargetEventsHandler.prototype = {
  get target() NetMonitorController._target,
  get webConsoleClient() NetMonitorController.webConsoleClient,

  


  connect: function() {
    dumpn("TargetEventsHandler is connecting...");
    this.target.on("close", this._onTabDetached);
    this.target.on("navigate", this._onTabNavigated);
    this.target.on("will-navigate", this._onTabNavigated);
  },

  


  disconnect: function() {
    if (!this.target) {
      return;
    }
    dumpn("TargetEventsHandler is disconnecting...");
    this.target.off("close", this._onTabDetached);
    this.target.off("navigate", this._onTabNavigated);
    this.target.off("will-navigate", this._onTabNavigated);
  },

  







  _onTabNavigated: function(aType, aPacket) {
    if (aType == "will-navigate") {
      NetMonitorView.RequestsMenu.reset();
      NetMonitorView.NetworkDetails.toggle(false);
      window.emit("NetMonitor:TargetWillNavigate");
    }
    if (aType == "navigate") {
      window.emit("NetMonitor:TargetNavigate");
    }
  },

  


  _onTabDetached: function() {
    NetMonitorController.shutdownNetMonitor();
  }
};




function NetworkEventsHandler() {
  this._onNetworkEvent = this._onNetworkEvent.bind(this);
  this._onNetworkEventUpdate = this._onNetworkEventUpdate.bind(this);
  this._onRequestHeaders = this._onRequestHeaders.bind(this);
  this._onRequestCookies = this._onRequestCookies.bind(this);
  this._onRequestPostData = this._onRequestPostData.bind(this);
  this._onResponseHeaders = this._onResponseHeaders.bind(this);
  this._onResponseCookies = this._onResponseCookies.bind(this);
  this._onResponseContent = this._onResponseContent.bind(this);
  this._onEventTimings = this._onEventTimings.bind(this);
}

NetworkEventsHandler.prototype = {
  get client() NetMonitorController._target.client,
  get webConsoleClient() NetMonitorController.webConsoleClient,

  


  connect: function() {
    dumpn("NetworkEventsHandler is connecting...");
    this.client.addListener("networkEvent", this._onNetworkEvent);
    this.client.addListener("networkEventUpdate", this._onNetworkEventUpdate);
  },

  


  disconnect: function() {
    if (!this.client) {
      return;
    }
    dumpn("NetworkEventsHandler is disconnecting...");
    this.client.removeListener("networkEvent", this._onNetworkEvent);
    this.client.removeListener("networkEventUpdate", this._onNetworkEventUpdate);
  },

  







  _onNetworkEvent: function(aType, aPacket) {
    let { actor, startedDateTime, method, url } = aPacket.eventActor;
    NetMonitorView.RequestsMenu.addRequest(actor, startedDateTime, method, url);

    window.emit("NetMonitor:NetworkEvent");
  },

  







  _onNetworkEventUpdate: function(aType, aPacket) {
    let actor = aPacket.from;

    switch (aPacket.updateType) {
      case "requestHeaders":
        this.webConsoleClient.getRequestHeaders(actor, this._onRequestHeaders);
        window.emit("NetMonitor:NetworkEventUpdating:RequestHeaders");
        break;
      case "requestCookies":
        this.webConsoleClient.getRequestCookies(actor, this._onRequestCookies);
        window.emit("NetMonitor:NetworkEventUpdating:RequestCookies");
        break;
      case "requestPostData":
        this.webConsoleClient.getRequestPostData(actor, this._onRequestPostData);
        window.emit("NetMonitor:NetworkEventUpdating:RequestPostData");
        break;
      case "responseHeaders":
        this.webConsoleClient.getResponseHeaders(actor, this._onResponseHeaders);
        window.emit("NetMonitor:NetworkEventUpdating:ResponseHeaders");
        break;
      case "responseCookies":
        this.webConsoleClient.getResponseCookies(actor, this._onResponseCookies);
        window.emit("NetMonitor:NetworkEventUpdating:ResponseCookies");
        break;
      case "responseStart":
        NetMonitorView.RequestsMenu.updateRequest(aPacket.from, {
          httpVersion: aPacket.response.httpVersion,
          status: aPacket.response.status,
          statusText: aPacket.response.statusText,
          headersSize: aPacket.response.headersSize
        });
        window.emit("NetMonitor:NetworkEventUpdating:ResponseStart");
        break;
      case "responseContent":
        NetMonitorView.RequestsMenu.updateRequest(aPacket.from, {
          contentSize: aPacket.contentSize,
          mimeType: aPacket.mimeType
        });
        this.webConsoleClient.getResponseContent(actor, this._onResponseContent);
        window.emit("NetMonitor:NetworkEventUpdating:ResponseContent");
        break;
      case "eventTimings":
        NetMonitorView.RequestsMenu.updateRequest(aPacket.from, {
          totalTime: aPacket.totalTime
        });
        this.webConsoleClient.getEventTimings(actor, this._onEventTimings);
        window.emit("NetMonitor:NetworkEventUpdating:EventTimings");
        break;
    }
  },

  





  _onRequestHeaders: function(aResponse) {
    NetMonitorView.RequestsMenu.updateRequest(aResponse.from, {
      requestHeaders: aResponse
    });
    window.emit("NetMonitor:NetworkEventUpdated:RequestHeaders");
  },

  





  _onRequestCookies: function(aResponse) {
    NetMonitorView.RequestsMenu.updateRequest(aResponse.from, {
      requestCookies: aResponse
    });
    window.emit("NetMonitor:NetworkEventUpdated:RequestCookies");
  },

  





  _onRequestPostData: function(aResponse) {
    NetMonitorView.RequestsMenu.updateRequest(aResponse.from, {
      requestPostData: aResponse
    });
    window.emit("NetMonitor:NetworkEventUpdated:RequestPostData");
  },

  





  _onResponseHeaders: function(aResponse) {
    NetMonitorView.RequestsMenu.updateRequest(aResponse.from, {
      responseHeaders: aResponse
    });
    window.emit("NetMonitor:NetworkEventUpdated:ResponseHeaders");
  },

  





  _onResponseCookies: function(aResponse) {
    NetMonitorView.RequestsMenu.updateRequest(aResponse.from, {
      responseCookies: aResponse
    });
    window.emit("NetMonitor:NetworkEventUpdated:ResponseCookies");
  },

  





  _onResponseContent: function(aResponse) {
    NetMonitorView.RequestsMenu.updateRequest(aResponse.from, {
      responseContent: aResponse
    });
    window.emit("NetMonitor:NetworkEventUpdated:ResponseContent");
  },

  





  _onEventTimings: function NEH__onEventTimings(aResponse) {
    NetMonitorView.RequestsMenu.updateRequest(aResponse.from, {
      eventTimings: aResponse
    });
    window.emit("NetMonitor:NetworkEventUpdated:EventTimings");
  },

  










  getString: function NEH_getString(aStringGrip) {
    
    if (typeof aStringGrip != "object" || aStringGrip.type != "longString") {
      return Promise.resolve(aStringGrip); 
    }
    
    if (aStringGrip._fullText) {
      return aStringGrip._fullText.promise;
    }

    let deferred = aStringGrip._fullText = Promise.defer();
    let { actor, initial, length } = aStringGrip;
    let longStringClient = this.webConsoleClient.longString(aStringGrip);

    longStringClient.substring(initial.length, length, (aResponse) => {
      if (aResponse.error) {
        Cu.reportError(aResponse.error + ": " + aResponse.message);
        deferred.reject(aResponse);
        return;
      }
      deferred.resolve(initial + aResponse.substring);
    });

    return deferred.promise;
  }
};




let L10N = new ViewHelpers.L10N(NET_STRINGS_URI);




let Prefs = new ViewHelpers.Prefs("devtools.netmonitor", {
  networkDetailsWidth: ["Int", "panes-network-details-width"],
  networkDetailsHeight: ["Int", "panes-network-details-height"]
});




EventEmitter.decorate(this);




NetMonitorController.TargetEventsHandler = new TargetEventsHandler();
NetMonitorController.NetworkEventsHandler = new NetworkEventsHandler();




Object.defineProperties(window, {
  "create": {
    get: function() ViewHelpers.create
  },
  "gNetwork": {
    get: function() NetMonitorController.NetworkEventsHandler
  }
});





function dumpn(str) {
  if (wantLogging) {
    dump("NET-FRONTEND: " + str + "\n");
  }
}

let wantLogging = Services.prefs.getBoolPref("devtools.debugger.log");






"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

const NET_STRINGS_URI = "chrome://browser/locale/devtools/netmonitor.properties";
const PKI_STRINGS_URI = "chrome://pippki/locale/pippki.properties";
const LISTENERS = [ "NetworkActivity" ];
const NET_PREFS = { "NetworkMonitor.saveRequestAndResponseBodies": true };


const EVENTS = {
  
  TARGET_WILL_NAVIGATE: "NetMonitor:TargetWillNavigate",
  TARGET_DID_NAVIGATE: "NetMonitor:TargetNavigate",

  
  
  
  NETWORK_EVENT: "NetMonitor:NetworkEvent",

  
  REQUEST_ADDED: "NetMonitor:RequestAdded",

  
  UPDATING_REQUEST_HEADERS: "NetMonitor:NetworkEventUpdating:RequestHeaders",
  RECEIVED_REQUEST_HEADERS: "NetMonitor:NetworkEventUpdated:RequestHeaders",

  
  UPDATING_REQUEST_COOKIES: "NetMonitor:NetworkEventUpdating:RequestCookies",
  RECEIVED_REQUEST_COOKIES: "NetMonitor:NetworkEventUpdated:RequestCookies",

  
  UPDATING_REQUEST_POST_DATA: "NetMonitor:NetworkEventUpdating:RequestPostData",
  RECEIVED_REQUEST_POST_DATA: "NetMonitor:NetworkEventUpdated:RequestPostData",

  
  UPDATING_SECURITY_INFO: "NetMonitor::NetworkEventUpdating:SecurityInfo",
  RECEIVED_SECURITY_INFO: "NetMonitor::NetworkEventUpdated:SecurityInfo",

  
  UPDATING_RESPONSE_HEADERS: "NetMonitor:NetworkEventUpdating:ResponseHeaders",
  RECEIVED_RESPONSE_HEADERS: "NetMonitor:NetworkEventUpdated:ResponseHeaders",

  
  UPDATING_RESPONSE_COOKIES: "NetMonitor:NetworkEventUpdating:ResponseCookies",
  RECEIVED_RESPONSE_COOKIES: "NetMonitor:NetworkEventUpdated:ResponseCookies",

  
  UPDATING_EVENT_TIMINGS: "NetMonitor:NetworkEventUpdating:EventTimings",
  RECEIVED_EVENT_TIMINGS: "NetMonitor:NetworkEventUpdated:EventTimings",

  
  STARTED_RECEIVING_RESPONSE: "NetMonitor:NetworkEventUpdating:ResponseStart",
  UPDATING_RESPONSE_CONTENT: "NetMonitor:NetworkEventUpdating:ResponseContent",
  RECEIVED_RESPONSE_CONTENT: "NetMonitor:NetworkEventUpdated:ResponseContent",

  
  REQUEST_POST_PARAMS_DISPLAYED: "NetMonitor:RequestPostParamsAvailable",

  
  RESPONSE_BODY_DISPLAYED: "NetMonitor:ResponseBodyAvailable",

  
  RESPONSE_HTML_PREVIEW_DISPLAYED: "NetMonitor:ResponseHtmlPreviewAvailable",

  
  RESPONSE_IMAGE_THUMBNAIL_DISPLAYED: "NetMonitor:ResponseImageThumbnailAvailable",

  
  TAB_UPDATED: "NetMonitor:TabUpdated",

  
  SIDEBAR_POPULATED: "NetMonitor:SidebarPopulated",

  
  NETWORKDETAILSVIEW_POPULATED: "NetMonitor:NetworkDetailsViewPopulated",

  
  CUSTOMREQUESTVIEW_POPULATED: "NetMonitor:CustomRequestViewPopulated",

  
  PLACEHOLDER_CHARTS_DISPLAYED: "NetMonitor:PlaceholderChartsDisplayed",
  PRIMED_CACHE_CHART_DISPLAYED: "NetMonitor:PrimedChartsDisplayed",
  EMPTY_CACHE_CHART_DISPLAYED: "NetMonitor:EmptyChartsDisplayed",

  
  
  CONNECTED: "connected",
};


const ACTIVITY_TYPE = {
  
  NONE: 0,

  
  RELOAD: {
    WITH_CACHE_ENABLED: 1,
    WITH_CACHE_DISABLED: 2,
    WITH_CACHE_DEFAULT: 3
  },

  
  ENABLE_CACHE: 3,
  DISABLE_CACHE: 4
};

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/devtools/SideMenuWidget.jsm");
Cu.import("resource:///modules/devtools/VariablesView.jsm");
Cu.import("resource:///modules/devtools/VariablesViewController.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

const require = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools.require;
const promise = Cu.import("resource://gre/modules/Promise.jsm", {}).Promise;
const EventEmitter = require("devtools/toolkit/event-emitter");
const Editor = require("devtools/sourceeditor/editor");
const {Tooltip} = require("devtools/shared/widgets/Tooltip");
const {ToolSidebar} = require("devtools/framework/sidebar");

XPCOMUtils.defineLazyModuleGetter(this, "Chart",
  "resource:///modules/devtools/Chart.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Curl",
  "resource:///modules/devtools/Curl.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "CurlUtils",
  "resource:///modules/devtools/Curl.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Task",
  "resource://gre/modules/Task.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PluralForm",
  "resource://gre/modules/PluralForm.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "DevToolsUtils",
  "resource://gre/modules/devtools/DevToolsUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "clipboardHelper",
  "@mozilla.org/widget/clipboardhelper;1", "nsIClipboardHelper");

XPCOMUtils.defineLazyServiceGetter(this, "DOMParser",
  "@mozilla.org/xmlextras/domparser;1", "nsIDOMParser");

Object.defineProperty(this, "NetworkHelper", {
  get: function() {
    return require("devtools/toolkit/webconsole/network-helper");
  },
  configurable: true,
  enumerable: true
});




let NetMonitorController = {
  





  startupNetMonitor: function() {
    if (this._startup) {
      return this._startup;
    }

    NetMonitorView.initialize();

    
    return this._startup = promise.resolve();
  },

  





  shutdownNetMonitor: function() {
    if (this._shutdown) {
      return this._shutdown;
    }

    NetMonitorView.destroy();
    this.TargetEventsHandler.disconnect();
    this.NetworkEventsHandler.disconnect();
    this.disconnect();

    
    return this._shutdown = promise.resolve();
  },

  








  connect: Task.async(function*() {
    if (this._connection) {
      return this._connection;
    }

    let deferred = promise.defer();
    this._connection = deferred.promise;

    this.client = this._target.client;
    
    
    if (this._target.isTabActor) {
      this.tabClient = this._target.activeTab;
    }
    this.webConsoleClient = this._target.activeConsole;
    this.webConsoleClient.setPreferences(NET_PREFS, () => {
      this.TargetEventsHandler.connect();
      this.NetworkEventsHandler.connect();
      deferred.resolve();
    });

    yield deferred.promise;
    window.emit(EVENTS.CONNECTED);
  }),

  


  disconnect: function() {
    
    
    this._connection = null;
    this.client = null;
    this.tabClient = null;
    this.webConsoleClient = null;
  },

  



  isConnected: function() {
    return !!this.client;
  },

  



  getCurrentActivity: function() {
    return this._currentActivity || ACTIVITY_TYPE.NONE;
  },

  









  triggerActivity: function(aType) {
    
    let standBy = () => {
      this._currentActivity = ACTIVITY_TYPE.NONE;
    };

    
    let waitForNavigation = () => {
      let deferred = promise.defer();
      this._target.once("will-navigate", () => {
        this._target.once("navigate", () => {
          deferred.resolve();
        });
      });
      return deferred.promise;
    };

    
    let reconfigureTab = aOptions => {
      let deferred = promise.defer();
      this._target.activeTab.reconfigure(aOptions, deferred.resolve);
      return deferred.promise;
    };

    
    let reconfigureTabAndWaitForNavigation = aOptions => {
      aOptions.performReload = true;
      let navigationFinished = waitForNavigation();
      return reconfigureTab(aOptions).then(() => navigationFinished);
    }
    if (aType == ACTIVITY_TYPE.RELOAD.WITH_CACHE_DEFAULT) {
      return reconfigureTabAndWaitForNavigation({}).then(standBy);
    }
    if (aType == ACTIVITY_TYPE.RELOAD.WITH_CACHE_ENABLED) {
      this._currentActivity = ACTIVITY_TYPE.ENABLE_CACHE;
      this._target.once("will-navigate", () => this._currentActivity = aType);
      return reconfigureTabAndWaitForNavigation({ cacheDisabled: false, performReload: true }).then(standBy);
    }
    if (aType == ACTIVITY_TYPE.RELOAD.WITH_CACHE_DISABLED) {
      this._currentActivity = ACTIVITY_TYPE.DISABLE_CACHE;
      this._target.once("will-navigate", () => this._currentActivity = aType);
      return reconfigureTabAndWaitForNavigation({ cacheDisabled: true, performReload: true }).then(standBy);
    }
    if (aType == ACTIVITY_TYPE.ENABLE_CACHE) {
      this._currentActivity = aType;
      return reconfigureTab({ cacheDisabled: false, performReload: false }).then(standBy);
    }
    if (aType == ACTIVITY_TYPE.DISABLE_CACHE) {
      this._currentActivity = aType;
      return reconfigureTab({ cacheDisabled: true, performReload: false }).then(standBy);
    }
    this._currentActivity = ACTIVITY_TYPE.NONE;
    return promise.reject(new Error("Invalid activity type"));
  },

  



  get supportsCustomRequest() {
    return this.webConsoleClient &&
           (this.webConsoleClient.traits.customNetworkRequest ||
            !this._target.isApp);
  },

  




  get supportsTransferredResponseSize() {
    return this.webConsoleClient &&
           this.webConsoleClient.traits.transferredResponseSize;
  },

  



  get supportsPerfStats() {
    return this.tabClient &&
           (this.tabClient.traits.reconfigure || !this._target.isApp);
  },

  _startup: null,
  _shutdown: null,
  _connection: null,
  _currentActivity: null,
  client: null,
  tabClient: null,
  webConsoleClient: null
};




function TargetEventsHandler() {
  this._onTabNavigated = this._onTabNavigated.bind(this);
  this._onTabDetached = this._onTabDetached.bind(this);
}

TargetEventsHandler.prototype = {
  get target() {
    return NetMonitorController._target;
  },

  


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
    switch (aType) {
      case "will-navigate": {
        
        if (!Services.prefs.getBoolPref("devtools.webconsole.persistlog")) {
          NetMonitorView.RequestsMenu.reset();
          NetMonitorView.Sidebar.toggle(false);
        }
        
        if (NetMonitorController.getCurrentActivity() == ACTIVITY_TYPE.NONE) {
          NetMonitorView.showNetworkInspectorView();
        }

        window.emit(EVENTS.TARGET_WILL_NAVIGATE);
        break;
      }
      case "navigate": {
        window.emit(EVENTS.TARGET_DID_NAVIGATE);
        break;
      }
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
  get client() {
    return NetMonitorController._target.client;
  },

  get webConsoleClient() {
    return NetMonitorController.webConsoleClient;
  },

  


  connect: function() {
    dumpn("NetworkEventsHandler is connecting...");
    this.webConsoleClient.on("networkEvent", this._onNetworkEvent);
    this.webConsoleClient.on("networkEventUpdate", this._onNetworkEventUpdate);
    this._displayCachedEvents();
  },

  


  _displayCachedEvents: function() {
    for (let cachedEvent of this.webConsoleClient.getNetworkEvents()) {
      
      this._onNetworkEvent("networkEvent", cachedEvent);
      
      for (let update of cachedEvent.updates) {
        this._onNetworkEventUpdate("networkEventUpdate", {
          packet: {
            updateType: update
          },
          networkInfo: cachedEvent
        });
      }
    }
  },

  


  disconnect: function() {
    if (!this.client) {
      return;
    }
    dumpn("NetworkEventsHandler is disconnecting...");
    this.webConsoleClient.off("networkEvent", this._onNetworkEvent);
    this.webConsoleClient.off("networkEventUpdate", this._onNetworkEventUpdate);
  },

  







  _onNetworkEvent: function(type, networkInfo) {
    let { actor, startedDateTime, request: { method, url }, isXHR, fromCache } = networkInfo;

    NetMonitorView.RequestsMenu.addRequest(
      actor, startedDateTime, method, url, isXHR, fromCache
    );
    window.emit(EVENTS.NETWORK_EVENT, actor);
  },

  









  _onNetworkEventUpdate: function(type, { packet, networkInfo }) {
    let { actor, request: { url } } = networkInfo;
    switch (packet.updateType) {
      case "requestHeaders":
        this.webConsoleClient.getRequestHeaders(actor, this._onRequestHeaders);
        window.emit(EVENTS.UPDATING_REQUEST_HEADERS, actor);
        break;
      case "requestCookies":
        this.webConsoleClient.getRequestCookies(actor, this._onRequestCookies);
        window.emit(EVENTS.UPDATING_REQUEST_COOKIES, actor);
        break;
      case "requestPostData":
        this.webConsoleClient.getRequestPostData(actor, this._onRequestPostData);
        window.emit(EVENTS.UPDATING_REQUEST_POST_DATA, actor);
        break;
      case "securityInfo":
        NetMonitorView.RequestsMenu.updateRequest(actor, {
          securityState: networkInfo.securityInfo,
        });
        this.webConsoleClient.getSecurityInfo(actor, this._onSecurityInfo);
        window.emit(EVENTS.UPDATING_SECURITY_INFO, actor);
        break;
      case "responseHeaders":
        this.webConsoleClient.getResponseHeaders(actor, this._onResponseHeaders);
        window.emit(EVENTS.UPDATING_RESPONSE_HEADERS, actor);
        break;
      case "responseCookies":
        this.webConsoleClient.getResponseCookies(actor, this._onResponseCookies);
        window.emit(EVENTS.UPDATING_RESPONSE_COOKIES, actor);
        break;
      case "responseStart":
        NetMonitorView.RequestsMenu.updateRequest(actor, {
          httpVersion: networkInfo.response.httpVersion,
          remoteAddress: networkInfo.response.remoteAddress,
          remotePort: networkInfo.response.remotePort,
          status: networkInfo.response.status,
          statusText: networkInfo.response.statusText,
          headersSize: networkInfo.response.headersSize
        });
        window.emit(EVENTS.STARTED_RECEIVING_RESPONSE, actor);
        break;
      case "responseContent":
        NetMonitorView.RequestsMenu.updateRequest(actor, {
          contentSize: networkInfo.response.bodySize,
          transferredSize: networkInfo.response.transferredSize,
          mimeType: networkInfo.response.content.mimeType
        });
        this.webConsoleClient.getResponseContent(actor, this._onResponseContent);
        window.emit(EVENTS.UPDATING_RESPONSE_CONTENT, actor);
        break;
      case "eventTimings":
        NetMonitorView.RequestsMenu.updateRequest(actor, {
          totalTime: networkInfo.totalTime
        });
        this.webConsoleClient.getEventTimings(actor, this._onEventTimings);
        window.emit(EVENTS.UPDATING_EVENT_TIMINGS, actor);
        break;
    }
  },

  





  _onRequestHeaders: function(aResponse) {
    NetMonitorView.RequestsMenu.updateRequest(aResponse.from, {
      requestHeaders: aResponse
    }, () => {
      window.emit(EVENTS.RECEIVED_REQUEST_HEADERS, aResponse.from);
    });
  },

  





  _onRequestCookies: function(aResponse) {
    NetMonitorView.RequestsMenu.updateRequest(aResponse.from, {
      requestCookies: aResponse
    }, () => {
      window.emit(EVENTS.RECEIVED_REQUEST_COOKIES, aResponse.from);
    });
  },

  





  _onRequestPostData: function(aResponse) {
    NetMonitorView.RequestsMenu.updateRequest(aResponse.from, {
      requestPostData: aResponse
    }, () => {
      window.emit(EVENTS.RECEIVED_REQUEST_POST_DATA, aResponse.from);
    });
  },

  





   _onSecurityInfo: function(aResponse) {
     NetMonitorView.RequestsMenu.updateRequest(aResponse.from, {
       securityInfo: aResponse.securityInfo
     }, () => {
       window.emit(EVENTS.RECEIVED_SECURITY_INFO, aResponse.from);
     });
   },

  





  _onResponseHeaders: function(aResponse) {
    NetMonitorView.RequestsMenu.updateRequest(aResponse.from, {
      responseHeaders: aResponse
    }, () => {
      window.emit(EVENTS.RECEIVED_RESPONSE_HEADERS, aResponse.from);
    });
  },

  





  _onResponseCookies: function(aResponse) {
    NetMonitorView.RequestsMenu.updateRequest(aResponse.from, {
      responseCookies: aResponse
    }, () => {
      window.emit(EVENTS.RECEIVED_RESPONSE_COOKIES, aResponse.from);
    });
  },

  





  _onResponseContent: function(aResponse) {
    NetMonitorView.RequestsMenu.updateRequest(aResponse.from, {
      responseContent: aResponse
    }, () => {
      window.emit(EVENTS.RECEIVED_RESPONSE_CONTENT, aResponse.from);
    });
  },

  





  _onEventTimings: function(aResponse) {
    NetMonitorView.RequestsMenu.updateRequest(aResponse.from, {
      eventTimings: aResponse
    }, () => {
      window.emit(EVENTS.RECEIVED_EVENT_TIMINGS, aResponse.from);
    });
  },

  










  getString: function(aStringGrip) {
    return this.webConsoleClient.getString(aStringGrip);
  }
};




let L10N = new ViewHelpers.L10N(NET_STRINGS_URI);
let PKI_L10N = new ViewHelpers.L10N(PKI_STRINGS_URI);




let Prefs = new ViewHelpers.Prefs("devtools.netmonitor", {
  networkDetailsWidth: ["Int", "panes-network-details-width"],
  networkDetailsHeight: ["Int", "panes-network-details-height"],
  statistics: ["Bool", "statistics"],
  filters: ["Json", "filters"]
});





XPCOMUtils.defineLazyGetter(window, "isRTL", function() {
  return window.getComputedStyle(document.documentElement, null).direction == "rtl";
});




EventEmitter.decorate(this);




NetMonitorController.TargetEventsHandler = new TargetEventsHandler();
NetMonitorController.NetworkEventsHandler = new NetworkEventsHandler();




Object.defineProperties(window, {
  "gNetwork": {
    get: function() {
      return NetMonitorController.NetworkEventsHandler;
    },
    configurable: true
  }
});












function whenDataAvailable(aDataStore, aMandatoryFields) {
  let deferred = promise.defer();

  let interval = setInterval(() => {
    if (aDataStore.every(item => aMandatoryFields.every(field => field in item))) {
      clearInterval(interval);
      clearTimeout(timer);
      deferred.resolve();
    }
  }, WDA_DEFAULT_VERIFY_INTERVAL);

  let timer = setTimeout(() => {
    clearInterval(interval);
    deferred.reject(new Error("Timed out while waiting for data"));
  }, WDA_DEFAULT_GIVE_UP_TIMEOUT);

  return deferred.promise;
};

const WDA_DEFAULT_VERIFY_INTERVAL = 50; 





const WDA_DEFAULT_GIVE_UP_TIMEOUT = gDevTools.testing ? 45000 : 2000; 





function dumpn(str) {
  if (wantLogging) {
    dump("NET-FRONTEND: " + str + "\n");
  }
}

let wantLogging = Services.prefs.getBoolPref("devtools.debugger.log");

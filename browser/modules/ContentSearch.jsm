



"use strict";

this.EXPORTED_SYMBOLS = [
  "ContentSearch",
];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "FormHistory",
  "resource://gre/modules/FormHistory.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PrivateBrowsingUtils",
  "resource://gre/modules/PrivateBrowsingUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "SearchSuggestionController",
  "resource://gre/modules/SearchSuggestionController.jsm");

const INBOUND_MESSAGE = "ContentSearch";
const OUTBOUND_MESSAGE = INBOUND_MESSAGE;



















































this.ContentSearch = {

  
  
  
  _eventQueue: [],
  _currentEvent: null,

  
  
  _suggestionMap: new WeakMap(),

  init: function () {
    Cc["@mozilla.org/globalmessagemanager;1"].
      getService(Ci.nsIMessageListenerManager).
      addMessageListener(INBOUND_MESSAGE, this);
    Services.obs.addObserver(this, "browser-search-engine-modified", false);
  },

  receiveMessage: function (msg) {
    
    
    
    
    msg.handleEvent = event => {
      let browserData = this._suggestionMap.get(msg.target);
      if (browserData) {
        this._suggestionMap.delete(msg.target);
        this._suggestionMap.set(event.detail, browserData);
      }
      msg.target.removeEventListener("SwapDocShells", msg, true);
      msg.target = event.detail;
      msg.target.addEventListener("SwapDocShells", msg, true);
    };
    msg.target.addEventListener("SwapDocShells", msg, true);

    this._eventQueue.push({
      type: "Message",
      data: msg,
    });
    this._processEventQueue();
  },

  observe: function (subj, topic, data) {
    switch (topic) {
    case "browser-search-engine-modified":
      this._eventQueue.push({
        type: "Observe",
        data: data,
      });
      this._processEventQueue();
      break;
    }
  },

  _processEventQueue: Task.async(function* () {
    if (this._currentEvent || !this._eventQueue.length) {
      return;
    }
    this._currentEvent = this._eventQueue.shift();
    try {
      yield this["_on" + this._currentEvent.type](this._currentEvent.data);
    }
    catch (err) {
      Cu.reportError(err);
    }
    finally {
      this._currentEvent = null;
      this._processEventQueue();
    }
  }),

  _onMessage: Task.async(function* (msg) {
    let methodName = "_onMessage" + msg.data.type;
    if (methodName in this) {
      yield this._initService();
      yield this[methodName](msg, msg.data.data);
      msg.target.removeEventListener("SwapDocShells", msg, true);
    }
  }),

  _onMessageGetState: function (msg, data) {
    return this._currentStateObj().then(state => {
      this._reply(msg, "State", state);
    });
  },

  _onMessageSearch: function (msg, data) {
    this._ensureDataHasProperties(data, [
      "engineName",
      "searchString",
      "whence",
    ]);
    let browserWin = msg.target.ownerDocument.defaultView;
    let engine = Services.search.getEngineByName(data.engineName);
    browserWin.BrowserSearch.recordSearchInHealthReport(engine, data.whence);
    let submission = engine.getSubmission(data.searchString, "", data.whence);
    browserWin.loadURI(submission.uri.spec, null, submission.postData);
    return Promise.resolve();
  },

  _onMessageSetCurrentEngine: function (msg, data) {
    Services.search.currentEngine = Services.search.getEngineByName(data);
    return Promise.resolve();
  },

  _onMessageManageEngines: function (msg, data) {
    let browserWin = msg.target.ownerDocument.defaultView;
    browserWin.BrowserSearch.searchBar.openManager(null);
    return Promise.resolve();
  },

  _onMessageGetSuggestions: Task.async(function* (msg, data) {
    this._ensureDataHasProperties(data, [
      "engineName",
      "searchString",
    ]);

    let engine = Services.search.getEngineByName(data.engineName);
    if (!engine) {
      throw new Error("Unknown engine name: " + data.engineName);
    }

    let browserData = this._suggestionDataForBrowser(msg.target, true);
    let { controller } = browserData;
    let ok = SearchSuggestionController.engineOffersSuggestions(engine);
    controller.maxLocalResults = ok ? 2 : 6;
    controller.maxRemoteResults = ok ? 6 : 0;
    controller.remoteTimeout = data.remoteTimeout || undefined;
    let priv = PrivateBrowsingUtils.isWindowPrivate(msg.target.contentWindow);
    
    
    let suggestions = yield controller.fetch(data.searchString, priv, engine);

    
    
    
    
    
    browserData.previousFormHistoryResult = suggestions.formHistoryResult;

    this._reply(msg, "Suggestions", {
      engineName: data.engineName,
      searchString: suggestions.term,
      formHistory: suggestions.local,
      remote: suggestions.remote,
    });
  }),

  _onMessageAddFormHistoryEntry: function (msg, entry) {
    
    
    
    
    if (!msg.target.contentWindow ||
        PrivateBrowsingUtils.isWindowPrivate(msg.target.contentWindow)) {
      return Promise.resolve();
    }
    let browserData = this._suggestionDataForBrowser(msg.target, true);
    FormHistory.update({
      op: "bump",
      fieldname: browserData.controller.formHistoryParam,
      value: entry,
    }, {
      handleCompletion: () => {},
      handleError: err => {
        Cu.reportError("Error adding form history entry: " + err);
      },
    });
    return Promise.resolve();
  },

  _onMessageRemoveFormHistoryEntry: function (msg, entry) {
    let browserData = this._suggestionDataForBrowser(msg.target);
    if (browserData && browserData.previousFormHistoryResult) {
      let { previousFormHistoryResult } = browserData;
      for (let i = 0; i < previousFormHistoryResult.matchCount; i++) {
        if (previousFormHistoryResult.getValueAt(i) == entry) {
          previousFormHistoryResult.removeValueAt(i, true);
          break;
        }
      }
    }
    return Promise.resolve();
  },

  _onMessageSpeculativeConnect: function (msg, engineName) {
    let engine = Services.search.getEngineByName(engineName);
    if (!engine) {
      throw new Error("Unknown engine name: " + engineName);
    }
    if (msg.target.contentWindow) {
      engine.speculativeConnect({
        window: msg.target.contentWindow,
      });
    }
  },

  _onObserve: Task.async(function* (data) {
    if (data == "engine-current") {
      let engine = yield this._currentEngineObj();
      this._broadcast("CurrentEngine", engine);
    }
    else if (data != "engine-default") {
      
      
      let state = yield this._currentStateObj();
      this._broadcast("CurrentState", state);
    }
  }),

  _suggestionDataForBrowser: function (browser, create=false) {
    let data = this._suggestionMap.get(browser);
    if (!data && create) {
      
      
      
      data = {
        controller: new SearchSuggestionController(),
      };
      this._suggestionMap.set(browser, data);
    }
    return data;
  },

  _reply: function (msg, type, data) {
    
    
    if (msg.target.messageManager) {
      msg.target.messageManager.sendAsyncMessage(...this._msgArgs(type, data));
    }
  },

  _broadcast: function (type, data) {
    Cc["@mozilla.org/globalmessagemanager;1"].
      getService(Ci.nsIMessageListenerManager).
      broadcastAsyncMessage(...this._msgArgs(type, data));
  },

  _msgArgs: function (type, data) {
    return [OUTBOUND_MESSAGE, {
      type: type,
      data: data,
    }];
  },

  _currentStateObj: Task.async(function* () {
    let state = {
      engines: [],
      currentEngine: yield this._currentEngineObj(),
    };
    for (let engine of Services.search.getVisibleEngines()) {
      let uri = engine.getIconURLBySize(16, 16);
      state.engines.push({
        name: engine.name,
        iconBuffer: yield this._arrayBufferFromDataURI(uri),
      });
    }
    return state;
  }),

  _currentEngineObj: Task.async(function* () {
    let engine = Services.search.currentEngine;
    let uri1x = engine.getIconURLBySize(65, 26);
    let uri2x = engine.getIconURLBySize(130, 52);
    let obj = {
      name: engine.name,
      logoBuffer: yield this._arrayBufferFromDataURI(uri1x),
      logo2xBuffer: yield this._arrayBufferFromDataURI(uri2x),
    };
    return obj;
  }),

  _arrayBufferFromDataURI: function (uri) {
    if (!uri) {
      return Promise.resolve(null);
    }
    let deferred = Promise.defer();
    let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].
              createInstance(Ci.nsIXMLHttpRequest);
    xhr.open("GET", uri, true);
    xhr.responseType = "arraybuffer";
    xhr.onloadend = () => {
      deferred.resolve(xhr.response);
    };
    try {
      
      xhr.send();
    }
    catch (err) {
      return Promise.resolve(null);
    }
    return deferred.promise;
  },

  _ensureDataHasProperties: function (data, requiredProperties) {
    for (let prop of requiredProperties) {
      if (!(prop in data)) {
        throw new Error("Message data missing required property: " + prop);
      }
    }
  },

  _initService: function () {
    if (!this._initServicePromise) {
      let deferred = Promise.defer();
      this._initServicePromise = deferred.promise;
      Services.search.init(() => deferred.resolve());
    }
    return this._initServicePromise;
  },
};

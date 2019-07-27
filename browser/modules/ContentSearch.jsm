



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
const MAX_LOCAL_SUGGESTIONS = 3;
const MAX_SUGGESTIONS = 6;



















































this.ContentSearch = {

  
  
  
  _eventQueue: [],
  _currentEventPromise: null,

  
  
  _suggestionMap: new WeakMap(),

  
  _destroyedPromise: null,

  init: function () {
    Cc["@mozilla.org/globalmessagemanager;1"].
      getService(Ci.nsIMessageListenerManager).
      addMessageListener(INBOUND_MESSAGE, this);
    Services.obs.addObserver(this, "browser-search-engine-modified", false);
    Services.obs.addObserver(this, "shutdown-leaks-before-check", false);
    this._stringBundle = Services.strings.createBundle("chrome://global/locale/autocomplete.properties");
  },

  destroy: function () {
    if (this._destroyedPromise) {
      return this._destroyedPromise;
    }

    Cc["@mozilla.org/globalmessagemanager;1"].
      getService(Ci.nsIMessageListenerManager).
      removeMessageListener(INBOUND_MESSAGE, this);
    Services.obs.removeObserver(this, "browser-search-engine-modified");
    Services.obs.removeObserver(this, "shutdown-leaks-before-check");

    this._eventQueue.length = 0;
    return this._destroyedPromise = Promise.resolve(this._currentEventPromise);
  },

  




  focusInput: function (messageManager) {
    messageManager.sendAsyncMessage(OUTBOUND_MESSAGE, {
      type: "FocusInput"
    });
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
    case "shutdown-leaks-before-check":
      subj.wrappedJSObject.client.addBlocker(
        "ContentSearch: Wait until the service is destroyed", () => this.destroy());
      break;
    }
  },

  _processEventQueue: function () {
    if (this._currentEventPromise || !this._eventQueue.length) {
      return;
    }

    let event = this._eventQueue.shift();

    return this._currentEventPromise = Task.spawn(function* () {
      try {
        yield this["_on" + event.type](event.data);
      } catch (err) {
        Cu.reportError(err);
      } finally {
        this._currentEventPromise = null;
        this._processEventQueue();
      }
    }.bind(this));
  },

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
    let engine = Services.search.getEngineByName(data.engineName);
    let submission = engine.getSubmission(data.searchString, "", data.whence);
    let browser = msg.target;
    let newTab;
    if (data.useNewTab) {
      newTab = browser.getTabBrowser().addTab();
      browser = newTab.linkedBrowser;
    }
    try {
      browser.loadURIWithFlags(submission.uri.spec,
                               Ci.nsIWebNavigation.LOAD_FLAGS_NONE, null, null,
                               submission.postData);
    }
    catch (err) {
      
      
      
      return Promise.resolve();
    }
    let win = browser.ownerDocument.defaultView;
    win.BrowserSearch.recordSearchInHealthReport(engine, data.whence,
                                                 data.selection || null);
    return Promise.resolve();
  },

  _onMessageSetCurrentEngine: function (msg, data) {
    Services.search.currentEngine = Services.search.getEngineByName(data);
    return Promise.resolve();
  },

  _onMessageManageEngines: function (msg, data) {
    let browserWin = msg.target.ownerDocument.defaultView;

    if (Services.prefs.getBoolPref("browser.search.showOneOffButtons")) {
      browserWin.openPreferences("paneSearch");
      return Promise.resolve();
    }

    let wm = Components.classes["@mozilla.org/appshell/window-mediator;1"].
             getService(Components.interfaces.nsIWindowMediator);
    let window = wm.getMostRecentWindow("Browser:SearchManager");

    if (window) {
      window.focus()
    }
    else {
      browserWin.setTimeout(function () {
        browserWin.openDialog("chrome://browser/content/search/engineManager.xul",
          "_blank", "chrome,dialog,modal,centerscreen,resizable");
      }, 0);
    }
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
    controller.maxLocalResults = ok ? MAX_LOCAL_SUGGESTIONS : MAX_SUGGESTIONS;
    controller.maxRemoteResults = ok ? MAX_SUGGESTIONS : 0;
    controller.remoteTimeout = data.remoteTimeout || undefined;
    let priv = PrivateBrowsingUtils.isBrowserPrivate(msg.target);
    
    
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
    let isPrivate = true;
    try {
      
      
      
      isPrivate = PrivateBrowsingUtils.isBrowserPrivate(msg.target);
    } catch (err) {}
    if (isPrivate || entry === "") {
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
    let favicon = engine.getIconURLBySize(16, 16);
    let uri1x = engine.getIconURLBySize(65, 26);
    let uri2x = engine.getIconURLBySize(130, 52);
    let placeholder = this._stringBundle.formatStringFromName(
      "searchWithEngine", [engine.name], 1);
    let obj = {
      name: engine.name,
      placeholder: placeholder,
      iconBuffer: yield this._arrayBufferFromDataURI(favicon),
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

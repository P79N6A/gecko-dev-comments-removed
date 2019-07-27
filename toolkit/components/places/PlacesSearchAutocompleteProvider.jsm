







"use strict";

this.EXPORTED_SYMBOLS = [ "PlacesSearchAutocompleteProvider" ];

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const SEARCH_ENGINE_TOPIC = "browser-search-engine-modified";

const SearchAutocompleteProviderInternal = {
  


  priorityMatches: null,

  


  defaultMatch: null,

  initialize: function () {
    return new Promise((resolve, reject) => {
      Services.search.init(status => {
        if (!Components.isSuccessCode(status)) {
          reject(new Error("Unable to initialize search service."));
        }

        try {
          
          this._refresh();

          Services.obs.addObserver(this, SEARCH_ENGINE_TOPIC, true);

          this.initialized = true;
          resolve();
        } catch (ex) {
          reject(ex);
        }
      });
    });
  },

  initialized: false,

  observe: function (subject, topic, data) {
    switch (data) {
      case "engine-added":
      case "engine-changed":
      case "engine-removed":
        this._refresh();
    }
  },

  _refresh: function () {
    this.priorityMatches = [];
    this.defaultMatch = null;

    let currentEngine = Services.search.currentEngine;
    
    if (currentEngine) {
      this.defaultMatch = {
        engineName: currentEngine.name,
        iconUrl: currentEngine.iconURI ? currentEngine.iconURI.spec : null,
      }
    }

    
    
    Services.search.getVisibleEngines().forEach(e => this._addEngine(e));
  },

  _addEngine: function (engine) {
    let token = engine.getResultDomain();
    if (!token) {
      return;
    }

    this.priorityMatches.push({
      token: token,
      
      
      url: engine.searchForm,
      engineName: engine.name,
      iconUrl: engine.iconURI ? engine.iconURI.spec : null,
    });
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),
}

let gInitializationPromise = null;

this.PlacesSearchAutocompleteProvider = Object.freeze({
  




  ensureInitialized: function () {
    if (!gInitializationPromise) {
      gInitializationPromise = SearchAutocompleteProviderInternal.initialize();
    }
    return gInitializationPromise;
  },

  















  findMatchByToken: Task.async(function* (searchToken) {
    yield this.ensureInitialized();

    
    
    return SearchAutocompleteProviderInternal.priorityMatches
                 .find(m => m.token.startsWith(searchToken));
  }),

  getDefaultMatch: Task.async(function* () {
    yield this.ensureInitialized();

    return SearchAutocompleteProviderInternal.defaultMatch;
  }),

  



















  parseSubmissionURL: function (url) {
    if (!SearchAutocompleteProviderInternal.initialized) {
      throw new Error("The component has not been initialized.");
    }

    let parseUrlResult = Services.search.parseSubmissionURL(url);
    return parseUrlResult.engine && {
      engineName: parseUrlResult.engine.name,
      terms: parseUrlResult.terms,
    };
  },
});

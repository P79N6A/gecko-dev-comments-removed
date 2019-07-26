



"use strict";

this.EXPORTED_SYMBOLS = [ "PriorityUrlProvider" ];

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Task.jsm");






const SEARCH_ENGINE_TOPIC = "browser-search-engine-modified";

let SearchEnginesProvider = {
  init: function () {
    this._engines = new Map();
    let deferred = Promise.defer();
    Services.search.init(rv => {
      if (Components.isSuccessCode(rv)) {
        Services.search.getVisibleEngines().forEach(this._addEngine, this);
        deferred.resolve();
      } else {
        deferred.reject(new Error("Unable to initialize search service."));
      }
    });
    Services.obs.addObserver(this, SEARCH_ENGINE_TOPIC, true);
    return deferred.promise;
  },

  observe: function (engine, topic, verb) {
    let engine = engine.QueryInterface(Ci.nsISearchEngine);
    switch (verb) {
      case "engine-added":
        this._addEngine(engine);
        break;
      case "engine-changed":
        if (engine.hidden) {
          this._removeEngine(engine);
        } else {
          this._addEngine(engine);
        }
        break;
      case "engine-removed":
        this._removeEngine(engine);
        break;
    }
  },

  _addEngine: function (engine) {
    if (this._engines.has(engine.name)) {
      return;
    }
    let token = engine.getResultDomain();
    if (!token) {
      return;
    }
    let match = { token: token,
                  
                  
                  url: engine.searchForm,
                  title: engine.name,
                  iconUrl: engine.iconURI ? engine.iconURI.spec : null,
                  reason: "search" }
    this._engines.set(engine.name, match);
    PriorityUrlProvider.addMatch(match);
  },

  _removeEngine: function (engine) {
    if (!this._engines.has(engine.name)) {
      return;
    }
    this._engines.delete(engine.name);
    PriorityUrlProvider.removeMatchByToken(engine.getResultDomain());
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference])
}












let matches = new Map();

let initialized = false;
function promiseInitialized() {
  if (initialized) {
    return Promise.resolve();
  }
  return Task.spawn(function* () {
    try {
      yield SearchEnginesProvider.init();
    } catch (ex) {
      Cu.reportError(ex);
    }
    initialized = true;
  });
}

this.PriorityUrlProvider = Object.freeze({
  addMatch: function (match) {
    matches.set(match.token, match);
  },

  removeMatchByToken: function (token) {
    matches.delete(token);
  },

  getMatch: function (searchToken) {
    return Task.spawn(function* () {
      yield promiseInitialized();
      for (let [token, match] of matches.entries()) {
        
        
        if (token.startsWith(searchToken)) {
          return match;
        }
      }
      return null;
    }.bind(this));
  }
});

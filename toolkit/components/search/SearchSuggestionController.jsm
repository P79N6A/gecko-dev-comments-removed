



"use strict";

this.EXPORTED_SYMBOLS = ["SearchSuggestionController"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NS_ASSERT", "resource://gre/modules/debug.js");

const SEARCH_RESPONSE_SUGGESTION_JSON = "application/x-suggestions+json";
const DEFAULT_FORM_HISTORY_PARAM      = "searchbar-history";
const HTTP_OK            = 200;
const REMOTE_TIMEOUT     = 500; 














this.SearchSuggestionController = function SearchSuggestionController(callback = null) {
  this._callback = callback;
};

this.SearchSuggestionController.prototype = {
  


  maxLocalResults: 7,

  


  maxRemoteResults: 10,

  


  remoteTimeout: REMOTE_TIMEOUT,

  


  formHistoryParam: DEFAULT_FORM_HISTORY_PARAM,

  
  



  _formHistoryResult: null,

  



  _remoteResultTimer: null,

  


  _deferredRemoteResult: null,

  


  _callback: null,

  


  _request: null,

  

  









  fetch: function(searchTerm, privateMode, engine) {
    
    
    

    this.stop();

    if (!Services.search.isInitialized) {
      throw new Error("Search not initialized yet (how did you get here?)");
    }
    if (typeof privateMode === "undefined") {
      throw new Error("The privateMode argument is required to avoid unintentional privacy leaks");
    }
    if (!(engine instanceof Ci.nsISearchEngine)) {
      throw new Error("Invalid search engine");
    }
    if (!this.maxLocalResults && !this.maxRemoteResults) {
      throw new Error("Zero results expected, what are you trying to do?");
    }
    if (this.maxLocalResults < 0 || this.remoteResult < 0) {
      throw new Error("Number of requested results must be positive");
    }

    
    let promises = [];
    this._searchString = searchTerm;

    
    if (searchTerm && this.maxRemoteResults &&
        engine.supportsResponseType(SEARCH_RESPONSE_SUGGESTION_JSON)) {
      this._deferredRemoteResult = this._fetchRemote(searchTerm, engine, privateMode);
      promises.push(this._deferredRemoteResult.promise);
    }

    
    if (this.maxLocalResults) {
      let deferredHistoryResult = this._fetchFormHistory(searchTerm);
      promises.push(deferredHistoryResult.promise);
    }

    function handleRejection(reason) {
      if (reason == "HTTP request aborted") {
        
        return null;
      }
      Cu.reportError("SearchSuggestionController rejection: " + reason);
      return null;
    }
    return Promise.all(promises).then(this._dedupeAndReturnResults.bind(this), handleRejection);
  },

  






  stop: function() {
    if (this._request) {
      this._request.abort();
    } else if (!this.maxRemoteResults) {
      Cu.reportError("SearchSuggestionController: Cannot stop fetching if remote results were not "+
                     "requested");
    }
    this._reset();
  },

  

  _fetchFormHistory: function(searchTerm) {
    let deferredFormHistory = Promise.defer();

    let acSearchObserver = {
      
      onSearchResult: (search, result) => {
        this._formHistoryResult = result;

        if (this._request) {
          this._remoteResultTimer = Cc["@mozilla.org/timer;1"].
                                    createInstance(Ci.nsITimer);
          this._remoteResultTimer.initWithCallback(this._onRemoteTimeout.bind(this),
                                                   this.remoteTimeout || REMOTE_TIMEOUT,
                                                   Ci.nsITimer.TYPE_ONE_SHOT);
        }

        switch (result.searchResult) {
          case Ci.nsIAutoCompleteResult.RESULT_SUCCESS:
          case Ci.nsIAutoCompleteResult.RESULT_NOMATCH:
            if (result.searchString !== this._searchString) {
              deferredFormHistory.resolve("Unexpected response, this._searchString does not match form history response");
              return;
            }
            let fhEntries = [];
            let maxHistoryItems = Math.min(result.matchCount, this.maxLocalResults);
            for (let i = 0; i < maxHistoryItems; ++i) {
              fhEntries.push(result.getValueAt(i));
            }
            deferredFormHistory.resolve({
              result: fhEntries,
              formHistoryResult: result,
            });
            break;
          case Ci.nsIAutoCompleteResult.RESULT_FAILURE:
          case Ci.nsIAutoCompleteResult.RESULT_IGNORED:
            deferredFormHistory.resolve("Form History returned RESULT_FAILURE or RESULT_IGNORED");
            break;
        }
      },
    };

    let formHistory = Cc["@mozilla.org/autocomplete/search;1?name=form-history"].
                      createInstance(Ci.nsIAutoCompleteSearch);
    formHistory.startSearch(searchTerm, this.formHistoryParam || DEFAULT_FORM_HISTORY_PARAM,
                            this._formHistoryResult,
                            acSearchObserver);
    return deferredFormHistory;
  },

  


  _fetchRemote: function(searchTerm, engine, privateMode) {
    let deferredResponse = Promise.defer();
    this._request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].
                    createInstance(Ci.nsIXMLHttpRequest);
    let submission = engine.getSubmission(searchTerm,
                                          SEARCH_RESPONSE_SUGGESTION_JSON);
    let method = (submission.postData ? "POST" : "GET");
    this._request.open(method, submission.uri.spec, true);
    if (this._request.channel instanceof Ci.nsIPrivateBrowsingChannel) {
      this._request.channel.setPrivate(privateMode);
    }
    this._request.mozBackgroundRequest = true; 

    this._request.addEventListener("load", this._onRemoteLoaded.bind(this, deferredResponse));
    this._request.addEventListener("error", (evt) => deferredResponse.resolve("HTTP error"));
    
    
    this._request.addEventListener("abort", (evt) => deferredResponse.reject("HTTP request aborted"));

    this._request.send(submission.postData);

    return deferredResponse;
  },

  




  _onRemoteLoaded: function(deferredResponse) {
    if (!this._request) {
      deferredResponse.resolve("Got HTTP response after the request was cancelled");
      return;
    }

    let status, serverResults;
    try {
      status = this._request.status;
    } catch (e) {
      
      deferredResponse.resolve("Unknown HTTP status: " + e);
      return;
    }

    if (status != HTTP_OK || this._request.responseText == "") {
      deferredResponse.resolve("Non-200 status or empty HTTP response: " + status);
      return;
    }

    try {
      serverResults = JSON.parse(this._request.responseText);
    } catch(ex) {
      deferredResponse.resolve("Failed to parse suggestion JSON: " + ex);
      return;
    }

    if (this._searchString !== serverResults[0]) {
      
      deferredResponse.resolve("Unexpected response, this._searchString does not match remote response");
      return;
    }
    let results = serverResults[1] || [];
    deferredResponse.resolve({ result: results });
  },

  


  _onRemoteTimeout: function () {
    this._request = null;

    
    
    this._remoteResultTimer = null;

    
    
    if (this._deferredRemoteResult) {
      this._deferredRemoteResult.resolve("HTTP Timeout");
      this._deferredRemoteResult = null;
    }
  },

  



  _dedupeAndReturnResults: function(suggestResults) {
    NS_ASSERT(this._searchString !== null, "this._searchString shouldn't be null when returning results");
    let results = {
      term: this._searchString,
      remote: [],
      local: [],
      formHistoryResult: null,
    };

    for (let result of suggestResults) {
      if (typeof result === "string") { 
        Cu.reportError("SearchSuggestionController: " + result);
      } else if (result.formHistoryResult) { 
        results.formHistoryResult = result.formHistoryResult;
        results.local = result.result || [];
      } else { 
        results.remote = result.result || [];
      }
    }

    
    
    if (results.remote.length && results.local.length) {
      for (let i = 0; i < results.local.length; ++i) {
        let term = results.local[i];
        let dupIndex = results.remote.indexOf(term);
        if (dupIndex != -1) {
          results.remote.splice(dupIndex, 1);
        }
      }
    }

    
    results.remote = results.remote.slice(0, this.maxRemoteResults);

    if (this._callback) {
      this._callback(results);
    }
    this._reset();

    return results;
  },

  _reset: function() {
    this._request = null;
    if (this._remoteResultTimer) {
      this._remoteResultTimer.cancel();
      this._remoteResultTimer = null;
    }
    this._deferredRemoteResult = null;
    this._searchString = null;
  },
};







this.SearchSuggestionController.engineOffersSuggestions = function(engine) {
 return engine.supportsResponseType(SEARCH_RESPONSE_SUGGESTION_JSON);
};

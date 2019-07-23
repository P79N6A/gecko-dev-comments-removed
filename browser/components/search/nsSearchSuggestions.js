






































const SEARCH_RESPONSE_SUGGESTION_JSON = "application/x-suggestions+json";

const BROWSER_SUGGEST_PREF = "browser.search.suggest.enabled";
const XPCOM_SHUTDOWN_TOPIC              = "xpcom-shutdown";
const NS_PREFBRANCH_PREFCHANGE_TOPIC_ID = "nsPref:changed";




const SEARCH_SUGGEST_CONTRACTID =
  "@mozilla.org/autocomplete/search;1?name=search-autocomplete";
const SEARCH_SUGGEST_CLASSNAME = "Remote Search Suggestions";
const SEARCH_SUGGEST_CLASSID =
  Components.ID("{aa892eb4-ffbf-477d-9f9a-06c995ae9f27}");

const SEARCH_BUNDLE = "chrome://browser/locale/search.properties";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const HTTP_OK                    = 200;
const HTTP_INTERNAL_SERVER_ERROR = 500;
const HTTP_BAD_GATEWAY           = 502;
const HTTP_SERVICE_UNAVAILABLE   = 503;







function SuggestAutoCompleteResult(searchString,
                                   searchResult,
                                   defaultIndex,
                                   errorDescription,
                                   results,
                                   comments,
                                   formHistoryResult) {
  this._searchString = searchString;
  this._searchResult = searchResult;
  this._defaultIndex = defaultIndex;
  this._errorDescription = errorDescription;
  this._results = results;
  this._comments = comments;
  this._formHistoryResult = formHistoryResult;
}
SuggestAutoCompleteResult.prototype = {
  



  _searchString: "",

  




  _searchResult: 0,

  



  _defaultIndex: 0,

  



  _errorDescription: "",

  



  _results: [],

  




  _comments: [],

  



  _formHistoryResult: null,

  


  get searchString() {
    return this._searchString;
  },

  






  get searchResult() {
    return this._searchResult;
  },

  


  get defaultIndex() {
    return this._defaultIndex;
  },

  


  get errorDescription() {
    return this._errorDescription;
  },

  


  get matchCount() {
    return this._results.length;
  },

  




  getValueAt: function(index) {
    return this._results[index];
  },

  




  getCommentAt: function(index) {
    return this._comments[index];
  },

  




  getStyleAt: function(index) {
    if (!this._comments[index])
      return null;  

    if (index == 0)
      return "suggestfirst";  

    return "suggesthint";   
  },

  



  removeValueAt: function(index, removeFromDatabase) {
    
    
    
    if (removeFromDatabase && this._formHistoryResult &&
        index < this._formHistoryResult.matchCount) {
      
      this._formHistoryResult.removeValueAt(index, true);
    }
    this._results.splice(index, 1);
    this._comments.splice(index, 1);
  },

  





  QueryInterface: function(iid) {
    if (!iid.equals(Ci.nsIAutoCompleteResult) &&
        !iid.equals(Ci.nsISupports))
      throw Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }
};









function SuggestAutoComplete() {
  this._init();
}
SuggestAutoComplete.prototype = {

  _init: function() {
    this._addObservers();
    this._loadSuggestPref();
  },

  


  get _strings() {
    if (!this.__strings) {
      var sbs = Cc["@mozilla.org/intl/stringbundle;1"].
                getService(Ci.nsIStringBundleService);

      this.__strings = sbs.createBundle(SEARCH_BUNDLE);
    }
    return this.__strings;
  },
  __strings: null,

  


  _loadSuggestPref: function SAC_loadSuggestPref() {
    var prefService = Cc["@mozilla.org/preferences-service;1"].
                      getService(Ci.nsIPrefBranch);
    this._suggestEnabled = prefService.getBoolPref(BROWSER_SUGGEST_PREF);
  },
  _suggestEnabled: null,

  




  



  _serverErrorLog: [],

  



  _maxErrorsBeforeBackoff: 3,

  




  _serverErrorPeriod: 600000,  

  



  _serverErrorTimeoutIncrement: 600000,  

  



  _serverErrorTimeout: 0,

  


  _nextRequestTime: 0,

  



  _serverErrorEngine: null,

  



  _request: null,

  




  _listener: null,

  



  _includeFormHistory: true,

  





  _sentSuggestRequest: false,

  




  notify: function SAC_notify(timer) {
    
    if ((timer != this._formHistoryTimer) || !this._listener)
      return;

    this._listener.onSearchResult(this, this._formHistoryResult);
    this._formHistoryTimer = null;
    this._reset();
  },

  



  _suggestionTimeout: 500,

  



  onSearchResult: function SAC_onSearchResult(search, result) {
    this._formHistoryResult = result;

    if (this._request) {
      
      
      this._formHistoryTimer = Cc["@mozilla.org/timer;1"].
                               createInstance(Ci.nsITimer);
      this._formHistoryTimer.initWithCallback(this, this._suggestionTimeout,
                                              Ci.nsITimer.TYPE_ONE_SHOT);
    } else if (!this._sentSuggestRequest) {
      
      this._listener.onSearchResult(this, this._formHistoryResult);
      this._reset();
    }
  },

  


  _suggestURI: null,

  


  _formHistoryResult: null,

  


  _formHistoryTimer: null,

  


  _reset: function SAC_reset() {
    
    
    if (!this._formHistoryTimer) {
      this._listener = null;
      this._formHistoryResult = null;
    }
    this._request = null;
  },

  



  _startHistorySearch: function SAC_SHSearch(searchString, searchParam, previousResult) {
    var formHistory =
      Cc["@mozilla.org/autocomplete/search;1?name=form-history"].
      createInstance(Ci.nsIAutoCompleteSearch);
    formHistory.startSearch(searchString, searchParam, previousResult, this);
  },

  



  _noteServerError: function SAC__noteServeError() {
    var currentTime = Date.now();

    this._serverErrorLog.push(currentTime);
    if (this._serverErrorLog.length > this._maxErrorsBeforeBackoff)
      this._serverErrorLog.shift();

    if ((this._serverErrorLog.length == this._maxErrorsBeforeBackoff) &&
        ((currentTime - this._serverErrorLog[0]) < this._serverErrorPeriod)) {
      
      this._serverErrorTimeout = (this._serverErrorTimeout * 2) +
                                 this._serverErrorTimeoutIncrement;
      this._nextRequestTime = currentTime + this._serverErrorTimeout;
    }
  },

  


  _clearServerErrors: function SAC__clearServerErrors() {
    this._serverErrorLog = [];
    this._serverErrorTimeout = 0;
    this._nextRequestTime = 0;
  },

  





  _okToRequest: function SAC__okToRequest() {
    return Date.now() > this._nextRequestTime;
  },

  







  _checkForEngineSwitch: function SAC__checkForEngineSwitch(engine) {
    if (engine == this._serverErrorEngine)
      return;

    
    this._serverErrorEngine = engine;
    this._clearServerErrors();
  },

  






  _isBackoffError: function SAC__isBackoffError(status) {
    return ((status == HTTP_INTERNAL_SERVER_ERROR) ||
            (status == HTTP_BAD_GATEWAY) ||
            (status == HTTP_SERVICE_UNAVAILABLE));
  },

  




  onReadyStateChange: function() {
    
    if (!this._request || this._request.readyState != 4)
      return;

    try {
      var status = this._request.status;
    } catch (e) {
      
      return;
    }

    if (this._isBackoffError(status)) {
      this._noteServerError();
      return;
    }

    var responseText = this._request.responseText;
    if (status != HTTP_OK || responseText == "")
      return;

    this._clearServerErrors();

    
    
    
    const JSON_STRING = /^("(\\.|[^"\\\n\r])*?"|[,:{}\[\]0-9.\-+Eaeflnr-u \n\r\t])+?$/;
    var sandbox = new Components.utils.Sandbox(this._suggestURI.prePath);
    function parseJSON(aString) {
      try {
        if (JSON_STRING.test(aString))
          return Components.utils.evalInSandbox("(" + aString + ")", sandbox);
      } catch (e) {}

      return [];
    };

    var serverResults = parseJSON(responseText);
    var searchString = serverResults[0] || "";
    var results = serverResults[1] || [];

    var comments = [];  
    var historyResults = [];
    var historyComments = [];

    
    if (this._includeFormHistory && this._formHistoryResult &&
        (this._formHistoryResult.searchResult ==
         Ci.nsIAutoCompleteResult.RESULT_SUCCESS)) {
      for (var i = 0; i < this._formHistoryResult.matchCount; ++i) {
        var term = this._formHistoryResult.getValueAt(i);

        
        var dupIndex = results.indexOf(term);
        if (dupIndex != -1)
          results.splice(dupIndex, 1);

        historyResults.push(term);
        historyComments.push("");
      }
    }

    
    for (var i = 0; i < results.length; ++i)
      comments.push("");

    
    if (comments.length > 0)
      comments[0] = this._strings.GetStringFromName("suggestion_label");

    
    var finalResults = historyResults.concat(results);
    var finalComments = historyComments.concat(comments);

    
    this.onResultsReady(searchString, finalResults, finalComments,
                        this._formHistoryResult);

    
    this._reset();
  },

  






  onResultsReady: function(searchString, results, comments,
                           formHistoryResult) {
    if (this._listener) {
      var result = new SuggestAutoCompleteResult(
          searchString,
          Ci.nsIAutoCompleteResult.RESULT_SUCCESS,
          0,
          "",
          results,
          comments,
          formHistoryResult);

      this._listener.onSearchResult(this, result);

      
      
      this._listener = null;
    }
  },

  













  startSearch: function(searchString, searchParam, previousResult, listener) {
    var searchService = Cc["@mozilla.org/browser/search-service;1"].
                        getService(Ci.nsIBrowserSearchService);

    
    
    
    
    
    this.stopSearch();

    this._listener = listener;

    var engine = searchService.currentEngine;

    this._checkForEngineSwitch(engine);

    if (!searchString ||
        !this._suggestEnabled ||
        !engine.supportsResponseType(SEARCH_RESPONSE_SUGGESTION_JSON) ||
        !this._okToRequest()) {
      
      
      
      
      this._sentSuggestRequest = false;
      this._startHistorySearch(searchString, searchParam, previousResult);
      return;
    }

    
    this._request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].
                    createInstance(Ci.nsIXMLHttpRequest);
    var submission = engine.getSubmission(searchString,
                                          SEARCH_RESPONSE_SUGGESTION_JSON);
    this._suggestURI = submission.uri;
    var method = (submission.postData ? "POST" : "GET");
    this._request.open(method, this._suggestURI.spec, true);

    var self = this;
    function onReadyStateChange() {
      self.onReadyStateChange();
    }
    this._request.onreadystatechange = onReadyStateChange;
    this._request.send(submission.postData);

    if (this._includeFormHistory) {
      this._sentSuggestRequest = true;
      this._startHistorySearch(searchString, searchParam, previousResult);
    }
  },

  



  stopSearch: function() {
    if (this._request) {
      this._request.abort();
      this._reset();
    }
  },

  


  observe: function SAC_observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case NS_PREFBRANCH_PREFCHANGE_TOPIC_ID:
        this._loadSuggestPref();
        break;
      case XPCOM_SHUTDOWN_TOPIC:
        this._removeObservers();
        break;
    }
  },

  _addObservers: function SAC_addObservers() {
    var prefService2 = Cc["@mozilla.org/preferences-service;1"].
                       getService(Ci.nsIPrefBranch2);
    prefService2.addObserver(BROWSER_SUGGEST_PREF, this, false);

    var os = Cc["@mozilla.org/observer-service;1"].
             getService(Ci.nsIObserverService);
    os.addObserver(this, XPCOM_SHUTDOWN_TOPIC, false);
  },

  _removeObservers: function SAC_removeObservers() {
    var prefService2 = Cc["@mozilla.org/preferences-service;1"].
                       getService(Ci.nsIPrefBranch2);
    prefService2.removeObserver(BROWSER_SUGGEST_PREF, this);

    var os = Cc["@mozilla.org/observer-service;1"].
             getService(Ci.nsIObserverService);
    os.removeObserver(this, XPCOM_SHUTDOWN_TOPIC);
  },

  





  QueryInterface: function(iid) {
    if (!iid.equals(Ci.nsIAutoCompleteSearch) &&
        !iid.equals(Ci.nsIAutoCompleteObserver) &&
        !iid.equals(Ci.nsISupports))
      throw Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }
};






function SearchSuggestAutoComplete() {
  
  
  this._init();
}
SearchSuggestAutoComplete.prototype = {
  __proto__: SuggestAutoComplete.prototype,
  serviceURL: ""
};

var gModule = {
  







  registerSelf: function(componentManager, location, loaderString, type) {
    if (this._firstTime) {
      this._firstTime = false;
      throw Cr.NS_ERROR_FACTORY_REGISTER_AGAIN;
    }
    componentManager =
      componentManager.QueryInterface(Ci.nsIComponentRegistrar);

    for (var key in this.objects) {
      var obj = this.objects[key];
      componentManager.registerFactoryLocation(obj.CID, obj.className, obj.contractID,
                                               location, loaderString, type);
    }
  },

  







  getClassObject: function(componentManager, cid, iid) {
    if (!iid.equals(Ci.nsIFactory))
      throw Cr.NS_ERROR_NOT_IMPLEMENTED;

    for (var key in this.objects) {
      if (cid.equals(this.objects[key].CID))
        return this.objects[key].factory;
    }

    throw Cr.NS_ERROR_NO_INTERFACE;
  },

  




  _makeFactory: function(constructor) {
    function createInstance(outer, iid) {
      if (outer != null)
        throw Cr.NS_ERROR_NO_AGGREGATION;
      return (new constructor()).QueryInterface(iid);
    }
    return { createInstance: createInstance };
  },

  



  canUnload: function(componentManager) {
    return true;
  }
};







function NSGetModule(componentManager, location) {
  
  gModule.objects = {
    search: {
      CID: SEARCH_SUGGEST_CLASSID,
      contractID: SEARCH_SUGGEST_CONTRACTID,
      className: SEARCH_SUGGEST_CLASSNAME,
      factory: gModule._makeFactory(SearchSuggestAutoComplete)
    },
  };
  return gModule;
}





const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/nsFormAutoCompleteResult.jsm");
Cu.import("resource://gre/modules/Services.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "SearchSuggestionController",
                                  "resource://gre/modules/SearchSuggestionController.jsm");









function SuggestAutoComplete() {
  this._init();
}
SuggestAutoComplete.prototype = {

  _init: function() {
    this._suggestionController = new SearchSuggestionController(obj => this.onResultsReturned(obj));
  },

  get _suggestionLabel() {
    delete this._suggestionLabel;
    let bundle = Services.strings.createBundle("chrome://global/locale/search/search.properties");
    return this._suggestionLabel = bundle.GetStringFromName("suggestion_label");
  },

  




  _listener: null,

  





  _historyLimit: 7,

  



  onResultsReturned: function(results) {
    let finalResults = [];
    let finalComments = [];

    
    let maxHistoryItems = Math.min(results.local.length, this._historyLimit);
    for (let i = 0; i < maxHistoryItems; ++i) {
      finalResults.push(results.local[i]);
      finalComments.push("");
    }

    
    if (results.remote.length) {
      
      let comments = new Array(results.remote.length).fill("", 1);
      comments[0] = this._suggestionLabel;
      
      finalResults = finalResults.concat(results.remote);
      finalComments = finalComments.concat(comments);
    }

    
    this.onResultsReady(results.term, finalResults, finalComments, results.formHistoryResult);
  },

  






  onResultsReady: function(searchString, results, comments, formHistoryResult) {
    if (this._listener) {
      let result = new FormAutoCompleteResult(
          searchString,
          Ci.nsIAutoCompleteResult.RESULT_SUCCESS,
          0,
          "",
          results,
          results,
          comments,
          formHistoryResult);

      this._listener.onSearchResult(this, result);

      
      this._listener = null;
    }
  },

  













  startSearch: function(searchString, searchParam, previousResult, listener) {
    
    if (!previousResult)
      this._formHistoryResult = null;

    var formHistorySearchParam = searchParam.split("|")[0];

    
    
    
    
    
    
    
    var privacyMode = (searchParam.split("|")[1] == "private");

    
    
    if (Services.search.isInitialized) {
      this._triggerSearch(searchString, formHistorySearchParam, listener, privacyMode);
      return;
    }

    Services.search.init((function startSearch_cb(aResult) {
      if (!Components.isSuccessCode(aResult)) {
        Cu.reportError("Could not initialize search service, bailing out: " + aResult);
        return;
      }
      this._triggerSearch(searchString, formHistorySearchParam, listener, privacyMode);
    }).bind(this));
  },

  


  _triggerSearch: function(searchString, searchParam, listener, privacyMode) {
    this._listener = listener;
    this._suggestionController.fetch(searchString,
                                     privacyMode,
                                     Services.search.currentEngine);
  },

  



  stopSearch: function() {
    this._suggestionController.stop();
  },

  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAutoCompleteSearch,
                                         Ci.nsIAutoCompleteObserver])
};






function SearchSuggestAutoComplete() {
  
  
  this._init();
}
SearchSuggestAutoComplete.prototype = {
  classID: Components.ID("{aa892eb4-ffbf-477d-9f9a-06c995ae9f27}"),
  __proto__: SuggestAutoComplete.prototype,
  serviceURL: ""
};

var component = [SearchSuggestAutoComplete];
this.NSGetFactory = XPCOMUtils.generateNSGetFactory(component);

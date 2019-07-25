

















function AutoCompleteInput(aSearches) {
  this.searches = aSearches;
}
AutoCompleteInput.prototype = {
  constructor: AutoCompleteInput, 
  
  
  searches: null,
  
  minResultsForPopup: 0,
  timeout: 10,
  searchParam: "",
  textValue: "",
  disableAutoComplete: false,  
  completeDefaultIndex: false,
  
  get searchCount() {
    return this.searches.length;
  },
  
  getSearchAt: function(aIndex) {
    return this.searches[aIndex];
  },
  
  onSearchBegin: function() {},
  onSearchComplete: function() {},
  
  popupOpen: false,  
  
  popup: { 
    setSelectedIndex: function(aIndex) {},
    invalidate: function() {},

    
    QueryInterface: function(iid) {
      if (iid.equals(Ci.nsISupports) ||
          iid.equals(Ci.nsIAutoCompletePopup))
        return this;

      throw Components.results.NS_ERROR_NO_INTERFACE;
    }    
  },
    
  
  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsISupports) ||
        iid.equals(Ci.nsIAutoCompleteInput))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
}






function AutoCompleteResult(aValues, aComments, aStyles) {
  this._values = aValues;
  this._comments = aComments;
  this._styles = aStyles;
  
  if (this._values.length > 0) {
    this.searchResult = Ci.nsIAutoCompleteResult.RESULT_SUCCESS;
  } else {
    this.searchResult = Ci.nsIAutoCompleteResult.NOMATCH;
  }
}
AutoCompleteResult.prototype = {
  constructor: AutoCompleteResult,
  
  
  _values: null,
  _comments: null,
  _styles: null,
  
  searchString: "",
  searchResult: null,
  
  defaultIndex: 0,

  get matchCount() {
    return this._values.length;
  },

  getValueAt: function(aIndex) {
    return this._values[aIndex];
  },

  getLabelAt: function(aIndex) {
    return this.getValueAt(aIndex);
  },
  
  getCommentAt: function(aIndex) {
    return this._comments[aIndex];
  },
  
  getStyleAt: function(aIndex) {
    return this._styles[aIndex];
  },
  
  getImageAt: function(aIndex) {
    return "";
  },

  removeValueAt: function (aRowIndex, aRemoveFromDb) {},

  
  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsISupports) ||
        iid.equals(Ci.nsIAutoCompleteResult))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  }  
}







function AutoCompleteSearch(aName, aResult) {
  this.name = aName;
}
AutoCompleteSearch.prototype = {
  constructor: AutoCompleteSearch,
  
  
  name: null,

  
  _result:null,  
  
  
  


  startSearch: function(aSearchString, 
                        aSearchParam, 
                        aPreviousResult, 
                        aListener) 
  {
    aListener.onSearchResult(this, this._result);
  },
  
  stopSearch: function() {},

  
  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsISupports) ||
        iid.equals(Ci.nsIFactory) ||
        iid.equals(Ci.nsIAutoCompleteSearch))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  },
  
  
  createInstance: function(outer, iid) {
    return this.QueryInterface(iid);
  }
}







function registerAutoCompleteSearch(aSearch) {
  var name = "@mozilla.org/autocomplete/search;1?name=" + aSearch.name;

  var uuidGenerator = Cc["@mozilla.org/uuid-generator;1"].
                      getService(Ci.nsIUUIDGenerator);
  var cid = uuidGenerator.generateUUID();

  var desc = "Test AutoCompleteSearch";
  
  var componentManager = Components.manager
                                   .QueryInterface(Ci.nsIComponentRegistrar);
  componentManager.registerFactory(cid, desc, name, aSearch);

  
  aSearch.cid = cid; 
}






function unregisterAutoCompleteSearch(aSearch) {
  var componentManager = Components.manager
                                   .QueryInterface(Ci.nsIComponentRegistrar);  
  componentManager.unregisterFactory(aSearch.cid, aSearch);
}






function run_test() {
  
  
  var emptySearch = new AutoCompleteSearch("test-empty-search", 
                             new AutoCompleteResult([], [], []));
  
  
  registerAutoCompleteSearch(emptySearch);
    
  var controller = Components.classes["@mozilla.org/autocomplete/controller;1"].
                   getService(Components.interfaces.nsIAutoCompleteController);  
  
  
  
  var input = new AutoCompleteInput([emptySearch.name]);
  var numSearchesStarted = 0;

  input.onSearchBegin = function() {
    numSearchesStarted++;
    do_check_eq(numSearchesStarted, 1);
  };

  input.onSearchComplete = function() {

    do_check_eq(numSearchesStarted, 1);

    do_check_eq(controller.searchStatus, 
                Ci.nsIAutoCompleteController.STATUS_COMPLETE_NO_MATCH);
    do_check_eq(controller.matchCount, 0);

    
    unregisterAutoCompleteSearch(emptySearch);

    do_test_finished();
  };

  controller.input = input;

  
  do_test_pending();
  
  controller.startSearch("test");
}


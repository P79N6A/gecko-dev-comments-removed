





































const Cc = Components.classes;
const Ci = Components.interfaces;







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
  this._result = aResult;
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
    var result = this._result;
    if (result._values.length > 0) {
      result.searchResult = Ci.nsIAutoCompleteResult.RESULT_SUCCESS_ONGOING;
    } else {
      result.searchResult = Ci.nsIAutoCompleteResult.RESULT_NOMATCH_ONGOING;
    }
    aListener.onSearchResult(this, result);

    if (result._values.length > 0) {
      result.searchResult = Ci.nsIAutoCompleteResult.RESULT_SUCCESS;
    } else {
      result.searchResult = Ci.nsIAutoCompleteResult.RESULT_NOMATCH;
    }
    aListener.onSearchResult(this, result);
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
  var expected1 = ["1","2","3"];
  var expected2 = ["a","b","c"];
  var search1 = new AutoCompleteSearch("search1", 
                             new AutoCompleteResult(expected1, [], []));
  var search2 = new AutoCompleteSearch("search2",
                             new AutoCompleteResult(expected2, [], []));
  
  
  registerAutoCompleteSearch(search1);
  registerAutoCompleteSearch(search2);
    
  var controller = Components.classes["@mozilla.org/autocomplete/controller;1"].
                   getService(Components.interfaces.nsIAutoCompleteController);  
  
  
  
  var input = new AutoCompleteInput([search1.name, search2.name]);
  input.onSearchComplete = function() {

    do_check_eq(controller.searchStatus, 
                Ci.nsIAutoCompleteController.STATUS_COMPLETE_MATCH);
    do_check_eq(controller.matchCount, expected1.length + expected2.length);

    
    unregisterAutoCompleteSearch(search1);
    unregisterAutoCompleteSearch(search2);

    do_test_finished();
  };

  controller.input = input;

  
  do_test_pending();
  
  controller.startSearch("test");
}

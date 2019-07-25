


Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;







function AutoCompleteInputBase(aSearches) {
  this.searches = aSearches;
}
AutoCompleteInputBase.prototype = {
 
  
  searches: null,
  
  minResultsForPopup: 0,
  timeout: 10,
  searchParam: "",
  textValue: "",
  disableAutoComplete: false,  
  completeDefaultIndex: false,

  
  _selStart: 0,
  _selEnd: 0,
  get selectionStart() {
    return this._selStart;
  },
  get selectionEnd() {
    return this._selEnd;
  },
  selectTextRange: function(aStart, aEnd) {
    this._selStart = aStart;
    this._selEnd = aEnd;
  },
  
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
    selectedIndex: 0,
    invalidate: function() {},

    
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIAutoCompletePopup])   
  },
    
  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAutoCompleteInput])
}




function AutoCompleteResultBase(aValues) {
  this._values = aValues;
}
AutoCompleteResultBase.prototype = {
  
  
  _values: null,
  _comments: [],
  _styles: [],
  
  searchString: "",
  searchResult: null,
  
  defaultIndex: -1,
  
  _typeAheadResult: false,
  get typeAheadResult() {
    return this._typeAheadResult;
  },

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

  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAutoCompleteResult])
}





function AutoCompleteSearchBase(aName, aResult) {
  this.name = aName;
  this._result = aResult;
}
AutoCompleteSearchBase.prototype = {
  
  
  name: null,

  
  _result: null,

  startSearch: function(aSearchString, 
                        aSearchParam, 
                        aPreviousResult, 
                        aListener) {
    var result = this._result;

    result.searchResult = Ci.nsIAutoCompleteResult.RESULT_SUCCESS;
    aListener.onSearchResult(this, result);
  },
  
  stopSearch: function() {},

  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIFactory,
                                         Ci.nsIAutoCompleteSearch]),
  
  
  createInstance: function(outer, iid) {
    return this.QueryInterface(iid);
  }
}





function registerAutoCompleteSearch(aSearch) {
  var name = "@mozilla.org/autocomplete/search;1?name=" + aSearch.name;
  var cid = Cc["@mozilla.org/uuid-generator;1"].
            getService(Ci.nsIUUIDGenerator).
            generateUUID();

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


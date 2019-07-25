


Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

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
  completeDefaultIndex: true,

  
  selStart: 0,
  selEnd: 0,
  get selectionStart() {
    return selStart;
  },
  get selectionEnd() {
    return selEnd;
  },
  selectTextRange: function(aStart, aEnd) {
    selStart = aStart;
    selEnd = aEnd;
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
    setSelectedIndex: function(aIndex) {},
    invalidate: function() {},

    
    QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports,
                                           Ci.nsIAutoCompletePopup])
  },
    
  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports,
                                         Ci.nsIAutoCompleteInput])
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
  searchResult: Ci.nsIAutoCompleteResult.RESULT_SUCCESS,
  
  defaultIndex: 0,
  isURLResult: true,

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

  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports,
                                         Ci.nsIAutoCompleteResult])
}






function AutoCompleteSearch(aName, aResult) {
  this.name = aName;
  this._result = aResult;
}
AutoCompleteSearch.prototype = {
  constructor: AutoCompleteSearch,
  
  
  name: null,

  
  _result: null,

  
  


  startSearch: function(aSearchString, 
                        aSearchParam, 
                        aPreviousResult, 
                        aListener) 
  {
    aListener.onSearchResult(this, this._result);
  },
  
  stopSearch: function() {},

  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports,
                                         Ci.nsIFactory,
                                         Ci.nsIAutoCompleteSearch]),
  
  
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





var tests = [
  {
    searchValues: ["mozilla.org"],        
    inputString: "moz",                   
    expectedAutocomplete: "mozilla.org",  
    expectedSelStart: 3,                  
    expectedSelEnd: 11
  },
  {
    
    searchValues: ["http://www.mozilla.org", "mozNotFirstMatch.org"],
    inputString: "moz",
    expectedAutocomplete: "mozilla.org",
    expectedSelStart: 3,
    expectedSelEnd: 11
  },
  {
    
    searchValues: ["ftp://ftp.mozilla.org/"],
    inputString: "ft",
    expectedAutocomplete: "ftp.mozilla.org/",
    expectedSelStart: 2,
    expectedSelEnd: 16
  },
  {
    
    searchValues: ["moz-action:someaction,http://www.mozilla.org", "mozNotFirstMatch.org"],
    inputString: "moz",
    expectedAutocomplete: "mozilla.org",
    expectedSelStart: 3,
    expectedSelEnd: 11
  },
  {
    
    searchValues: ["unimportantTLD.org/moz", "mozilla.org"],
    inputString: "moz",
    expectedAutocomplete: "mozilla.org",
    expectedSelStart: 3,
    expectedSelEnd: 11
  },
  {
    
    searchValues: ["http://mozilla.org/credits/morecredits"],
    inputString: "moz",
    expectedAutocomplete: "mozilla.org/",
    expectedSelStart: 3,
    expectedSelEnd: 12
  },
  {
    
    searchValues: ["http://mozilla.org/credits/morecredits"],
    inputString: "mozilla.org/cr",
    expectedAutocomplete: "mozilla.org/credits/",
    expectedSelStart: 14,
    expectedSelEnd: 20
  },
  {
    
    searchValues: ["http://mozilla.org/credits#VENTNOR"],
    inputString: "mozilla.org/cr",
    expectedAutocomplete: "mozilla.org/credits",
    expectedSelStart: 14,
    expectedSelEnd: 19
  },
  {
    
    searchValues: ["http://mozilla.org/credits?mozilla=awesome"],
    inputString: "mozilla.org/cr",
    expectedAutocomplete: "mozilla.org/credits",
    expectedSelStart: 14,
    expectedSelEnd: 19
  },
  {
    
    searchValues: ["http://www.mozilla.org/credits"],
    inputString: "http://mozi",
    expectedAutocomplete: "http://mozilla.org/",
    expectedSelStart: 11,
    expectedSelEnd: 19
  },
];





function run_search() {
  if (tests.length == 0) {
    do_test_finished();
    return;
  }

  var test = tests.shift();

  var search = new AutoCompleteSearch("test-autofill1",
    new AutoCompleteResult(test.searchValues, ["", ""], ["", ""]));

  
  registerAutoCompleteSearch(search);

  var controller = Cc["@mozilla.org/autocomplete/controller;1"].
                   getService(Ci.nsIAutoCompleteController);

  
  
  var input = new AutoCompleteInput([search.name]);
  input.textValue = test.inputString;

  
  
  var strLen = test.inputString.length;
  input.selectTextRange(strLen, strLen);

  input.onSearchComplete = function() {
    do_check_eq(input.textValue, test.expectedAutocomplete);
    do_check_eq(input.selectionStart, test.expectedSelStart);
    do_check_eq(input.selectionEnd, test.expectedSelEnd);

    
    unregisterAutoCompleteSearch(search);
    run_search();
  };

  controller.input = input;
  controller.startSearch(test.inputString);
}




function run_test() {
  
  do_test_pending();
  run_search();
}


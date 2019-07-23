







































var current_test = 0;

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


try {
  var tagssvc = Cc["@mozilla.org/browser/tagging-service;1"].
                getService(Ci.nsITaggingService);
} catch(ex) {
  do_throw("Could not get tagging service\n");
}

function ensure_tag_results(results, searchTerm)
{
  var controller = Cc["@mozilla.org/autocomplete/controller;1"].
                   getService(Ci.nsIAutoCompleteController);  
  
  
  
  var input = new AutoCompleteInput(["places-tag-autocomplete"]);

  controller.input = input;

  var numSearchesStarted = 0;
  input.onSearchBegin = function input_onSearchBegin() {
    numSearchesStarted++;
    do_check_eq(numSearchesStarted, 1);
  };

  input.onSearchComplete = function input_onSearchComplete() {
    do_check_eq(numSearchesStarted, 1);
    if (results.length)
      do_check_eq(controller.searchStatus, 
                  Ci.nsIAutoCompleteController.STATUS_COMPLETE_MATCH);
    else
      do_check_eq(controller.searchStatus, 
                  Ci.nsIAutoCompleteController.STATUS_COMPLETE_NO_MATCH);

    do_check_eq(controller.matchCount, results.length);
    for (var i=0; i<controller.matchCount; i++) {
      do_check_eq(controller.getValueAt(i), results[i]);
    }
   
    if (current_test < (tests.length - 1)) {
      current_test++;
      tests[current_test]();
    }
    else {
      
      do_test_finished();
    }
  };

  controller.startSearch(searchTerm);
}

var uri1 = uri("http://site.tld/1");
  
var tests = [
  function test1() { ensure_tag_results(["bar", "baz", "boo"], "b"); },
  function test2() { ensure_tag_results(["bar", "baz"], "ba"); },
  function test3() { ensure_tag_results(["bar"], "bar"); }, 
  function test4() { ensure_tag_results([], "barb"); }, 
  function test5() { ensure_tag_results([], "foo"); },
  function test6() { ensure_tag_results(["first tag, bar", "first tag, baz"], "first tag, ba"); },
  function test7() { ensure_tag_results(["first tag;  bar", "first tag;  baz"], "first tag;  ba"); }
];




function run_test() {
  
  do_test_pending();

  tagssvc.tagURI(uri1, ["bar", "baz", "boo", "*nix"]);

  tests[0]();
}

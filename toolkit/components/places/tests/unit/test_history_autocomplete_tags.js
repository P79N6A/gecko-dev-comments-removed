






































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

function ensure_tag_results(uris, searchTerm)
{
  var controller = Components.classes["@mozilla.org/autocomplete/controller;1"].
                   getService(Components.interfaces.nsIAutoCompleteController);  
  
  
  
  var input = new AutoCompleteInput(["history"]);

  controller.input = input;

  
  do_test_pending();

  var numSearchesStarted = 0;
  input.onSearchBegin = function() {
    numSearchesStarted++;
    do_check_eq(numSearchesStarted, 1);
  };

  input.onSearchComplete = function() {
    do_check_eq(numSearchesStarted, 1);
    do_check_eq(controller.searchStatus, 
                Ci.nsIAutoCompleteController.STATUS_COMPLETE_MATCH);
    do_check_eq(controller.matchCount, uris.length);
    for (var i=0; i<controller.matchCount; i++) {
      do_check_eq(controller.getValueAt(i), uris[i].spec);
      do_check_eq(controller.getStyleAt(i), "tag");
    }
   
    if (current_test < (tests.length - 1)) {
      current_test++;
      tests[current_test]();
    }

    do_test_finished();
  };

  controller.startSearch(searchTerm);
}

var uri1 = uri("http://site.tld/1");
var uri2 = uri("http://site.tld/2");
var uri3 = uri("http://site.tld/3");
var uri4 = uri("http://site.tld/4");
var uri5 = uri("http://site.tld/5");
var uri6 = uri("http://site.tld/6");
  
var tests = [function() { ensure_tag_results([uri1], "foo"); }, 
             function() { ensure_tag_results([uri2], "bar"); }, 
             function() { ensure_tag_results([uri3], "cheese"); }, 
             function() { ensure_tag_results([uri1, uri2, uri4], "foo bar"); }, 
             function() { ensure_tag_results([uri1, uri2], "bar foo"); }, 
             function() { ensure_tag_results([uri2, uri3, uri5], "bar cheese"); }, 
             function() { ensure_tag_results([uri2, uri3], "cheese bar"); }, 
             function() { ensure_tag_results([uri1, uri2, uri3, uri4, uri5, uri6], "foo bar cheese"); }];




function run_test() {
  tagssvc.tagURI(uri1, ["foo"]);
  tagssvc.tagURI(uri2, ["bar"]);
  tagssvc.tagURI(uri3, ["cheese"]);
  tagssvc.tagURI(uri4, ["foo bar"]);
  tagssvc.tagURI(uri5, ["bar cheese"]);
  tagssvc.tagURI(uri6, ["foo bar cheese"]);

  tests[0]();
}

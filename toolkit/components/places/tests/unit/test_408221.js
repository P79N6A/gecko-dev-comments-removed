







































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
    let vals = [];
    for (var i=0; i<controller.matchCount; i++) {
      
      vals.push(controller.getValueAt(i));
      do_check_eq(controller.getStyleAt(i), "tag");
    }
    
    vals.sort().forEach(function(val, i) do_check_eq(val, uris[i].spec))
   
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
  
var tests = [function() { ensure_tag_results([uri1, uri2, uri3], "foo"); }, 
             function() { ensure_tag_results([uri1, uri2, uri3], "Foo"); }, 
             function() { ensure_tag_results([uri1, uri2, uri3], "foO"); },
             function() { ensure_tag_results([uri4, uri5, uri6], "bar mud"); },
             function() { ensure_tag_results([uri4, uri5, uri6], "BAR MUD"); },
             function() { ensure_tag_results([uri4, uri5, uri6], "Bar Mud"); }];




function run_test() {
  
  var prefs = Cc["@mozilla.org/preferences-service;1"].
              getService(Ci.nsIPrefBranch);
  prefs.setIntPref("browser.urlbar.search.sources", 3);

  tagssvc.tagURI(uri1, ["Foo"]);
  tagssvc.tagURI(uri2, ["FOO"]);
  tagssvc.tagURI(uri3, ["foO"]);
  tagssvc.tagURI(uri4, ["BAR"]);
  tagssvc.tagURI(uri4, ["MUD"]);
  tagssvc.tagURI(uri5, ["bar"]);
  tagssvc.tagURI(uri5, ["mud"]);
  tagssvc.tagURI(uri6, ["baR"]);
  tagssvc.tagURI(uri6, ["muD"]);

  tests[0]();
}





















































try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                getService(Ci.nsINavHistoryService);
  var bhist = histsvc.QueryInterface(Ci.nsIBrowserHistory);
  var ghist = Cc["@mozilla.org/browser/global-history;2"].
              getService(Ci.nsIGlobalHistory2);
  var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
              getService(Ci.nsINavBookmarksService);
  var prefs = Cc["@mozilla.org/preferences-service;1"].
              getService(Ci.nsIPrefBranch);
} catch(ex) {
  do_throw("Could not get services\n");
}

function add_visit(aURI, aVisitDate, aVisitType) {
  var isRedirect = aVisitType == histsvc.TRANSITION_REDIRECT_PERMANENT ||
                   aVisitType == histsvc.TRANSITION_REDIRECT_TEMPORARY;
  var visitId = histsvc.addVisit(aURI, aVisitDate, null,
                                 aVisitType, isRedirect, 0);
  return visitId;
}

var bucketPrefs = [
  [ "firstBucketCutoff", "firstBucketWeight"],
  [ "secondBucketCutoff", "secondBucketWeight"],
  [ "thirdBucketCutoff", "thirdBucketWeight"],
  [ "fourthBucketCutoff", "fourthBucketWeight"],
  [ null, "defaultBucketWeight"]
];

var bonusPrefs = {
  embedVisitBonus: Ci.nsINavHistoryService.TRANSITION_EMBED,
  linkVisitBonus: Ci.nsINavHistoryService.TRANSITION_LINK,
  typedVisitBonus: Ci.nsINavHistoryService.TRANSITION_TYPED,
  bookmarkVisitBonus: Ci.nsINavHistoryService.TRANSITION_BOOKMARK,
  downloadVisitBonus: Ci.nsINavHistoryService.TRANSITION_DOWNLOAD,
  permRedirectVisitBonus: Ci.nsINavHistoryService.TRANSITION_REDIRECT_PERMANENT,
  tempRedirectVisitBonus: Ci.nsINavHistoryService.TRANSITION_REDIRECT_TEMPORARY,
  defaultVisitBonus: 0,
  unvisitedBookmarkBonus: 0


};


var searchTerm = "frecency";
var results = [];
var matchCount = 0;
var now = Date.now();
var prefPrefix = "places.frecency.";
bucketPrefs.every(function(bucket) {
  let [cutoffName, weightName] = bucket;
  
  var weight = 0, cutoff = 0, bonus = 0;
  try {
    weight = prefs.getIntPref(prefPrefix + weightName);
  } catch(ex) {}
  try {
    cutoff = prefs.getIntPref(prefPrefix + cutoffName);
  } catch(ex) {}

  if (cutoff < 1)
    return true;

  
  var dateInPeriod = (now - ((cutoff - 1) * 86400 * 1000)) * 1000;

  for (let [bonusName, visitType] in Iterator(bonusPrefs)) {
    var frecency = -1;
    var calculatedURI = null;
    var matchTitle = "";
    var bonusValue = prefs.getIntPref(prefPrefix + bonusName);
    
    if (bonusName == "unvisitedBookmarkBonus" || bonusName == "unvisitedTypedBonus") {
      if (cutoffName == "firstBucketCutoff") {
        var points = Math.ceil(bonusValue / parseFloat(100.0) * weight); 
        var visitCount = 1; 
        frecency = Math.ceil(visitCount * points);
        calculatedURI = uri("http://" + searchTerm + ".com/" +
          bonusName + ":" + bonusValue + "/cutoff:" + cutoff +
          "/weight:" + weight + "/frecency:" + frecency);
        if (bonusName == "unvisitedBookmarkBonus") {
          matchTitle = searchTerm + "UnvisitedBookmark";
          bmsvc.insertBookmark(bmsvc.unfiledBookmarksFolder, calculatedURI, bmsvc.DEFAULT_INDEX, matchTitle);
        }
        else {
          matchTitle = searchTerm + "UnvisitedTyped";
          ghist.setPageTitle(calculatedURI, matchTitle);
          bhist.markPageAsTyped(calculatedURI);
        }
      }
    }
    else {
      
      
      if (visitType == Ci.nsINavHistoryService.TRANSITION_BOOKMARK)
        bonusValue = bonusValue * 2;

      var points = Math.ceil(1 * ((bonusValue / parseFloat(100.000000)).toFixed(6) * weight) / 1);
      if (!points) {
        if (!visitType ||
            visitType == Ci.nsINavHistoryService.TRANSITION_EMBED ||
            visitType == Ci.nsINavHistoryService.TRANSITION_DOWNLOAD ||
            bonusName == "defaultVisitBonus")
          frecency = 0;
        else
          frecency = -1;
      }
      else
        frecency = points;
      calculatedURI = uri("http://" + searchTerm + ".com/" +
        bonusName + ":" + bonusValue + "/cutoff:" + cutoff +
        "/weight:" + weight + "/frecency:" + frecency);
      if (visitType == Ci.nsINavHistoryService.TRANSITION_BOOKMARK) {
        matchTitle = searchTerm + "Bookmarked";
        bmsvc.insertBookmark(bmsvc.unfiledBookmarksFolder, calculatedURI, bmsvc.DEFAULT_INDEX, matchTitle);
      }
      else
        matchTitle = calculatedURI.spec.substr(calculatedURI.spec.lastIndexOf("/")+1);
      add_visit(calculatedURI, dateInPeriod, visitType);
    }

    if (calculatedURI && frecency) {
      results.push([calculatedURI, frecency, matchTitle]);
      setPageTitle(calculatedURI, matchTitle);
    }
  }
  return true;
});


results.sort(function(a,b) a[1] - b[1]);
results.reverse();

prefs.setIntPref("browser.urlbar.maxRichResults", results.length);



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

function run_test() {
  var controller = Components.classes["@mozilla.org/autocomplete/controller;1"].
                   getService(Components.interfaces.nsIAutoCompleteController);

  
  
  var input = new AutoCompleteInput(["history"]);

  controller.input = input;

  
  prefs.setIntPref("browser.urlbar.search.sources", 3);
  prefs.setIntPref("browser.urlbar.default.behavior", 0);

  
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

    
    do_check_eq(controller.matchCount, results.length);

    
    for (var i = 0; i < controller.matchCount; i++) {
      let searchURL = controller.getValueAt(i);
      let expectURL = results[i][0].spec;
      if (searchURL == expectURL) {
        do_check_eq(controller.getValueAt(i), results[i][0].spec);
        do_check_eq(controller.getCommentAt(i), results[i][2]);
      } else {
        
        
        
        
        let getFrecency = function(aURL) aURL.match(/frecency:(-?\d+)$/)[1];
        print("### checking for same frecency between '" + searchURL +
              "' and '" + expectURL + "'");
        do_check_eq(getFrecency(searchURL), getFrecency(expectURL));
      }
    }

    do_test_finished();
  };

  controller.startSearch(searchTerm);
}

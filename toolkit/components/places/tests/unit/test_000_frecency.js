version(180);



















































var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
              getService(Ci.nsINavHistoryService);
var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
            getService(Ci.nsINavBookmarksService);
var prefs = Cc["@mozilla.org/preferences-service;1"].
            getService(Ci.nsIPrefBranch);

function add_visit(aURI, aVisitDate, aVisitType) {
  var isRedirect = aVisitType == histsvc.TRANSITION_REDIRECT_PERMANENT ||
                   aVisitType == histsvc.TRANSITION_REDIRECT_TEMPORARY;
  var placeID = histsvc.addVisit(aURI, aVisitDate, null,
                                 aVisitType, isRedirect, 0);
  do_check_true(placeID > 0);
  return placeID;
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
          histsvc.setPageDetails(calculatedURI, matchTitle, 1, false, true);
        }
      }
    }
    else {
      
      var points = Math.ceil(1 * ((bonusValue / parseFloat(100.000000)).toFixed(6) * weight) / 1);
      if (!points) {
        if (visitType == Ci.nsINavHistoryService.TRANSITION_EMBED || bonusName == "defaultVisitBonus")
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

    if (calculatedURI && frecency)
      results.push([calculatedURI, frecency, matchTitle]);
  }
  return true;
});


results.sort(function(a,b) a[1] - b[1]);
results.reverse();



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

function run_test() {
  var controller = Components.classes["@mozilla.org/autocomplete/controller;1"].
                   getService(Components.interfaces.nsIAutoCompleteController);  
  
  
  
  var input = new AutoCompleteInput(["history"]);

  controller.input = input;

  
  do_test_pending();

  input.onSearchComplete = function() {
    do_check_eq(controller.searchStatus, 
                Ci.nsIAutoCompleteController.STATUS_COMPLETE_MATCH);

    
    do_check_eq(controller.matchCount, results.length);

    
    for (var i = 0; i < controller.matchCount; i++) {
      do_check_eq(controller.getValueAt(i), results[i][0].spec);
      do_check_eq(controller.getCommentAt(i), results[i][2]);
    }

    do_test_finished();
  };

  controller.startSearch(searchTerm);
}


















try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                getService(Ci.nsINavHistoryService);
  var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
              getService(Ci.nsINavBookmarksService);
  var prefs = Cc["@mozilla.org/preferences-service;1"].
              getService(Ci.nsIPrefBranch);
} catch(ex) {
  do_throw("Could not get services\n");
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
  framedLinkVisitBonus: Ci.nsINavHistoryService.TRANSITION_FRAMED_LINK,
  linkVisitBonus: Ci.nsINavHistoryService.TRANSITION_LINK,
  typedVisitBonus: Ci.nsINavHistoryService.TRANSITION_TYPED,
  bookmarkVisitBonus: Ci.nsINavHistoryService.TRANSITION_BOOKMARK,
  downloadVisitBonus: Ci.nsINavHistoryService.TRANSITION_DOWNLOAD,
  permRedirectVisitBonus: Ci.nsINavHistoryService.TRANSITION_REDIRECT_PERMANENT,
  tempRedirectVisitBonus: Ci.nsINavHistoryService.TRANSITION_REDIRECT_TEMPORARY,
};


var searchTerm = "frecency";
var results = [];
var matchCount = 0;
var now = Date.now();
var prefPrefix = "places.frecency.";

function task_initializeBucket(bucket) {
  let [cutoffName, weightName] = bucket;
  
  var weight = 0, cutoff = 0, bonus = 0;
  try {
    weight = prefs.getIntPref(prefPrefix + weightName);
  } catch(ex) {}
  try {
    cutoff = prefs.getIntPref(prefPrefix + cutoffName);
  } catch(ex) {}

  if (cutoff < 1)
    return;

  
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
          yield promiseAddVisits({
            uri: calculatedURI,
            title: matchTitle,
            transition: visitType,
            visitDate: now
          });
          histsvc.markPageAsTyped(calculatedURI);
        }
      }
    }
    else {
      
      
      if (visitType == Ci.nsINavHistoryService.TRANSITION_BOOKMARK)
        bonusValue = bonusValue * 2;

      var points = Math.ceil(1 * ((bonusValue / parseFloat(100.000000)).toFixed(6) * weight) / 1);
      if (!points) {
        if (visitType == Ci.nsINavHistoryService.TRANSITION_EMBED ||
            visitType == Ci.nsINavHistoryService.TRANSITION_FRAMED_LINK ||
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
      yield promiseAddVisits({
        uri: calculatedURI,
        transition: visitType,
        visitDate: dateInPeriod
      });
    }

    if (calculatedURI && frecency) {
      results.push([calculatedURI, frecency, matchTitle]);
      yield promiseAddVisits({
        uri: calculatedURI,
        title: matchTitle,
        transition: visitType,
        visitDate: dateInPeriod
      });
    }
  }
}

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

function run_test()
{
  run_next_test();
}

add_task(function test_frecency()
{
  for (let [, bucket] in Iterator(bucketPrefs)) {
    yield task_initializeBucket(bucket);
  }

  
  results.sort(function(a,b) b[1] - a[1]);
  
  prefs.setIntPref("browser.urlbar.maxRichResults", results.length);

  
  

  yield promiseAsyncUpdates();

  var controller = Components.classes["@mozilla.org/autocomplete/controller;1"].
                   getService(Components.interfaces.nsIAutoCompleteController);

  
  
  var input = new AutoCompleteInput(["history"]);

  controller.input = input;

  
  prefs.setIntPref("browser.urlbar.search.sources", 3);
  prefs.setIntPref("browser.urlbar.default.behavior", 0);

  var numSearchesStarted = 0;
  input.onSearchBegin = function() {
    numSearchesStarted++;
    do_check_eq(numSearchesStarted, 1);
  };

  let deferred = Promise.defer();
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
    deferred.resolve();
  };

  controller.startSearch(searchTerm);

  yield deferred.promise;
});

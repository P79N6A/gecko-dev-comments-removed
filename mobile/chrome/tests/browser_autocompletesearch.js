Components.utils.import("resource://gre/modules/Services.jsm");

let match= [
  ["http://example.com/a", "A", "favicon", "http://example.com/a/favicon.png"],
  ["http://example.com/b", "B", "favicon", "http://example.com/b/favicon.png"],
  ["http://example.com/c", "C", "favicon", "http://example.com/c/favicon.png"],
  ["http://example.com/d", "D", "bookmark", "http://example.com/d/favicon.png"],
  ["http://example.com/e", "E", "boolmark", "http://example.com/e/favicon.png"]
];

var gAutocomplete = null;
var gProfileDir = null;

function test() {
  waitForExplicitFinish();

  gProfileDir = Services.dirsvc.get("ProfD", Ci.nsIFile);

  
  let oldCacheFile = gProfileDir.clone();
  oldCacheFile.append("autocomplete.json");
  if (oldCacheFile.exists())
    oldCacheFile.remove(true);

  
  
  Services.obs.addObserver(function (aSubject, aTopic, aData) {
    Services.obs.removeObserver(arguments.callee, aTopic, false);
    saveMockCache();
  }, "browser:cache-session-history-write-complete", false);

  
  
  gAutocomplete = Cc["@mozilla.org/autocomplete/search;1?name=history"].getService(Ci.nsIAutoCompleteSearch);

  
  Services.obs.notifyObservers(null, "browser:cache-session-history-reload", "");
}

function saveMockCache() {
  
  let oldCacheFile = gProfileDir.clone();
  oldCacheFile.append("autocomplete.json");
  if (oldCacheFile.exists())
    oldCacheFile.remove(true);

  let mockCachePath = gTestPath;
  let mockCacheFile = getChromeDir(getResolvedURI(mockCachePath));
  mockCacheFile.append("mock_autocomplete.json");
  mockCacheFile.copyToFollowingLinks(gProfileDir, "autocomplete.json");

  
  Services.obs.addObserver(function (aSubject, aTopic, aData) {
    Services.obs.removeObserver(arguments.callee, aTopic, false);
    runTest();
  }, "browser:cache-session-history-read-complete", false);

  
  Services.obs.notifyObservers(null, "browser:cache-session-history-reload", "");
}

function runTest() {
  let cacheFile = gProfileDir.clone();
  cacheFile.append("autocomplete.json");
  ok(cacheFile.exists(), "Mock autocomplete JSON cache file exists");

  
  gAutocomplete.startSearch("", "", null, {
    onSearchResult: function(search, result) {
      is(result.matchCount, 5, "matchCount is correct");
      
      for (let i=0; i<5; i++) {
        is(result.getValueAt(i), match[i][0], "value matches");
        is(result.getCommentAt(i), match[i][1], "comment matches");
        is(result.getStyleAt(i), match[i][2], "style matches");
        is(result.getImageAt(i), match[i][3], "image matches");
      }

      if (cacheFile.exists())
        cacheFile.remove(true);

      finish();
    }
  });
}




const URL = "http://mochi.test:8888/";
const URL_COPY = URL + "#copy";

XPCOMUtils.defineLazyGetter(this, "Sanitizer", function () {
  let tmp = {};
  Cc["@mozilla.org/moz/jssubscript-loader;1"]
    .getService(Ci.mozIJSSubScriptLoader)
    .loadSubScript("chrome://browser/content/sanitize.js", tmp);
  return tmp.Sanitizer;
});






function runTests() {
  yield clearHistory();
  yield createThumbnail();

  
  PageThumbsStorage.copy(URL, URL_COPY);
  let copy = PageThumbsStorage.getFileForURL(URL_COPY);
  let mtime = copy.lastModifiedTime -= 60;

  PageThumbsStorage.copy(URL, URL_COPY);
  isnot(PageThumbsStorage.getFileForURL(URL_COPY).lastModifiedTime, mtime,
        "thumbnail file was updated");

  let file = PageThumbsStorage.getFileForURL(URL);
  let fileCopy = PageThumbsStorage.getFileForURL(URL_COPY);

  
  
  while (file.exists() || fileCopy.exists()) {
    yield clearHistory();
  }

  yield createThumbnail();

  
  yield clearHistory(true);

  
  while (file.exists()) {
    
    
    let time = Date.now() * 1000;
    let trans = Ci.nsINavHistoryService.TRANSITION_LINK;
    PlacesUtils.history.addVisit(makeURI(URL), time, null, trans, false, 0);

    
    yield clearHistory(true);
  }
}

function clearHistory(aUseRange) {
  let s = new Sanitizer();
  s.prefDomain = "privacy.cpd.";

  let prefs = gPrefService.getBranch(s.prefDomain);
  prefs.setBoolPref("history", true);
  prefs.setBoolPref("downloads", false);
  prefs.setBoolPref("cache", false);
  prefs.setBoolPref("cookies", false);
  prefs.setBoolPref("formdata", false);
  prefs.setBoolPref("offlineApps", false);
  prefs.setBoolPref("passwords", false);
  prefs.setBoolPref("sessions", false);
  prefs.setBoolPref("siteSettings", false);

  if (aUseRange) {
    let usec = Date.now() * 1000;
    s.range = [usec - 10 * 60 * 1000 * 1000, usec];
    s.ignoreTimespan = false;
  }

  s.sanitize();
  s.range = null;
  s.ignoreTimespan = true;

  executeSoon(next);
}

function createThumbnail() {
  addTab(URL, function () {
    whenFileExists(function () {
      gBrowser.removeTab(gBrowser.selectedTab);
      next();
    });
  });
}

function whenFileExists(aCallback) {
  let callback;
  let file = PageThumbsStorage.getFileForURL(URL);
  if (file.exists() && file.fileSize) {
    callback = aCallback;
  } else {
    callback = function () whenFileExists(aCallback);
  }

  executeSoon(callback);
}

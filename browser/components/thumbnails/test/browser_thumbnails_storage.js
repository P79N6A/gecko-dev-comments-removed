


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

  
  yield addTab(URL);
  yield whenFileExists();
  gBrowser.removeTab(gBrowser.selectedTab);

  
  yield clearHistory();

  
  yield addTab(URL);
  yield whenFileExists();
  gBrowser.removeTab(gBrowser.selectedTab);

  
  PageThumbsStorage.copy(URL, URL_COPY);
  let copy = PageThumbsStorage.getFileForURL(URL_COPY);
  let mtime = copy.lastModifiedTime -= 60;

  PageThumbsStorage.copy(URL, URL_COPY);
  isnot(PageThumbsStorage.getFileForURL(URL_COPY).lastModifiedTime, mtime,
        "thumbnail file was updated");

  
  yield clearHistory(true);
  yield whenFileRemoved();
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

  executeSoon(function () {
    if (PageThumbsStorage.getFileForURL(URL).exists())
      clearHistory(aUseRange);
    else
      next();
  });
}

function whenFileExists() {
  let callback = whenFileExists;

  let file = PageThumbsStorage.getFileForURL(URL);
  if (file.exists() && file.fileSize)
    callback = next;

  executeSoon(callback);
}

function whenFileRemoved() {
  let callback = whenFileRemoved;

  let file = PageThumbsStorage.getFileForURL(URL);
  if (!file.exists())
    callback = next;

  executeSoon(callback);
}

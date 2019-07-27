


const PREF_DISK_CACHE_SSL = "browser.cache.disk_cache_ssl";
const URL = "://example.com/browser/toolkit/components/thumbnails/" +
            "test/privacy_cache_control.sjs";

function runTests() {
  registerCleanupFunction(function () {
    Services.prefs.clearUserPref(PREF_DISK_CACHE_SSL);
  });

  let positive = [
    
    {scheme: "http", cacheControl: null, diskCacheSSL: false},

    
    {scheme: "http", cacheControl: "private", diskCacheSSL: false},

    
    {scheme: "https", cacheControl: null, diskCacheSSL: true},
    {scheme: "https", cacheControl: "public", diskCacheSSL: true},
    {scheme: "https", cacheControl: "private", diskCacheSSL: true}
  ];

  let negative = [
    
    {scheme: "http", cacheControl: "no-store", diskCacheSSL: false},
    {scheme: "http", cacheControl: "no-store", diskCacheSSL: true},
    {scheme: "https", cacheControl: "no-store", diskCacheSSL: false},
    {scheme: "https", cacheControl: "no-store", diskCacheSSL: true},

    
    {scheme: "https", cacheControl: null, diskCacheSSL: false},
    {scheme: "https", cacheControl: "public", diskCacheSSL: false},
    {scheme: "https", cacheControl: "private", diskCacheSSL: false}
  ];

  yield checkCombinations(positive, true);
  yield checkCombinations(negative, false);
}

function checkCombinations(aCombinations, aResult) {
  let combi = aCombinations.shift();
  if (!combi) {
    next();
    return;
  }

  let url = combi.scheme + URL;
  if (combi.cacheControl)
    url += "?" + combi.cacheControl;
  Services.prefs.setBoolPref(PREF_DISK_CACHE_SSL, combi.diskCacheSSL);

  
  addVisitsAndRepopulateNewTabLinks(url, _ => {
    testCombination(combi, url, aCombinations, aResult);
  });
}

function testCombination(combi, url, aCombinations, aResult) {
  let tab = gBrowser.selectedTab = gBrowser.addTab(url);
  let browser = gBrowser.selectedBrowser;

  whenLoaded(browser, function () {
    let msg = JSON.stringify(combi) + " == " + aResult;
    is(gBrowserThumbnails._shouldCapture(browser), aResult, msg);
    gBrowser.removeTab(tab);

    
    checkCombinations(aCombinations, aResult);
  });
}

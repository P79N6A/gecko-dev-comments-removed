


const URL = "http://mochi.test:8888/browser/browser/components/thumbnails/" +
            "test/background_red_redirect.sjs";

let cacheService = Cc["@mozilla.org/network/cache-service;1"]
                   .getService(Ci.nsICacheService);




function runTests() {
  
  yield addTab(URL);
  gBrowser.removeTab(gBrowser.selectedTab);

  
  yield addTab(URL);
  yield captureAndCheckColor(255, 0, 0, "we have a red thumbnail");

  
  yield whenCacheEntryExists(URL);
  yield checkThumbnailColor(URL, 255, 0, 0, "referrer has a red thumbnail");
}

function whenCacheEntryExists(aKey) {
  let callback = next;

  checkCacheEntryExists(aKey, function (aExists) {
    if (!aExists)
      callback = function () whenCacheEntryExists(aKey);

    executeSoon(callback);
  });
}

function checkCacheEntryExists(aKey, aCallback) {
  PageThumbsCache.getReadEntry(aKey, function (aEntry) {
    let inputStream = aEntry && aEntry.openInputStream(0);
    let exists = inputStream && inputStream.available();

    if (inputStream)
      inputStream.close();

    if (aEntry)
      aEntry.close();

    aCallback(exists);
  });
}

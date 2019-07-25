


const URL = "http://mochi.test:8888/browser/browser/components/thumbnails/" +
            "test/background_red_redirect.sjs";




function runTests() {
  
  yield addTab(URL);
  gBrowser.removeTab(gBrowser.selectedTab);

  
  yield addTab(URL);
  yield captureAndCheckColor(255, 0, 0, "we have a red thumbnail");

  
  yield whenFileExists(URL);
  yield checkThumbnailColor(URL, 255, 0, 0, "referrer has a red thumbnail");
}

function whenFileExists(aURL) {
  let callback = next;

  let file = PageThumbsStorage.getFileForURL(aURL);
  if (!file.exists())
    callback = function () whenFileExists(aURL);

  executeSoon(callback);
}




const URL = "http://mochi.test:8888/browser/toolkit/components/thumbnails/" +
            "test/background_red_redirect.sjs";

const FINAL_URL = "http://mochi.test:8888/browser/toolkit/components/" +
                  "thumbnails/test/background_red.html";




function runTests() {
  dontExpireThumbnailURLs([URL, FINAL_URL]);

  
  yield addTab(URL);
  gBrowser.removeTab(gBrowser.selectedTab);

  
  yield addTab(URL);
  yield captureAndCheckColor(255, 0, 0, "we have a red thumbnail");

  
  yield whenFileExists(URL);
  yield retrieveImageDataForURL(URL, function ([r, g, b]) {
    is("" + [r,g,b], "255,0,0", "referrer has a red thumbnail");
    next();
  });
}

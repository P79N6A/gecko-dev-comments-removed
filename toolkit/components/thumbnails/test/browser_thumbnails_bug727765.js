


const URL = "http://mochi.test:8888/browser/toolkit/components/thumbnails/" +
            "test/background_red_scroll.html";


function runTests() {
  
  yield addTab(URL);
  yield captureAndCheckColor(255, 0, 0, "we have a red thumbnail");

  
  yield whenFileExists(URL);
  yield retrieveImageDataForURL(URL, function (aData) {
    let [r, g, b] = [].slice.call(aData, -4);
    is("" + [r,g,b], "255,0,0", "we have a red thumbnail");
    next();
  });
}

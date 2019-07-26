


const URL = "http://mochi.test:8888/browser/toolkit/components/thumbnails/" +
            "test/background_red.html?" + Date.now();


function runTests() {

  let path = PageThumbs.getThumbnailPath(URL);
  yield testIfExists(path, false, "Thumbnail file does not exist");

  yield createThumbnail(URL);

  path = PageThumbs.getThumbnailPath(URL);
  let expectedPath = PageThumbsStorage.getFilePathForURL(URL);
  is(path, expectedPath, "Thumbnail file has correct path");

  yield testIfExists(path, true, "Thumbnail file exists");

}

function createThumbnail(aURL) {
  addTab(aURL, function () {
    whenFileExists(aURL, function () {
      gBrowser.removeTab(gBrowser.selectedTab);
      next();
    });
  });
}

function testIfExists(aPath, aExpected, aMessage) {
  return OS.File.exists(aPath).then(
    function onSuccess(exists) {
      is(exists, aExpected, aMessage);
    },
    function onFailure(error) {
      ok(false, "OS.File.exists() failed " + error);
    }
  );
}

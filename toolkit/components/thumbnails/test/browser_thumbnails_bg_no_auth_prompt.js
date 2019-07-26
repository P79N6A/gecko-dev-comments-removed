










function runTests() {
  let url = "http://mochi.test:8888/browser/browser/base/content/test/general/authenticate.sjs?user=anyone";
  ok(!thumbnailExists(url), "Thumbnail file should not already exist.");

  let capturedURL = yield bgCapture(url);
  is(capturedURL, url, "Captured URL should be URL passed to capture.");
  ok(thumbnailExists(url),
     "Thumbnail file should exist even though it requires auth.");
  removeThumbnail(url);
}

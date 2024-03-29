


function runTests() {
  let url = "http://www.example.com/";
  ok(!thumbnailExists(url), "Thumbnail should not be cached yet.");

  let capturedURL = yield bgCapture(url);
  is(capturedURL, url, "Captured URL should be URL passed to capture");

  ok(thumbnailExists(url), "Thumbnail should be cached after capture");
  removeThumbnail(url);
}

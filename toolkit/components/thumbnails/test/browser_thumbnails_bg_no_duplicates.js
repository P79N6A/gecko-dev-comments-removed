


function runTests() {
  let url = "http://example.com/1";
  ok(!thumbnailExists(url), "Thumbnail file should not already exist.");
  let numCallbacks = 0;
  let doneCallback = function(doneUrl) {
    is(doneUrl, url, "called back with correct url");
    numCallbacks += 1;
    
    
    
    
    if (numCallbacks == 1) {
      ok(thumbnailExists(url), "Thumbnail file should now exist.");
      removeThumbnail(url);
      return;
    }
    if (numCallbacks == 2) {
      ok(!thumbnailExists(url), "Thumbnail file should still be deleted.");
      
      next();
      return;
    }
    ok(false, "only expecting 2 callbacks");
  }
  BackgroundPageThumbs.capture(url, {onDone: doneCallback});
  BackgroundPageThumbs.capture(url, {onDone: doneCallback});
  yield true;
}




function runTests() {
  let urls = [
    "http://www.example.com/0",
    "http://www.example.com/1",
    
    bgTestPageURL({ wait: 2002 }),
    "http://www.example.com/2",
  ];
  dontExpireThumbnailURLs(urls);
  urls.forEach(url => {
    ok(!thumbnailExists(url), "Thumbnail should not exist yet: " + url);
    let isTimeoutTest = url.indexOf("wait") >= 0;
    BackgroundPageThumbs.capture(url, {
      timeout: isTimeoutTest ? 100 : 30000,
      onDone: function onDone(capturedURL) {
        ok(urls.length > 0, "onDone called, so URLs should still remain");
        is(capturedURL, urls.shift(),
           "Captured URL should be currently expected URL (i.e., " +
           "capture() callbacks should be called in the correct order)");
        if (isTimeoutTest) {
          ok(!thumbnailExists(capturedURL),
             "Thumbnail shouldn't exist for timed out capture");
        } else {
          ok(thumbnailExists(capturedURL),
             "Thumbnail should be cached after capture");
          removeThumbnail(url);
        }
        if (!urls.length)
          
          next();
      },
    });
  });
  yield true;
}








function runTests() {
  
  let tests = [
    simpleCaptureTest,
    capIfStaleErrorResponseUpdateTest,
    capIfStaleGoodResponseUpdateTest,
    regularCapErrorResponseUpdateTest,
    regularCapGoodResponseUpdateTest
  ];
  for (let test of tests) {
    info("Running subtest " + test.name);
    for (let iterator of test())
      yield iterator;
  }
}

function ensureThumbnailStale(url) {
  
  
  let fname = PageThumbsStorage.getFilePathForURL(url);
  let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
  file.initWithPath(fname);
  ok(file.exists(), fname + " should exist");
  
  file.lastModifiedTime = Date.now() - 1000000000;
}

function getThumbnailModifiedTime(url) {
  let fname = PageThumbsStorage.getFilePathForURL(url);
  let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
  file.initWithPath(fname);
  return file.lastModifiedTime;
}



function simpleCaptureTest() {
  let numNotifications = 0;
  const URL = "http://mochi.test:8888/browser/toolkit/components/thumbnails/test/thumbnails_update.sjs?simple";

  function observe(subject, topic, data) {
    is(topic, "page-thumbnail:create", "got expected topic");
    is(data, URL, "data is our test URL");
    if (++numNotifications == 2) {
      
      Services.obs.removeObserver(observe, "page-thumbnail:create");
      gBrowser.removeTab(gBrowser.selectedTab);
      next();
    }
  }

  Services.obs.addObserver(observe, "page-thumbnail:create", false);
  
  yield addTab(URL);
  let browser = gBrowser.selectedBrowser;

  
  PageThumbs.captureAndStore(browser, function () {
    
    is(numNotifications, 1, "got notification of item being created.");
    
    
    PageThumbs.captureAndStoreIfStale(browser, function() {
      is(numNotifications, 1, "still only 1 notification of item being created.");

      ensureThumbnailStale(URL);
      
      PageThumbs.captureAndStoreIfStale(browser);
      
      
    });
  });
  yield undefined 
}




function capIfStaleErrorResponseUpdateTest() {
  const URL = "http://mochi.test:8888/browser/toolkit/components/thumbnails/test/thumbnails_update.sjs?fail";
  yield addTab(URL);

  yield captureAndCheckColor(0, 255, 0, "we have a green thumbnail");
  
  
  
  
  ensureThumbnailStale(URL);
  yield navigateTo(URL);
  
  
  
  let now = Date.now() - 1000 ;
  PageThumbs.captureAndStoreIfStale(gBrowser.selectedBrowser, () => {
    ok(getThumbnailModifiedTime(URL) < now, "modified time should be < now");
    retrieveImageDataForURL(URL, function ([r, g, b]) {
      is("" + [r,g,b], "" + [0, 255, 0], "thumbnail is still green");
      gBrowser.removeTab(gBrowser.selectedTab);
      next();
    });
  });
  yield undefined; 
}





function capIfStaleGoodResponseUpdateTest() {
  const URL = "http://mochi.test:8888/browser/toolkit/components/thumbnails/test/thumbnails_update.sjs?ok";
  yield addTab(URL);
  let browser = gBrowser.selectedBrowser;

  yield captureAndCheckColor(0, 255, 0, "we have a green thumbnail");
  
  
  
  ensureThumbnailStale(URL);
  yield navigateTo(URL);
  
  
  
  let now = Date.now() - 1000 ;
  PageThumbs.captureAndStoreIfStale(browser, () => {
    ok(getThumbnailModifiedTime(URL) >= now, "modified time should be >= now");
    
    
    retrieveImageDataForURL(URL, function ([r, g, b]) {
      is("" + [r,g,b], "" + [255, 0, 0], "thumbnail is now red");
      next();
    });
  });
  yield undefined; 
}




function regularCapErrorResponseUpdateTest() {
  const URL = "http://mochi.test:8888/browser/toolkit/components/thumbnails/test/thumbnails_update.sjs?fail";
  yield addTab(URL);
  yield captureAndCheckColor(0, 255, 0, "we have a green thumbnail");
  gBrowser.removeTab(gBrowser.selectedTab);
  
  
  yield addTab(URL);
  yield captureAndCheckColor(0, 255, 0, "we still have a green thumbnail");
}




function regularCapGoodResponseUpdateTest() {
  const URL = "http://mochi.test:8888/browser/toolkit/components/thumbnails/test/thumbnails_update.sjs?ok";
  yield addTab(URL);
  yield captureAndCheckColor(0, 255, 0, "we have a green thumbnail");
  gBrowser.removeTab(gBrowser.selectedTab);
  
  
  yield addTab(URL);
  yield captureAndCheckColor(255, 0, 0, "we now  have a red thumbnail");
}

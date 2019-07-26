






function runTests() {
  
  let tests = [
    simpleCaptureTest,
    errorResponseUpdateTest,
    goodResponseUpdateTest,
    foregroundErrorResponseUpdateTest,
    foregroundGoodResponseUpdateTest
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
  const URL = "data:text/html;charset=utf-8,<body%20bgcolor=ff0000></body>";

  function observe(subject, topic, data) {
    is(topic, "page-thumbnail:create", "got expected topic");
    is(data, URL, "data is our test URL");
    if (++numNotifications == 2) {
      
      Services.obs.removeObserver(observe, "page-thumbnail:create");
      next();
    }
  }

  Services.obs.addObserver(observe, "page-thumbnail:create", false);
  
  yield addTab(URL);
  let browser = gBrowser.selectedBrowser;

  
  PageThumbs.captureAndStore(browser, function () {
    
    gBrowser.removeTab(gBrowser.selectedTab);
    
    is(numNotifications, 1, "got notification of item being created.");
    
    
    PageThumbs.captureIfStale(URL);
    is(numNotifications, 1, "still only 1 notification of item being created.");

    ensureThumbnailStale(URL);
    
    PageThumbs.captureIfStale(URL);
    
    
  });
  yield undefined 
}




function errorResponseUpdateTest() {
  const URL = "http://mochi.test:8888/browser/toolkit/components/thumbnails/test/thumbnails_update.sjs?fail";
  yield addTab(URL);

  yield captureAndCheckColor(0, 255, 0, "we have a green thumbnail");
  gBrowser.removeTab(gBrowser.selectedTab);
  
  
  
  
  
  ensureThumbnailStale(URL);
  let now = Date.now();
  PageThumbs.captureIfStale(URL).then(() => {
    ok(getThumbnailModifiedTime(URL) >= now, "modified time should be >= now");
    retrieveImageDataForURL(URL, function ([r, g, b]) {
      is("" + [r,g,b], "" + [0, 255, 0], "thumbnail is still green");
      next();
    });
  }).then(null, err => {ok(false, "Error in captureIfStale: " + err)});
  yield undefined; 
}





function goodResponseUpdateTest() {
  const URL = "http://mochi.test:8888/browser/toolkit/components/thumbnails/test/thumbnails_update.sjs?ok";
  yield addTab(URL);
  let browser = gBrowser.selectedBrowser;

  yield captureAndCheckColor(0, 255, 0, "we have a green thumbnail");
  
  
  
  ensureThumbnailStale(URL);
  let now = Date.now();
  PageThumbs.captureIfStale(URL).then(() => {
    ok(getThumbnailModifiedTime(URL) >= now, "modified time should be >= now");
    
    
    retrieveImageDataForURL(URL, function ([r, g, b]) {
      is("" + [r,g,b], "" + [255, 0, 0], "thumbnail is now red");
      next();
    });
  }).then(null, err => {ok(false, "Error in captureIfStale: " + err)});
  yield undefined; 
}




function foregroundErrorResponseUpdateTest() {
  const URL = "http://mochi.test:8888/browser/toolkit/components/thumbnails/test/thumbnails_update.sjs?fail";
  yield addTab(URL);
  yield captureAndCheckColor(0, 255, 0, "we have a green thumbnail");
  gBrowser.removeTab(gBrowser.selectedTab);
  
  
  yield addTab(URL);
  yield captureAndCheckColor(0, 255, 0, "we still have a green thumbnail");
}

function foregroundGoodResponseUpdateTest() {
  const URL = "http://mochi.test:8888/browser/toolkit/components/thumbnails/test/thumbnails_update.sjs?ok";
  yield addTab(URL);
  yield captureAndCheckColor(0, 255, 0, "we have a green thumbnail");
  gBrowser.removeTab(gBrowser.selectedTab);
  
  
  yield addTab(URL);
  yield captureAndCheckColor(255, 0, 0, "we now  have a red thumbnail");
}

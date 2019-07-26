






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

function runTests() {
  Services.obs.addObserver(observe, "page-thumbnail:create", false);
  
  yield addTab(URL);
  let browser = gBrowser.selectedBrowser;

  
  PageThumbs.captureAndStore(browser, function () {
    
    is(numNotifications, 1, "got notification of item being created.");
    
    
    PageThumbs.captureIfStale(URL);
    is(numNotifications, 1, "still only 1 notification of item being created.");

    
    
    let fname = PageThumbsStorage.getFilePathForURL(URL);
    let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
    file.initWithPath(fname);
    ok(file.exists(), fname + " doesn't exist");
    
    file.lastModifiedTime = Date.now() - 1000000000;
    
    PageThumbs.captureIfStale(URL);
    
    
  });
  yield undefined; 
}

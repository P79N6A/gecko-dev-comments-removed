


const TEST_PAGE_URL = "http://mochi.test:8888/browser/toolkit/components/thumbnails/test/thumbnails_background.sjs";

const imports = {};
Cu.import("resource://gre/modules/BackgroundPageThumbs.jsm", imports);
Cu.import("resource://gre/modules/PageThumbs.jsm", imports);
Cu.import("resource://gre/modules/Task.jsm", imports);
Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js", imports);
registerCleanupFunction(function () {
  imports.BackgroundPageThumbs._destroy();
});

function test() {
  waitForExplicitFinish();
  spawnNextTest();
}

function spawnNextTest() {
  if (!tests.length) {
    finish();
    return;
  }
  imports.Task.spawn(tests.shift()).then(spawnNextTest, function onError(err) {
    ok(false, err);
    spawnNextTest();
  });
}

let tests = [

  function basic() {
    let url = "http://www.example.com/";
    let file = fileForURL(url);
    ok(!file.exists(), "Thumbnail should not be cached yet.");

    let capturedURL = yield capture(url);
    is(capturedURL, url, "Captured URL should be URL passed to capture");

    ok(file.exists(), "Thumbnail should be cached after capture: " + file.path);
    file.remove(false);
  },

  function queueing() {
    let deferred = imports.Promise.defer();
    let urls = [
      "http://www.example.com/0",
      "http://www.example.com/1",
      "http://www.example.com/2",
    ];
    let files = urls.map(fileForURL);
    files.forEach(f => ok(!f.exists(), "Thumbnail should not be cached yet."));
    urls.forEach(function (url) {
      imports.BackgroundPageThumbs.capture(url, {
        onDone: function onDone(capturedURL) {
          ok(urls.length > 0, "onDone called, so URLs should still remain");
          is(capturedURL, urls.shift(),
             "Captured URL should be currently expected URL (i.e., " +
             "capture() callbacks should be called in the correct order)");
          let file = files.shift();
          ok(file.exists(),
             "Thumbnail should be cached after capture: " + file.path);
          if (!urls.length)
            deferred.resolve();
        },
      });
    });
    yield deferred.promise;
  },

  function timeout() {
    let deferred = imports.Promise.defer();
    let url = testPageURL({ wait: 30000 });
    let file = fileForURL(url);
    ok(!file.exists(), "Thumbnail should not be cached already.");
    let numCalls = 0;
    imports.BackgroundPageThumbs.capture(url, {
      timeout: 0,
      onDone: function onDone(capturedURL) {
        is(capturedURL, url, "Captured URL should be URL passed to capture");
        is(numCalls++, 0, "onDone should be called only once");
        ok(!file.exists(),
           "Capture timed out so thumbnail should not be cached: " + file.path);
        deferred.resolve();
      },
    });
    yield deferred.promise;
  },

  function timeoutQueueing() {
    let deferred = imports.Promise.defer();
    let urls = [
      { url: testPageURL({ wait: 2000 }), timeout: 30000 },
      { url: testPageURL({ wait: 2001 }), timeout: 1000 },
      { url: testPageURL({ wait: 2002 }), timeout: 0 },
    ];

    
    
    let expectedOrder = urls.slice();
    expectedOrder.reverse();
    expectedOrder = expectedOrder.map(u => u.url);

    let files = expectedOrder.map(fileForURL);
    files.forEach(f => ok(!f.exists(), "Thumbnail should not be cached yet."));

    urls.forEach(function ({ url, timeout }) {
      imports.BackgroundPageThumbs.capture(url, {
        timeout: timeout,
        onDone: function onDone(capturedURL) {
          ok(expectedOrder.length > 0,
             "onDone called, so URLs should still remain");
          is(capturedURL, expectedOrder.shift(),
             "Captured URL should be currently expected URL (i.e., " +
             "capture() callbacks should be called in the correct order)");
          let file = files.shift();
          if (timeout > 2000)
            ok(file.exists(),
               "Thumbnail should be cached after capture: " + file.path);
          else
            ok(!file.exists(),
               "Capture timed out so thumbnail should not be cached: " +
               file.path);
          if (!expectedOrder.length)
            deferred.resolve();
        },
      });
    });
    yield deferred.promise;
  },

  function redirect() {
    let finalURL = "http://example.com/redirected";
    let originalURL = testPageURL({ redirect: finalURL });

    let originalFile = fileForURL(originalURL);
    ok(!originalFile.exists(),
       "Thumbnail file for original URL should not exist yet.");

    let finalFile = fileForURL(finalURL);
    ok(!finalFile.exists(),
       "Thumbnail file for final URL should not exist yet.");

    let capturedURL = yield capture(originalURL);
    is(capturedURL, originalURL,
       "Captured URL should be URL passed to capture");
    ok(originalFile.exists(),
       "Thumbnail for original URL should be cached: " + originalFile.path);
    ok(finalFile.exists(),
       "Thumbnail for final URL should be cached: " + finalFile.path);

    originalFile.remove(false);
    finalFile.remove(false);
  },

  function destroyBrowserTimeout() {
    let url1 = "http://example.com/1";
    let file1 = fileForURL(url1);
    ok(!file1.exists(), "First file should not exist yet.");

    let url2 = "http://example.com/2";
    let file2 = fileForURL(url2);
    ok(!file2.exists(), "Second file should not exist yet.");

    let defaultTimeout = imports.BackgroundPageThumbs._destroyBrowserTimeout;
    imports.BackgroundPageThumbs._destroyBrowserTimeout = 1000;

    yield capture(url1);
    ok(file1.exists(), "First file should exist after capture.");

    yield wait(2000);
    is(imports.BackgroundPageThumbs._thumbBrowser, undefined,
       "Thumb browser should be destroyed after timeout.");

    yield capture(url2);
    ok(file2.exists(), "Second file should exist after capture.");

    imports.BackgroundPageThumbs._destroyBrowserTimeout = defaultTimeout;
    isnot(imports.BackgroundPageThumbs._thumbBrowser, undefined,
          "Thumb browser should exist immediately after capture.");
  },

  function privateBrowsingActive() {
    let url = "http://example.com/";
    let file = fileForURL(url);
    ok(!file.exists(), "Thumbnail file should not already exist.");

    let win = yield openPrivateWindow();
    let capturedURL = yield capture(url);
    is(capturedURL, url, "Captured URL should be URL passed to capture.");
    ok(!file.exists(),
       "Thumbnail file should not exist because a private window is open.");

    win.close();
  },

  function noCookies() {
    
    let url = testPageURL({ setGreenCookie: true });
    let tab = gBrowser.loadOneTab(url, { inBackground: false });
    let browser = tab.linkedBrowser;
    yield onPageLoad(browser);

    
    let greenStr = "rgb(0, 255, 0)";
    isnot(browser.contentDocument.documentElement.style.backgroundColor,
          greenStr,
          "The page shouldn't be green yet.");

    
    
    browser.reload();
    yield onPageLoad(browser);
    is(browser.contentDocument.documentElement.style.backgroundColor,
       greenStr,
       "The page should be green now.");

    
    
    yield capture(url);
    let file = fileForURL(url);
    ok(file.exists(), "Thumbnail file should exist after capture.");

    let deferred = imports.Promise.defer();
    retrieveImageDataForURL(url, function ([r, g, b]) {
      isnot([r, g, b].toString(), [0, 255, 0].toString(),
            "The captured page should not be green.");
      gBrowser.removeTab(tab);
      file.remove(false);
      deferred.resolve();
    });
    yield deferred.promise;
  },

  
  
  
  
  
  
  
  
  function noAuthPrompt() {
    let url = "http://mochi.test:8888/browser/browser/base/content/test/authenticate.sjs?user=anyone";
    let file = fileForURL(url);
    ok(!file.exists(), "Thumbnail file should not already exist.");

    let capturedURL = yield capture(url);
    is(capturedURL, url, "Captured URL should be URL passed to capture.");
    ok(file.exists(),
       "Thumbnail file should exist even though it requires auth.");
    file.remove(false);
  },

  function noAlert() {
    let url = "data:text/html,<script>alert('yo!');</script>";
    let file = fileForURL(url);
    ok(!file.exists(), "Thumbnail file should not already exist.");

    let capturedURL = yield capture(url);
    is(capturedURL, url, "Captured URL should be URL passed to capture.");
    ok(file.exists(),
       "Thumbnail file should exist even though it alerted.");
    file.remove(false);
  },
];

function capture(url, options) {
  let deferred = imports.Promise.defer();
  options = options || {};
  options.onDone = function onDone(capturedURL) {
    deferred.resolve(capturedURL);
  };
  imports.BackgroundPageThumbs.capture(url, options);
  return deferred.promise;
}

function testPageURL(opts) {
  return TEST_PAGE_URL + "?" + encodeURIComponent(JSON.stringify(opts || {}));
}

function fileForURL(url) {
  let path = imports.PageThumbsStorage.getFilePathForURL(url);
  let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
  file.initWithPath(path);
  return file;
}

function wait(ms) {
  let deferred = imports.Promise.defer();
  setTimeout(function onTimeout() {
    deferred.resolve();
  }, ms);
  return deferred.promise;
}

function openPrivateWindow() {
  let deferred = imports.Promise.defer();
  
  let win = window.openDialog("chrome://browser/content/", "_blank",
                              "chrome,all,dialog=no,private",
                              "about:privatebrowsing");
  win.addEventListener("load", function load(event) {
    if (event.target == win.document) {
      win.removeEventListener("load", load);
      deferred.resolve(win);
    }
  });
  return deferred.promise;
}

function onPageLoad(browser) {
  let deferred = imports.Promise.defer();
  browser.addEventListener("load", function load(event) {
    if (event.target == browser.contentWindow.document) {
      browser.removeEventListener("load", load, true);
      deferred.resolve();
    }
  }, true);
  return deferred.promise;
}

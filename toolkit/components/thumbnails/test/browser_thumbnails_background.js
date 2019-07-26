


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

  let nextTest = tests.shift();
  info("starting sub-test " + nextTest.name);
  imports.Task.spawn(nextTest).then(spawnNextTest, function onError(err) {
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
      
      testPageURL({ wait: 2002 }),
      "http://www.example.com/2",
    ];
    let files = urls.map(fileForURL);
    files.forEach(f => ok(!f.exists(), "Thumbnail should not be cached yet."));
    urls.forEach(function (url) {
      let isTimeoutTest = url.indexOf("wait") >= 0;
      imports.BackgroundPageThumbs.capture(url, {
        timeout: isTimeoutTest ? 100 : 30000,
        onDone: function onDone(capturedURL) {
          ok(urls.length > 0, "onDone called, so URLs should still remain");
          is(capturedURL, urls.shift(),
             "Captured URL should be currently expected URL (i.e., " +
             "capture() callbacks should be called in the correct order)");
          let file = files.shift();
          if (isTimeoutTest) {
            ok(!file.exists(),
               "Thumbnail shouldn't exist for timed out capture: " + file.path);
          } else {
            ok(file.exists(),
               "Thumbnail should be cached after capture: " + file.path);
          }
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
    file1.remove(false);

    yield wait(2000);
    is(imports.BackgroundPageThumbs._thumbBrowser, undefined,
       "Thumb browser should be destroyed after timeout.");
    imports.BackgroundPageThumbs._destroyBrowserTimeout = defaultTimeout;

    yield capture(url2);
    ok(file2.exists(), "Second file should exist after capture.");
    file2.remove(false);

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
    ok(file.exists(),
       "Thumbnail file should be created even when a private window is open.");
    file.remove(false);

    win.close();
  },

  function openPrivateWindowDuringCapture() {
    let url = "http://example.com/";
    let file = fileForURL(url);
    ok(!file.exists(), "Thumbnail file should not already exist.");

    let deferred = imports.Promise.defer();

    let waitCount = 0;
    function maybeFinish() {
      if (++waitCount == 2)
        deferred.resolve();
    }

    imports.BackgroundPageThumbs.capture(url, {
      onDone: function (capturedURL) {
        is(capturedURL, url, "Captured URL should be URL passed to capture.");
        ok(file.exists(),
           "Thumbnail file should be created even though a private window " +
           "was opened during the capture.");
        file.remove(false);
        maybeFinish();
      },
    });

    
    
    
    
    
    openPrivateWindow().then(function (win) {
      win.close();
      maybeFinish();
    });

    yield deferred.promise;
  },

  function noCookiesSent() {
    
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

  
  
  function noCookiesStored() {
    let url = testPageURL({ setRedCookie: true });
    let file = fileForURL(url);
    ok(!file.exists(), "Thumbnail file should not exist before capture.");
    yield capture(url);
    ok(file.exists(), "Thumbnail file should exist after capture.");
    file.remove(false);
    
    
    let tab = gBrowser.loadOneTab(url, { inBackground: false });
    let browser = tab.linkedBrowser;
    yield onPageLoad(browser);

    
    let redStr = "rgb(255, 0, 0)";
    isnot(browser.contentDocument.documentElement.style.backgroundColor,
          redStr,
          "The page shouldn't be red.");
    gBrowser.removeTab(tab);
  },

  
  
  
  
  
  
  
  
  function noAuthPrompt() {
    let url = "http://mochi.test:8888/browser/browser/base/content/test/general/authenticate.sjs?user=anyone";
    let file = fileForURL(url);
    ok(!file.exists(), "Thumbnail file should not already exist.");

    let capturedURL = yield capture(url);
    is(capturedURL, url, "Captured URL should be URL passed to capture.");
    ok(file.exists(),
       "Thumbnail file should exist even though it requires auth.");
    file.remove(false);
  },

  function noAlert() {
    let url = "data:text/html,<script>try { alert('yo!'); } catch (e) {}</script>";
    let file = fileForURL(url);
    ok(!file.exists(), "Thumbnail file should not already exist.");

    let capturedURL = yield capture(url);
    is(capturedURL, url, "Captured URL should be URL passed to capture.");
    ok(file.exists(),
       "Thumbnail file should exist even though it alerted.");
    file.remove(false);
  },

  function noDuplicates() {
    let deferred = imports.Promise.defer();
    let url = "http://example.com/1";
    let file = fileForURL(url);
    ok(!file.exists(), "Thumbnail file should not already exist.");
    let numCallbacks = 0;
    let doneCallback = function(doneUrl) {
      is(doneUrl, url, "called back with correct url");
      numCallbacks += 1;
      
      
      
      
      if (numCallbacks == 1) {
        ok(file.exists(), "Thumbnail file should now exist.");
        file.remove(false);
        return;
      }
      if (numCallbacks == 2) {
        ok(!file.exists(), "Thumbnail file should still be deleted.");
        
        deferred.resolve();
        return;
      }
      ok(false, "only expecting 2 callbacks");
    }
    imports.BackgroundPageThumbs.capture(url, {onDone: doneCallback});
    imports.BackgroundPageThumbs.capture(url, {onDone: doneCallback});
    yield deferred.promise;
  },

  function capIfMissing() {
    let url = "http://example.com/";
    let file = fileForURL(url);
    ok(!file.exists(), "Thumbnail file should not already exist.");

    let capturedURL = yield captureIfMissing(url);
    is(capturedURL, url, "Captured URL should be URL passed to capture");
    ok(file.exists(), "Thumbnail should be cached after capture: " + file.path);

    let past = Date.now() - 1000000000;
    let pastFudge = past + 30000;
    file.lastModifiedTime = past;
    ok(file.lastModifiedTime < pastFudge, "Last modified time should stick!");
    capturedURL = yield captureIfMissing(url);
    is(capturedURL, url, "Captured URL should be URL passed to second capture");
    ok(file.exists(), "Thumbnail should remain cached after second capture: " +
                      file.path);
    ok(file.lastModifiedTime < pastFudge,
       "File should not have been overwritten");

    file.remove(false);
  },
];

function capture(url, options) {
  return captureWithMethod("capture", url, options);
}

function captureIfMissing(url, options) {
  return captureWithMethod("captureIfMissing", url, options);
}

function captureWithMethod(methodName, url, options={}) {
  let deferred = imports.Promise.defer();
  options.onDone = function onDone(capturedURL) {
    deferred.resolve(capturedURL);
  };
  imports.BackgroundPageThumbs[methodName](url, options);
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

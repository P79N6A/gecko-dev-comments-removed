


const TEST_PAGE_URL = "http://mochi.test:8888/browser/toolkit/components/thumbnails/test/thumbnails_background.sjs";
const TEST_CONTENT_HELPER = "chrome://mochitests/content/browser/toolkit/components/thumbnails/test/thumbnails_crash_content_helper.js";

const imports = {};
Cu.import("resource://gre/modules/BackgroundPageThumbs.jsm", imports);
Cu.import("resource://gre/modules/PageThumbs.jsm", imports);
Cu.import("resource://gre/modules/Task.jsm", imports);
Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js", imports);

function test() {
  waitForExplicitFinish();

  spawnNextTest();
}

function spawnNextTest() {
  if (!tests.length) {
    finish();
    return;
  }
  let test = tests.shift();
  info("Running sub-test " + test.name);
  imports.Task.spawn(test).then(spawnNextTest, function onError(err) {
    ok(false, err);
    spawnNextTest();
  });
}

let tests = [

  function crashDuringCapture() {
    
    let goodUrl = testPageURL();
    yield capture(goodUrl);
    let goodFile = fileForURL(goodUrl);
    ok(goodFile.exists(), "Thumbnail should be cached after capture: " + goodFile.path);
    goodFile.remove(false);
    
    let mm = injectContentScript();
    
    
    let waitUrl = testPageURL({ wait: 30000 });
    let deferred1 = capture(waitUrl);
    let deferred2 = capture(goodUrl);
    info("Crashing the thumbnail content process.");
    mm.sendAsyncMessage("thumbnails-test:crash");
    yield deferred1;
    let waitFile = fileForURL(waitUrl);
    ok(!waitFile.exists(), "Thumbnail should not have been saved due to the crash: " + waitFile.path);
    yield deferred2;
    ok(goodFile.exists(), "We should have recovered and completed the 2nd capture after the crash: " + goodFile.path);
    goodFile.remove(false);
  },

  function crashWhileIdle() {
    
    let goodUrl = testPageURL();
    yield capture(goodUrl);
    let goodFile = fileForURL(goodUrl);
    ok(goodFile.exists(), "Thumbnail should be cached after capture: " + goodFile.path);
    goodFile.remove(false);
    
    let mm = injectContentScript();
    
    
    
    let deferred = imports.Promise.defer();
    Services.obs.addObserver(function crashObserver() {
      Services.obs.removeObserver(crashObserver, "oop-frameloader-crashed");
      
      executeSoon(function() {
        
        capture(goodUrl).then(() => {
          ok(goodFile.exists(), "We should have recovered and handled new capture requests: " + goodFile.path);
          goodFile.remove(false);
          deferred.resolve();
        });
      });
    } , "oop-frameloader-crashed", false);

    
    info("Crashing the thumbnail content process.");
    mm.sendAsyncMessage("thumbnails-test:crash");
    yield deferred.promise;
  },
];

function injectContentScript() {
  let thumbnailBrowser = imports.BackgroundPageThumbs._thumbBrowser;
  let mm = thumbnailBrowser.messageManager;
  mm.loadFrameScript(TEST_CONTENT_HELPER, false);
  return mm;
}

function capture(url, options) {
  let deferred = imports.Promise.defer();
  options = options || {};
  options.onDone = function onDone(capturedURL) {
    deferred.resolve(capturedURL);
  };
  imports.BackgroundPageThumbs.capture(url, options);
  return deferred.promise;
}

function fileForURL(url) {
  let path = imports.PageThumbsStorage.getFilePathForURL(url);
  let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
  file.initWithPath(path);
  return file;
}

function testPageURL(opts) {
  return TEST_PAGE_URL + "?" + encodeURIComponent(JSON.stringify(opts || {}));
}

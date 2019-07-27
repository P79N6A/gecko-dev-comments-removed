


let tmp = {};
Cu.import("resource://gre/modules/PageThumbs.jsm", tmp);
Cu.import("resource://gre/modules/BackgroundPageThumbs.jsm", tmp);
Cu.import("resource://gre/modules/NewTabUtils.jsm", tmp);
Cu.import("resource:///modules/sessionstore/SessionStore.jsm", tmp);
Cu.import("resource://gre/modules/FileUtils.jsm", tmp);
Cu.import("resource://gre/modules/osfile.jsm", tmp);
let {PageThumbs, BackgroundPageThumbs, NewTabUtils, PageThumbsStorage, SessionStore, FileUtils, OS} = tmp;

Cu.import("resource://gre/modules/PlacesUtils.jsm");

let oldEnabledPref = Services.prefs.getBoolPref("browser.pagethumbnails.capturing_disabled");
Services.prefs.setBoolPref("browser.pagethumbnails.capturing_disabled", false);

registerCleanupFunction(function () {
  while (gBrowser.tabs.length > 1)
    gBrowser.removeTab(gBrowser.tabs[1]);
  Services.prefs.setBoolPref("browser.pagethumbnails.capturing_disabled", oldEnabledPref)
});




function test() {
  TestRunner.run();
}




let TestRunner = {
  


  run: function () {
    waitForExplicitFinish();

    SessionStore.promiseInitialized.then(function () {
      this._iter = runTests();
      if (this._iter) {
        this.next();
      } else {
        finish();
      }
    }.bind(this));
  },

  




  next: function (aValue) {
    try {
      let value = TestRunner._iter.send(aValue);
      if (value && typeof value.then == "function") {
        value.then(result => {
          next(result);
        }, error => {
          ok(false, error + "\n" + error.stack);
        });
      }
    } catch (e if e instanceof StopIteration) {
      finish();
    }
  }
};






function next(aValue) {
  TestRunner.next(aValue);
}






function addTab(aURI, aCallback) {
  let tab = gBrowser.selectedTab = gBrowser.addTab(aURI);
  whenLoaded(tab.linkedBrowser, aCallback);
}





function navigateTo(aURI) {
  let browser = gBrowser.selectedTab.linkedBrowser;
  whenLoaded(browser);
  browser.loadURI(aURI);
}







function whenLoaded(aElement, aCallback = next) {
  aElement.addEventListener("load", function onLoad() {
    aElement.removeEventListener("load", onLoad, true);
    executeSoon(aCallback);
  }, true);
}









function captureAndCheckColor(aRed, aGreen, aBlue, aMessage) {
  let browser = gBrowser.selectedBrowser;
  
  
  dontExpireThumbnailURLs([browser.currentURI.spec]);

  
  PageThumbs.captureAndStore(browser, function () {
    retrieveImageDataForURL(browser.currentURI.spec, function ([r, g, b]) {
      is("" + [r,g,b], "" + [aRed, aGreen, aBlue], aMessage);
      next();
    });
  });
}







function retrieveImageDataForURL(aURL, aCallback) {
  let width = 100, height = 100;
  let thumb = PageThumbs.getThumbnailURL(aURL, width, height);
  
  
  
  
  
  
  
  
  
  addTab("chrome://global/content/mozilla.xhtml", () => {
    let doc = gBrowser.selectedBrowser.contentDocument;
    let htmlns = "http://www.w3.org/1999/xhtml";
    let img = doc.createElementNS(htmlns, "img");
    img.setAttribute("src", thumb);

    whenLoaded(img, function () {
      let canvas = document.createElementNS(htmlns, "canvas");
      canvas.setAttribute("width", width);
      canvas.setAttribute("height", height);

      
      let ctx = canvas.getContext("2d");
      ctx.drawImage(img, 0, 0, width, height);
      let result = ctx.getImageData(0, 0, 100, 100).data;
      gBrowser.removeTab(gBrowser.selectedTab);
      aCallback(result);
    });
  });
}





function thumbnailFile(aURL) {
  return new FileUtils.File(PageThumbsStorage.getFilePathForURL(aURL));
}





function thumbnailExists(aURL) {
  let file = thumbnailFile(aURL);
  return file.exists() && file.fileSize;
}





function removeThumbnail(aURL) {
  let file = thumbnailFile(aURL);
  file.remove(false);
}


















function addVisits(aPlaceInfo, aCallback) {
  let places = [];
  if (aPlaceInfo instanceof Ci.nsIURI) {
    places.push({ uri: aPlaceInfo });
  }
  else if (Array.isArray(aPlaceInfo)) {
    places = places.concat(aPlaceInfo);
  } else {
    places.push(aPlaceInfo)
  }

  
  let now = Date.now();
  for (let i = 0; i < places.length; i++) {
    if (typeof(places[i]) == "string") {
      places[i] = { uri: Services.io.newURI(places[i], "", null) };
    }
    if (!places[i].title) {
      places[i].title = "test visit for " + places[i].uri.spec;
    }
    places[i].visits = [{
      transitionType: places[i].transition === undefined ? PlacesUtils.history.TRANSITION_LINK
                                                         : places[i].transition,
      visitDate: places[i].visitDate || (now++) * 1000,
      referrerURI: places[i].referrer
    }];
  }

  PlacesUtils.asyncHistory.updatePlaces(
    places,
    {
      handleError: function AAV_handleError() {
        throw("Unexpected error in adding visit.");
      },
      handleResult: function () {},
      handleCompletion: function UP_handleCompletion() {
        if (aCallback)
          aCallback();
      }
    }
  );
}





function addVisitsAndRepopulateNewTabLinks(aPlaceInfo, aCallback) {
  addVisits(aPlaceInfo, () => NewTabUtils.links.populateCache(aCallback, true));
}









function whenFileExists(aURL, aCallback = next) {
  let callback = aCallback;
  if (!thumbnailExists(aURL)) {
    callback = function () whenFileExists(aURL, aCallback);
  }

  executeSoon(callback);
}









function whenFileRemoved(aFile, aCallback) {
  let callback = aCallback;
  if (aFile.exists()) {
    callback = function () whenFileRemoved(aFile, aCallback);
  }

  executeSoon(callback || next);
}

function wait(aMillis) {
  setTimeout(next, aMillis);
}






function dontExpireThumbnailURLs(aURLs) {
  let dontExpireURLs = (cb) => cb(aURLs);
  PageThumbs.addExpirationFilter(dontExpireURLs);

  registerCleanupFunction(function () {
    PageThumbs.removeExpirationFilter(dontExpireURLs);
  });
}

function bgCapture(aURL, aOptions) {
  bgCaptureWithMethod("capture", aURL, aOptions);
}

function bgCaptureIfMissing(aURL, aOptions) {
  bgCaptureWithMethod("captureIfMissing", aURL, aOptions);
}

function bgCaptureWithMethod(aMethodName, aURL, aOptions = {}) {
  
  
  dontExpireThumbnailURLs([aURL]);
  if (!aOptions.onDone)
    aOptions.onDone = next;
  BackgroundPageThumbs[aMethodName](aURL, aOptions);
}

function bgTestPageURL(aOpts = {}) {
  let TEST_PAGE_URL = "http://mochi.test:8888/browser/toolkit/components/thumbnails/test/thumbnails_background.sjs";
  return TEST_PAGE_URL + "?" + encodeURIComponent(JSON.stringify(aOpts));
}

function bgAddCrashObserver() {
  let crashed = false;
  Services.obs.addObserver(function crashObserver(subject, topic, data) {
    is(topic, 'ipc:content-shutdown', 'Received correct observer topic.');
    ok(subject instanceof Components.interfaces.nsIPropertyBag2,
       'Subject implements nsIPropertyBag2.');
    
    
    if (!subject.hasKey("abnormal")) {
      info("This is a normal termination and isn't the one we are looking for...");
      return;
    }
    Services.obs.removeObserver(crashObserver, 'ipc:content-shutdown');
    crashed = true;

    var dumpID;
    if ('nsICrashReporter' in Components.interfaces) {
      dumpID = subject.getPropertyAsAString('dumpID');
      ok(dumpID, "dumpID is present and not an empty string");
    }

    if (dumpID) {
      var minidumpDirectory = getMinidumpDirectory();
      removeFile(minidumpDirectory, dumpID + '.dmp');
      removeFile(minidumpDirectory, dumpID + '.extra');
    }
  }, 'ipc:content-shutdown', false);
  return {
    get crashed() crashed
  };
}

function bgInjectCrashContentScript() {
  const TEST_CONTENT_HELPER = "chrome://mochitests/content/browser/toolkit/components/thumbnails/test/thumbnails_crash_content_helper.js";
  let thumbnailBrowser = BackgroundPageThumbs._thumbBrowser;
  let mm = thumbnailBrowser.messageManager;
  mm.loadFrameScript(TEST_CONTENT_HELPER, false);
  return mm;
}

function getMinidumpDirectory() {
  var dir = Services.dirsvc.get('ProfD', Components.interfaces.nsIFile);
  dir.append("minidumps");
  return dir;
}

function removeFile(directory, filename) {
  var file = directory.clone();
  file.append(filename);
  if (file.exists()) {
    file.remove(false);
  }
}

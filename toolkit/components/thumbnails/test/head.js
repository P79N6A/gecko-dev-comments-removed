


let tmp = {};
Cu.import("resource://gre/modules/PageThumbs.jsm", tmp);
Cu.import("resource:///modules/sessionstore/SessionStore.jsm", tmp);
Cu.import("resource://gre/modules/FileUtils.jsm", tmp);
Cu.import("resource://gre/modules/osfile.jsm", tmp);
let {PageThumbs, PageThumbsStorage, SessionStore, FileUtils, OS} = tmp;

Cu.import("resource://gre/modules/PlacesUtils.jsm");

registerCleanupFunction(function () {
  while (gBrowser.tabs.length > 1)
    gBrowser.removeTab(gBrowser.tabs[1]);
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

  


  next: function () {
    try {
      let value = TestRunner._iter.next();
      if (value && typeof value.then == "function")
        value.then(next);
    } catch (e if e instanceof StopIteration) {
      finish();
    }
  }
};




function next() {
  TestRunner.next();
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

  let htmlns = "http://www.w3.org/1999/xhtml";
  let img = document.createElementNS(htmlns, "img");
  img.setAttribute("src", thumb);

  whenLoaded(img, function () {
    let canvas = document.createElementNS(htmlns, "canvas");
    canvas.setAttribute("width", width);
    canvas.setAttribute("height", height);

    
    let ctx = canvas.getContext("2d");
    ctx.drawImage(img, 0, 0, width, height);
    aCallback(ctx.getImageData(0, 0, 100, 100).data);
  });
}





function thumbnailExists(aURL) {
  let file = new FileUtils.File(PageThumbsStorage.getFilePathForURL(aURL));
  return file.exists() && file.fileSize;
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

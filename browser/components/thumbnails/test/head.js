


let tmp = {};
Cu.import("resource:///modules/PageThumbs.jsm", tmp);
let PageThumbs = tmp.PageThumbs;
let PageThumbsStorage = tmp.PageThumbsStorage;

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
    this._iter = runTests();

    if (this._iter)
      this.next();
    else
      finish();
  },

  


  next: function () {
    try {
      TestRunner._iter.next();
    } catch (e if e instanceof StopIteration) {
      finish();
    }
  }
};




function next() {
  TestRunner.next();
}





function addTab(aURI) {
  let tab = gBrowser.selectedTab = gBrowser.addTab(aURI);
  whenLoaded(tab.linkedBrowser);
}





function navigateTo(aURI) {
  let browser = gBrowser.selectedTab.linkedBrowser;
  whenLoaded(browser);
  browser.loadURI(aURI);
}







function whenLoaded(aElement, aCallback) {
  aElement.addEventListener("load", function onLoad() {
    aElement.removeEventListener("load", onLoad, true);
    executeSoon(aCallback || next);
  }, true);
}









function captureAndCheckColor(aRed, aGreen, aBlue, aMessage) {
  let browser = gBrowser.selectedBrowser;

  
  PageThumbs.captureAndStore(browser, function () {
    checkThumbnailColor(browser.currentURI.spec, aRed, aGreen, aBlue, aMessage);
  });
}









function checkThumbnailColor(aURL, aRed, aGreen, aBlue, aMessage) {
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
    checkCanvasColor(ctx, aRed, aGreen, aBlue, aMessage);

    next();
  });
}









function checkCanvasColor(aContext, aRed, aGreen, aBlue, aMessage) {
  let [r, g, b] = aContext.getImageData(0, 0, 1, 1).data;
  ok(r == aRed && g == aGreen && b == aBlue, aMessage);
}

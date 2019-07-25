


let tmp = {};
Cu.import("resource:///modules/PageThumbs.jsm", tmp);
let PageThumbs = tmp.PageThumbs;
let PageThumbsCache = tmp.PageThumbsCache;

registerCleanupFunction(function () {
  while (gBrowser.tabs.length > 1)
    gBrowser.removeTab(gBrowser.tabs[1]);
});

let cachedXULDocument;




function test() {
  TestRunner.run();
}




let TestRunner = {
  


  run: function () {
    waitForExplicitFinish();

    this._iter = runTests();
    this.next();
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
  let window = gBrowser.selectedTab.linkedBrowser.contentWindow;

  let key = Date.now();
  let data = PageThumbs.capture(window);

  
  PageThumbs.store(key, data, function () {
    let width = 100, height = 100;
    let thumb = PageThumbs.getThumbnailURL(key, width, height);

    getXULDocument(function (aDocument) {
      let htmlns = "http://www.w3.org/1999/xhtml";
      let img = aDocument.createElementNS(htmlns, "img");
      img.setAttribute("src", thumb);

      whenLoaded(img, function () {
        let canvas = aDocument.createElementNS(htmlns, "canvas");
        canvas.setAttribute("width", width);
        canvas.setAttribute("height", height);

        
        let ctx = canvas.getContext("2d");
        ctx.drawImage(img, 0, 0, width, height);
        checkCanvasColor(ctx, aRed, aGreen, aBlue, aMessage);

        next();
      });
    });
  });
}






function getXULDocument(aCallback) {
  let hiddenWindow = Services.appShell.hiddenDOMWindow;
  let doc = cachedXULDocument || hiddenWindow.document;

  if (doc instanceof XULDocument) {
    aCallback(cachedXULDocument = doc);
    return;
  }

  let iframe = doc.createElement("iframe");
  iframe.setAttribute("src", "chrome://global/content/mozilla.xhtml");

  iframe.addEventListener("DOMContentLoaded", function onLoad() {
    iframe.removeEventListener("DOMContentLoaded", onLoad, false);
    aCallback(cachedXULDocument = iframe.contentDocument);
  }, false);

  doc.body.appendChild(iframe);
  registerCleanupFunction(function () { doc.body.removeChild(iframe); });
}









function checkCanvasColor(aContext, aRed, aGreen, aBlue, aMessage) {
  let [r, g, b] = aContext.getImageData(0, 0, 1, 1).data;
  ok(r == aRed && g == aGreen && b == aBlue, aMessage);
}

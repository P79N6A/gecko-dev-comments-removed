


function runTests() {
  
  let url = bgTestPageURL({ setGreenCookie: true });
  let tab = gBrowser.loadOneTab(url, { inBackground: false });
  let browser = tab.linkedBrowser;
  yield whenLoaded(browser);

  
  let greenStr = "rgb(0, 255, 0)";
  isnot(browser.contentDocument.documentElement.style.backgroundColor,
        greenStr,
        "The page shouldn't be green yet.");

  
  
  browser.reload();
  yield whenLoaded(browser);
  is(browser.contentDocument.documentElement.style.backgroundColor,
     greenStr,
     "The page should be green now.");

  
  
  yield bgCapture(url);
  ok(thumbnailExists(url), "Thumbnail file should exist after capture.");

  retrieveImageDataForURL(url, function ([r, g, b]) {
    isnot([r, g, b].toString(), [0, 255, 0].toString(),
          "The captured page should not be green.");
    gBrowser.removeTab(tab);
    removeThumbnail(url);
    next();
  });
  yield true;
}

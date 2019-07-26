




function runTests() {
  let url = bgTestPageURL({ setRedCookie: true });
  ok(!thumbnailExists(url), "Thumbnail file should not exist before capture.");
  yield bgCapture(url);
  ok(thumbnailExists(url), "Thumbnail file should exist after capture.");
  removeThumbnail(url);
  
  
  let tab = gBrowser.loadOneTab(url, { inBackground: false });
  let browser = tab.linkedBrowser;
  yield whenLoaded(browser);

  
  let redStr = "rgb(255, 0, 0)";
  isnot(browser.contentDocument.documentElement.style.backgroundColor,
        redStr,
        "The page shouldn't be red.");
  gBrowser.removeTab(tab);
}

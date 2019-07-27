



add_task(function* () {
  let firstLocation = "http://example.org/browser/browser/base/content/test/general/dummy_page.html";
  yield BrowserTestUtils.openNewForegroundTab(gBrowser, firstLocation);

  yield ContentTask.spawn(gBrowser.selectedBrowser, {}, function* () {
    
    content.history.pushState("page2", "page2", "page2");

    
    
    content.addEventListener("popstate", function onPopState() {
      content.removeEventListener("popstate", onPopState, false);
      sendAsyncMessage("Test:PopStateOccurred", { location: content.document.location.href });
    }, false);
  });

  window.maximize();

  
  var navBar = document.getElementById("nav-bar");
  var boundingRect = navBar.getBoundingClientRect();
  var yPixel = boundingRect.top + Math.floor(boundingRect.height / 2);
  var xPixel = 0; 

  let resultLocation = yield new Promise(resolve => {
    messageManager.addMessageListener("Test:PopStateOccurred", function statePopped(message) {
      messageManager.removeMessageListener("Test:PopStateOccurred", statePopped);
      resolve(message.data.location);
    });

    EventUtils.synthesizeMouseAtPoint(xPixel, yPixel, {}, window);
  });

  is(resultLocation, firstLocation, "Clicking the first pixel should have navigated back.");
  window.restore();

  gBrowser.removeCurrentTab();
});

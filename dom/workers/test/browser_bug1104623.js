



function whenBrowserLoaded(aBrowser, aCallback) {
  aBrowser.addEventListener("load", function onLoad(event) {
    if (event.target == aBrowser.contentDocument) {
      aBrowser.removeEventListener("load", onLoad, true);
      executeSoon(aCallback);
    }
  }, true);
}

function test() {
  waitForExplicitFinish();

  let testURL = "chrome://mochitests/content/chrome/dom/base/test/file_empty.html";

  let tab = gBrowser.addTab(testURL);
  gBrowser.selectedTab = tab;

  whenBrowserLoaded(tab.linkedBrowser, function() {
    let doc = tab.linkedBrowser.contentDocument;
    let contentWin = tab.linkedBrowser.contentWindow;

    let blob = new contentWin.Blob(['onmessage = function() { postMessage(true); }']);
    ok(blob, "Blob has been created");

    let blobURL = contentWin.URL.createObjectURL(blob);
    ok(blobURL, "Blob URL has been created");

    let worker = new contentWin.Worker(blobURL);
    ok(worker, "Worker has been created");

    worker.onerror = function(error) {
      ok(false, "Worker.onerror:" + error.message);
      contentWin.URL.revokeObjectURL(blob);
      gBrowser.removeTab(tab);
      executeSoon(finish);
    }

    worker.onmessage = function() {
      ok(true, "Worker.onmessage");
      contentWin.URL.revokeObjectURL(blob);
      gBrowser.removeTab(tab);
      executeSoon(finish);
    }

    worker.postMessage(true);
  });
}

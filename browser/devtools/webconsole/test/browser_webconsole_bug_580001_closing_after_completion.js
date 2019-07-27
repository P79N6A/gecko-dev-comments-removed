







const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/" +
                 "test/test-console.html";

let test = asyncTest(function* () {
  let { browser } = yield loadTab(TEST_URI);

  let hud = yield openConsole();
  yield testClosingAfterCompletion(hud, browser);
});

function testClosingAfterCompletion(hud, browser) {
  let deferred = promise.defer();

  let inputNode = hud.jsterm.inputNode;

  let errorWhileClosing = false;
  function errorListener() {
    errorWhileClosing = true;
  }

  browser.addEventListener("error", errorListener, false);

  
  inputNode.focus();

  gDevTools.once("toolbox-destroyed", function() {
    browser.removeEventListener("error", errorListener, false);
    is(errorWhileClosing, false, "no error while closing the WebConsole");
    deferred.resolve();
  });

  if (Services.appinfo.OS == "Darwin") {
    EventUtils.synthesizeKey("i", { accelKey: true, altKey: true });
  } else {
    EventUtils.synthesizeKey("i", { accelKey: true, shiftKey: true });
  }

  return deferred.promise;
}


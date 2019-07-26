





const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-error.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, testViewSource);
  }, true);
}

function testViewSource(hud) {
  let button = content.document.querySelector("button");
  button = XPCNativeWrapper.unwrap(button);
  ok(button, "we have the button on the page");

  expectUncaughtException();
  EventUtils.sendMouseEvent({ type: "click" }, button, XPCNativeWrapper.unwrap(content));

  waitForSuccess({
    name: "find the location node",
    validatorFn: function()
    {
      return hud.outputNode.querySelector(".webconsole-location");
    },
    successFn: function()
    {
      let locationNode = hud.outputNode.querySelector(".webconsole-location");

      Services.ww.registerNotification(observer);

      EventUtils.sendMouseEvent({ type: "click" }, locationNode);
    },
    failureFn: finishTest,
  });
}

let observer = {
  observe: function(aSubject, aTopic, aData) {
    if (aTopic != "domwindowopened") {
      return;
    }

    ok(true, "the view source window was opened in response to clicking " +
       "the location node");

    
    executeSoon(function() {
      aSubject.close();
      finishTest();
    });
  }
};

registerCleanupFunction(function() {
  Services.ww.unregisterNotification(observer);
});

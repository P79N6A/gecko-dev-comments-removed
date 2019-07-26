






const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console.html?" + Date.now();

function test()
{
  Services.obs.addObserver(function observer(aSubject) {
    Services.obs.removeObserver(observer, "web-console-created");
    aSubject.QueryInterface(Ci.nsISupportsString);

    let hud = HUDService.getBrowserConsole();
    ok(hud, "browser console is open");
    is(aSubject.data, hud.hudId, "notification hudId is correct");

    executeSoon(() => consoleOpened(hud));
  }, "web-console-created", false);

  let hud = HUDService.getBrowserConsole();
  ok(!hud, "browser console is not open");
  info("wait for the browser console to open with ctrl-shift-j");
  EventUtils.synthesizeKey("j", { accelKey: true, shiftKey: true }, window);
}

function consoleOpened(hud)
{
  hud.jsterm.clearOutput(true);

  expectUncaughtException();
  executeSoon(() => {
    foobarExceptionBug587757();
  });

  
  hud.iframeWindow.console.log("bug587757a");

  
  content.console.log("bug587757b");

  
  hud.jsterm.execute("document.location.href");

  
  let xhr = new XMLHttpRequest();
  xhr.onload = () => console.log("xhr loaded, status is: " + xhr.status);
  xhr.open("get", TEST_URI, true);
  xhr.send();

  waitForMessages({
    webconsole: hud,
    messages: [
      {
        name: "chrome window console.log() is displayed",
        text: "bug587757a",
        category: CATEGORY_WEBDEV,
        severity: SEVERITY_LOG,
      },
      {
        name: "content window console.log() is displayed",
        text: "bug587757b",
        category: CATEGORY_WEBDEV,
        severity: SEVERITY_LOG,
      },
      {
        name: "jsterm eval result",
        text: "browser.xul",
        category: CATEGORY_OUTPUT,
        severity: SEVERITY_LOG,
      },
      {
        name: "exception message",
        text: "foobarExceptionBug587757",
        category: CATEGORY_JS,
        severity: SEVERITY_ERROR,
      },
      {
        name: "network message",
        text: "test-console.html",
        category: CATEGORY_NETWORK,
        severity: SEVERITY_LOG,
      },
    ],
  }).then(finishTest);
}

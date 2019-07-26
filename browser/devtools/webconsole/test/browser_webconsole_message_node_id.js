




const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", onLoad, false);
}

function onLoad() {
  browser.removeEventListener("DOMContentLoaded", onLoad, false);
  openConsole(null, function(hud) {
    content.console.log("a log message");

    waitForMessages({
      webconsole: hud,
      messages: [{
        text: "a log message",
        category: CATEGORY_WEBDEV,
        severity: SEVERITY_LOG,
      }],
    }).then(([result]) => {
      let msg = [...result.matched][0];
      ok(msg.getAttribute("id"), "log message has an ID");
      finishTest();
    });
  });
}

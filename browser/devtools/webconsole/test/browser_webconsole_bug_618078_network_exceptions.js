







const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-bug-618078-network-exceptions.html";

function test()
{
  addTab("data:text/html;charset=utf-8,Web Console test for bug 618078");

  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, function(hud) {
      expectUncaughtException();
      content.location = TEST_URI;

      waitForMessages({
        webconsole: hud,
        messages: [{
          text: "bug618078exception",
          category: CATEGORY_JS,
          severity: SEVERITY_ERROR,
        }],
      }).then(finishTest);
    });
  }, true);
}

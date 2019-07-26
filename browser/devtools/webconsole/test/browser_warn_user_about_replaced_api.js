




const TEST_REPLACED_API_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console-replaced-api.html";
const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/testscript.js";

function test() {
  waitForExplicitFinish();

  
  
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, testWarningNotPresent);
  }, true);

  function testWarningNotPresent(hud)
  {
    is(hud.outputNode.textContent.indexOf("logging API"), -1,
       "no warning displayed");

    
    info("reload " + TEST_URI);
    executeSoon(() => content.location.reload());

    waitForMessages({
      webconsole: hud,
      messages: [{
        text: "testscript.js",
        category: CATEGORY_NETWORK,
      }],
    }).then(() => executeSoon(() => {
      is(hud.outputNode.textContent.indexOf("logging API"), -1,
         "no warning displayed");

      closeConsole(null, loadTestPage);
    }));
  }

  function loadTestPage()
  {
    info("load test " + TEST_REPLACED_API_URI);
    browser.addEventListener("load", function onLoad() {
      browser.removeEventListener("load", onLoad, true);
      openConsole(null, testWarningPresent);
    }, true);
    content.location = TEST_REPLACED_API_URI;
  }

  function testWarningPresent(hud)
  {
    info("wait for the warning to show");
    let warning = {
      webconsole: hud,
      messages: [{
        text: /logging API .+ disabled by a script/,
        category: CATEGORY_JS,
        severity: SEVERITY_WARNING,
      }],
    };

    waitForMessages(warning).then(() => {
      hud.jsterm.clearOutput();

      executeSoon(() => {
        info("reload the test page and wait for the warning to show");
        waitForMessages(warning).then(finishTest);
        content.location.reload();
      });
    });
  }
}

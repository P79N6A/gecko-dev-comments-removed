








const TEST_VIOLATION = "https://example.com/browser/browser/devtools/webconsole/test/test_bug_770099_violation.html";
const CSP_VIOLATION_MSG = "CSP WARN:  Directive default-src https://example.com:443 violated by http://some.example.com/test.png"

let hud = undefined;

function test() {
  addTab("data:text/html;charset=utf8,Web Console CSP violation test");
  browser.addEventListener("load", function _onLoad() {
    browser.removeEventListener("load", _onLoad, true);
    openConsole(null, loadDocument);
  }, true);
}

function loadDocument(theHud){
  hud = theHud;
  hud.jsterm.clearOutput()
  browser.addEventListener("load", onLoad, true);
  content.location = TEST_VIOLATION;
}

function onLoad(aEvent) {
  browser.removeEventListener("load", onLoad, true);
  testViolationMessage();
}

function testViolationMessage(){
  let aOutputNode = hud.outputNode;

  waitForSuccess({
      name: "CSP policy URI warning displayed successfully",
      validatorFn: function() {
        return hud.outputNode.textContent.indexOf(CSP_VIOLATION_MSG) > -1;
      },
      successFn: finishTest,
      failureFn: finishTest,
    });
}

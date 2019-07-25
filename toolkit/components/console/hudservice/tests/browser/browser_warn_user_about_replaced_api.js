




































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/HUDService.jsm");

const TEST_REPLACED_API_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console-replaced-api.html";

function log(aMsg)
{
  dump("*** WebConsoleTest: " + aMsg + "\n");
}

function testOpenWebConsole()
{
  HUDService.activateHUDForContext(gBrowser.selectedTab);
  is(HUDService.displaysIndex().length, 1, "WebConsole was opened");

  hudId = HUDService.displaysIndex()[0];
  hud = HUDService.getHeadsUpDisplay(hudId);

  HUDService.logWarningAboutReplacedAPI(hudId);
}

function testWarning()
{
  const successMsg = "Found the warning message";
  const errMsg = "Could not find the warning message about the replaced API";

  var display = HUDService.getDisplayByURISpec(content.location.href);
  var outputNode = display.querySelectorAll(".hud-output-node")[0];
  executeSoon(function () {
    testLogEntry(outputNode, "disabled", { success: successMsg, err: errMsg });
  });
}

function testLogEntry(aOutputNode, aMatchString, aSuccessErrObj)
{
  var message = aOutputNode.textContent.indexOf(aMatchString);
  if (message > -1) {
    ok(true, aSuccessErrObj.success);
  return;
  }
  ok(false, aSuccessErrObj.err);
}

function finishTest() {
  hud = null;
  hudId = null;

  executeSoon(function() {
    finish();
  });
}

let hud, hudId, tab, browser, filterBox, outputNode;
let win = gBrowser.selectedBrowser;

tab = gBrowser.selectedTab;
browser = gBrowser.getBrowserForTab(tab);

content.location.href = TEST_REPLACED_API_URI;

function test() {
  waitForExplicitFinish();
  browser.addEventListener("DOMContentLoaded", function onLoad(event) {
    browser.removeEventListener("DOMContentLoaded", onLoad, false);
    executeSoon(function (){
      testOpenWebConsole();
      executeSoon(function (){
        testWarning();
      });
    });
  }, false);
  finishTest();
}

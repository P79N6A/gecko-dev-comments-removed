












const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const TEST_URI = "http://example.com/";

XPCOMUtils.defineLazyGetter(this, "HUDService", function () {
  Cu.import("resource:///modules/HUDService.jsm");
  return HUDService;
});

function test()
{
  waitForExplicitFinish();
  gBrowser.selectedTab = gBrowser.addTab(TEST_URI);
  gBrowser.selectedBrowser.addEventListener("DOMContentLoaded", onLoad, false);
}

function onLoad() {
  gBrowser.selectedBrowser.removeEventListener("DOMContentLoaded", onLoad,
                                               false);
  executeSoon(testNewlines);
}

function testNewlines() {
  HUDService.activateHUDForContext(gBrowser.selectedTab);
  let hudId = HUDService.displaysIndex()[0];
  ok(hudId != null, "we have the HUD ID");

  HUDService.clearDisplay(hudId);

  let contentWindow = gBrowser.selectedTab.linkedBrowser.contentWindow;
  let console = contentWindow.wrappedJSObject.console;
  ok(console != null, "we have the console object");

  for (let i = 0; i < 20; i++) {
    console.log("Hello world!");
  }

  let hudNode = HUDService.getOutputNodeById(hudId);
  let outputNode = hudNode.querySelector(".hud-output-node");
  ok(outputNode != null, "we have the output node");

  let labels = outputNode.querySelectorAll("label");
  is(labels.length, 20, "we found 20 labels in the output node");

  for (let i = 0; i < labels.length; i++) {
    let value = labels[i].textContent;
    is(value[value.length - 1], "\n", "the value of label " + i + " ends " +
       "with a newline");
  }

  HUDService.deactivateHUDForContext(gBrowser.selectedTab);
  gBrowser.removeCurrentTab();
  finish();
}


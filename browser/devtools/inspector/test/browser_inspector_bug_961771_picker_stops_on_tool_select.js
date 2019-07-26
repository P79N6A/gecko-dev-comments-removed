





function test() {
  let {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
  let {require} = devtools;
  let promise = require("sdk/core/promise");
  let {Task} = Cu.import("resource://gre/modules/Task.jsm", {});

  waitForExplicitFinish();

  let inspector, doc, toolbox;

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onload() {
    gBrowser.selectedBrowser.removeEventListener("load", onload, true);
    doc = content.document;
    waitForFocus(setupTest, content);
  }, true);
  content.location = "data:text/html,testing the highlighter goes away on tool selection";

  function setupTest() {
    openInspector((aInspector, aToolbox) => {
      toolbox = aToolbox;
      inspector = aInspector;

      toolbox.once("picker-stopped", () => {
        ok(true, "picker-stopped event fired after switch tools, so picker is closed");
        finishUp();
      });

      Task.spawn(function() {
        yield toolbox.startPicker();
        yield toolbox.selectNextTool();
      }).then(null, Cu.reportError);
    });
  }

  function finishUp() {
    inspector = doc = toolbox = null;
    gBrowser.removeCurrentTab();
    finish();
  }
}


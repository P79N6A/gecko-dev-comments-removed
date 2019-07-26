






function test() {
  let {Task} = Cu.import("resource://gre/modules/Task.jsm", {});

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onload() {
    gBrowser.selectedBrowser.removeEventListener("load", onload, true);
    waitForFocus(runTest, content);
  }, true);
  content.location = "data:text/html,<p>testing the highlighter goes away on destroy</p>";

  function runTest() {
    openInspector((inspector, toolbox) => {
      let pickerStopped = toolbox.once("picker-stopped");

      Task.spawn(function() {
        
        
        
        yield inspector.selection.setNode(content.document.querySelector("p"));
        yield inspector.once("inspector-updated");
        info("inspector displayed and ready, starting the picker");
        yield toolbox.highlighterUtils.startPicker();
        info("destroying the toolbox");
        yield toolbox.destroy();
        info("waiting for the picker-stopped event that should be fired when the toolbox is destroyed");
        yield pickerStopped;
        ok(true, "picker-stopped event fired after switch tools, so picker is closed");
      }).then(null, ok.bind(null, false)).then(finishUp);
    });
  }

  function finishUp() {
    gBrowser.removeCurrentTab();
    finish();
  }
}

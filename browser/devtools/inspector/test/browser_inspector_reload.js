






function test() {
  let inspector, toolbox;

  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onload() {
    gBrowser.selectedBrowser.removeEventListener("load", onload, true);
    waitForFocus(function() {
      let target = TargetFactory.forTab(gBrowser.selectedTab);
      gDevTools.showToolbox(target, "inspector").then(function(toolbox) {
        startInspectorTests(toolbox);
      }).then(null, console.error);
    }, content);
  }, true);

  
  
  
  content.location = "data:text/html,<p id='1'>p</p>";

  function startInspectorTests(aToolbox)
  {
    toolbox = aToolbox;
    inspector = toolbox.getCurrentPanel();
    info("Inspector started");
    let p = content.document.querySelector("p");
    inspector.selection.setNode(p);
    inspector.once("inspector-updated", () => {
      is(inspector.selection.node, p, "Node selected.");
      inspector.once("markuploaded", onReload);
      content.location.reload();
    });
  }

  function onReload() {
    info("Page reloaded");
    let p = content.document.querySelector("p");
    inspector.selection.setNode(p);
    inspector.once("inspector-updated", () => {
      is(inspector.selection.node, p, "Node re-selected.");
      toolbox.destroy();
      toolbox = inspector = null;
      gBrowser.removeCurrentTab();
      finish();
    });
  }
}

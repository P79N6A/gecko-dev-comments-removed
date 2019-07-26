


function test() {
  addTab().then(function(data) {
    data.target.makeRemote().then(performChecks.bind(null, data));
  }).then(null, console.error);

  function performChecks(data) {
    let toolIds = gDevTools.getToolDefinitionArray()
                    .filter(def => def.isTargetSupported(data.target))
                    .map(def => def.id);

    let open = function(index) {
      let toolId = toolIds[index];

      info("About to open " + index + "/" + toolId);
      gDevTools.showToolbox(data.target, toolId).then(function(toolbox) {
        ok(toolbox, "toolbox exists for " + toolId);
        is(toolbox.currentToolId, toolId, "currentToolId should be " + toolId);

        let panel = toolbox.getCurrentPanel();
        ok(panel.isReady, toolId + " panel should be ready");

        let nextIndex = index + 1;
        if (nextIndex >= toolIds.length) {
          toolbox.destroy().then(function() {
            gBrowser.removeCurrentTab();
            finish();
          });
        }
        else {
          open(nextIndex);
        }
      }, console.error);
    };

    open(0);
  }
}

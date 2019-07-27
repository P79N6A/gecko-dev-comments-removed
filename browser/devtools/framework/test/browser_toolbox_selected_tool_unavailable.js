

"use strict";




const testToolDefinition = {
    id: "test-tool",
    isTargetSupported: () => true,
    visibilityswitch: "devtools.test-tool.enabled",
    url: "about:blank",
    label: "someLabel",
    build: (iframeWindow, toolbox) => {
      return {
        target: toolbox.target,
        toolbox: toolbox,
        isReady: true,
        destroy: () => {},
        panelDoc: iframeWindow.document
      };
    }
  };

add_task(function*() {
  gDevTools.registerTool(testToolDefinition);
  let tab = yield addTab("about:blank");
  let target = TargetFactory.forTab(tab);

  let toolbox = yield gDevTools.showToolbox(target, testToolDefinition.id);
  is(toolbox.currentToolId, "test-tool", "test-tool was selected");
  yield gDevTools.closeToolbox(target);

  
  testToolDefinition.isTargetSupported = () => false;

  target = TargetFactory.forTab(tab);
  toolbox = yield gDevTools.showToolbox(target);
  is(toolbox.currentToolId, "webconsole", "web console was selected");

  yield gDevTools.closeToolbox(target);
  gDevTools.unregisterTool(testToolDefinition.id);
  tab = toolbox = target = null;
  gBrowser.removeCurrentTab();
});

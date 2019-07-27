



"use strict";




add_task(function*() {
  
  
  
  ok(true);

  yield addTab("data:text/html;charset=utf-8,test inspector destroy");

  info("Open the toolbox on the debugger panel");
  let target = TargetFactory.forTab(gBrowser.selectedTab);
  let toolbox = yield gDevTools.showToolbox(target, "jsdebugger");

  info("Switch to the inspector panel and immediately end the test");
  let onInspectorSelected = toolbox.once("inspector-selected");
  toolbox.selectTool("inspector");
  let inspector = yield onInspectorSelected;
});

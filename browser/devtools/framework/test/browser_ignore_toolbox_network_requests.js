

"use strict";




add_task(function*() {
  
  
  
  let isTesting = DevToolsUtils.testing;
  DevToolsUtils.testing = false;

  let tab = yield addTab(URL_ROOT + "doc_viewsource.html");
  let target = TargetFactory.forTab(tab);
  let toolbox = yield gDevTools.showToolbox(target, "styleeditor");
  let panel = toolbox.getPanel("styleeditor");

  is(panel.UI.editors.length, 1, "correct number of editors opened");

  let monitor = yield toolbox.selectTool("netmonitor");
  let { RequestsMenu } = monitor.panelWin.NetMonitorView;
  is(RequestsMenu.itemCount, 0, "No network requests appear in the network panel");

  yield gDevTools.closeToolbox(target);
  tab = target = toolbox = panel = null;
  gBrowser.removeCurrentTab();
  DevToolsUtils.testing = isTesting;
});

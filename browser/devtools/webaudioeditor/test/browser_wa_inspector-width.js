

 




 
add_task(function*() {
  let { target, panel } = yield initWebAudioEditor(SIMPLE_CONTEXT_URL);
  let { panelWin } = panel;
  let { gFront, $, $$, EVENTS, InspectorView } = panelWin;
  let gVars = InspectorView._propsView;
 
  let started = once(gFront, "start-context");
 
  reload(target);
 
  let [actors] = yield Promise.all([
    get3(gFront, "create-node"),
    waitForGraphRendered(panelWin, 3, 2)
  ]);
  let nodeIds = actors.map(actor => actor.actorID);
 
  ok(!InspectorView.isVisible(), "InspectorView hidden on start.");
 
  
  $("#inspector-pane-toggle").click();
  yield once(panelWin, EVENTS.UI_INSPECTOR_TOGGLED);
 
  let newInspectorWidth = 500;
 
  
  $("#web-audio-inspector").setAttribute("width", newInspectorWidth);
  reload(target);
 
  
  [actors] = yield Promise.all([
    get3(gFront, "create-node"),
    waitForGraphRendered(panelWin, 3, 2)
  ]);
  nodeIds = actors.map(actor => actor.actorID);
 
  
  $("#inspector-pane-toggle").click();
  yield once(panelWin, EVENTS.UI_INSPECTOR_TOGGLED);
 
  let nodeSet = Promise.all([
    once(panelWin, EVENTS.UI_INSPECTOR_NODE_SET),
    once(panelWin, EVENTS.UI_PROPERTIES_TAB_RENDERED),
    once(panelWin, EVENTS.UI_AUTOMATION_TAB_RENDERED)
  ]);
 
  click(panelWin, findGraphNode(panelWin, nodeIds[1]));
  yield nodeSet;
 
  
  let width = $("#web-audio-inspector").getAttribute("width");
 
  is(width, newInspectorWidth, "WebAudioEditor's Inspector width should be saved as a preference");
 
  yield teardown(target);
});
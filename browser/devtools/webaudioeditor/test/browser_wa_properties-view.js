






function spawnTest() {
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

  
  click(panelWin, findGraphNode(panelWin, nodeIds[2]));
  yield once(panelWin, EVENTS.UI_INSPECTOR_NODE_SET);

  ok(isVisible($("#properties-tabpanel-content")), "Parameters shown when they exist.");
  ok(!isVisible($("#properties-tabpanel-content-empty")),
    "Empty message hidden when AudioParams exist.");

  
  click(panelWin, findGraphNode(panelWin, nodeIds[0]));
  yield once(panelWin, EVENTS.UI_INSPECTOR_NODE_SET);

  ok(!isVisible($("#properties-tabpanel-content")),
    "Parameters hidden when they don't exist.");
  ok(isVisible($("#properties-tabpanel-content-empty")),
    "Empty message shown when no AudioParams exist.");

  yield teardown(panel);
  finish();
}

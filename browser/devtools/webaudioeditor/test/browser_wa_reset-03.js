







function spawnTest() {
  let [target, debuggee, panel] = yield initWebAudioEditor(SIMPLE_CONTEXT_URL);
  let { panelWin } = panel;
  let { gFront, $, InspectorView } = panelWin;

  reload(target);

  let [actors] = yield Promise.all([
    get3(gFront, "create-node"),
    waitForGraphRendered(panelWin, 3, 2)
  ]);
  let nodeIds = actors.map(actor => actor.actorID);

  yield clickGraphNode(panelWin, nodeIds[1], true);
  ok(InspectorView.isVisible(), "InspectorView visible after selecting a node.");
  is(InspectorView.getCurrentAudioNode().id, nodeIds[1], "InspectorView has correct node set.");

  



  reload(target);

  [actors] = yield Promise.all([
    get3(gFront, "create-node"),
    waitForGraphRendered(panelWin, 3, 2)
  ]);
  nodeIds = actors.map(actor => actor.actorID);

  ok(!InspectorView.isVisible(), "InspectorView hidden on start.");
  ise(InspectorView.getCurrentAudioNode(), null,
    "InspectorView has no current node set on reset.");

  yield clickGraphNode(panelWin, nodeIds[2], true);
  ok(InspectorView.isVisible(),
    "InspectorView visible after selecting a node after a reset.");
  is(InspectorView.getCurrentAudioNode().id, nodeIds[2], "InspectorView has correct node set upon clicking graph node after a reset.");

  yield teardown(panel);
  finish();
}

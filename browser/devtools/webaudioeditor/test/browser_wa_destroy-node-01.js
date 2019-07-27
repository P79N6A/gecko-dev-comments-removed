










function spawnTest() {
  let [target, debuggee, panel] = yield initWebAudioEditor(DESTROY_NODES_URL);
  let { panelWin } = panel;
  let { gFront, $, $$, EVENTS } = panelWin;

  let started = once(gFront, "start-context");

  reload(target);

  let destroyed = getN(panelWin, EVENTS.DESTROY_NODE, 10);

  forceCC();

  let [created] = yield Promise.all([
    getNSpread(panelWin, EVENTS.CREATE_NODE, 13),
    waitForGraphRendered(panelWin, 13, 2)
  ]);

  
  
  let actorIDs = created.map(ev => ev[1]);

  
  yield clickGraphNode(panelWin, actorIDs[5]);

  forceCC();

  
  yield Promise.all([destroyed, waitForGraphRendered(panelWin, 3, 2)]);

  
  is(panelWin.AudioNodes.length, 3, "All nodes should be GC'd except one gain, osc and dest node.");

  
  ok(findGraphNode(panelWin, actorIDs[0]), "dest should be in graph");
  ok(findGraphNode(panelWin, actorIDs[1]), "osc should be in graph");
  ok(findGraphNode(panelWin, actorIDs[2]), "gain should be in graph");

  let { nodes, edges } = countGraphObjects(panelWin);

  is(nodes, 3, "Only 3 nodes rendered in graph.");
  is(edges, 2, "Only 2 edges rendered in graph.");

  
  ok(isVisible($("#web-audio-editor-details-pane-empty")),
    "InspectorView empty message should show if the currently selected node gets collected.");

  yield teardown(panel);
  finish();
}


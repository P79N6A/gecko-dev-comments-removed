







function spawnTest() {
  let { target, panel } = yield initWebAudioEditor(SIMPLE_CONTEXT_URL);
  let { panelWin } = panel;
  let { gFront, $ } = panelWin;

  reload(target);

  var [actors] = yield Promise.all([
    get3(gFront, "create-node"),
    waitForGraphRendered(panelWin, 3, 2)
  ]);

  var { nodes, edges } = countGraphObjects(panelWin);
  ise(nodes, 3, "should only be 3 nodes.");
  ise(edges, 2, "should only be 2 edges.");

  navigate(target, SIMPLE_NODES_URL);

  var [actors] = yield Promise.all([
    getN(gFront, "create-node", 14),
    waitForGraphRendered(panelWin, 14, 0)
  ]);

  is($("#reload-notice").hidden, true,
    "The 'reload this page' notice should be hidden after context found after navigation.");
  is($("#waiting-notice").hidden, true,
    "The 'waiting for an audio context' notice should be hidden after context found after navigation.");
  is($("#content").hidden, false,
    "The tool's content should reappear without closing and reopening the toolbox.");

  var { nodes, edges } = countGraphObjects(panelWin);
  ise(nodes, 14, "after navigation, should have 14 nodes");
  ise(edges, 0, "after navigation, should have 0 edges.");

  yield teardown(panel);
  finish();
}









function spawnTest() {
  let { target, panel } = yield initWebAudioEditor(SIMPLE_CONTEXT_URL);
  let { panelWin } = panel;
  let { gFront, $ } = panelWin;

  reload(target);

  let [actors] = yield Promise.all([
    get3(gFront, "create-node"),
    waitForGraphRendered(panelWin, 3, 2)
  ]);

  let { nodes, edges } = countGraphObjects(panelWin);
  ise(nodes, 3, "should only be 3 nodes.");
  ise(edges, 2, "should only be 2 edges.");

  reload(target);

  [actors] = yield Promise.all([
    get3(gFront, "create-node"),
    waitForGraphRendered(panelWin, 3, 2)
  ]);

  ({ nodes, edges } = countGraphObjects(panelWin));
  ise(nodes, 3, "after reload, should only be 3 nodes.");
  ise(edges, 2, "after reload, should only be 2 edges.");

  yield teardown(panel);
  finish();
}

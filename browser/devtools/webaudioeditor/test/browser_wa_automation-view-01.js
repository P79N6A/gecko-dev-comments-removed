







add_task(function*() {
  let { target, panel } = yield initWebAudioEditor(AUTOMATION_URL);
  let { panelWin } = panel;
  let { gFront, $, $$, EVENTS } = panelWin;

  let started = once(gFront, "start-context");

  reload(target);

  let [actors] = yield Promise.all([
    get3(gFront, "create-node"),
    waitForGraphRendered(panelWin, 3, 2)
  ]);
  let nodeIds = actors.map(actor => actor.actorID);

  
  click(panelWin, findGraphNode(panelWin, nodeIds[1]));
  yield waitForInspectorRender(panelWin, EVENTS);
  click(panelWin, $("#automation-tab"));

  ok(isVisible($("#automation-graph-container")), "graph container should be visible");
  ok(isVisible($("#automation-content")), "automation content should be visible");
  ok(!isVisible($("#automation-no-events")), "no-events panel should not be visible");
  ok(!isVisible($("#automation-empty")), "empty panel should not be visible");

  
  click(panelWin, findGraphNode(panelWin, nodeIds[2]));
  yield waitForInspectorRender(panelWin, EVENTS);
  click(panelWin, $("#automation-tab"));

  ok(!isVisible($("#automation-graph-container")), "graph container should be visible");
  ok(isVisible($("#automation-content")), "automation content should not be visible");
  ok(isVisible($("#automation-no-events")), "no-events panel should be visible");
  ok(!isVisible($("#automation-empty")), "empty panel should not be visible");

  
  click(panelWin, findGraphNode(panelWin, nodeIds[0]));
  yield waitForInspectorRender(panelWin, EVENTS);
  click(panelWin, $("#automation-tab"));

  ok(!isVisible($("#automation-graph-container")), "graph container should not be visible");
  ok(!isVisible($("#automation-content")), "automation content should not be visible");
  ok(!isVisible($("#automation-no-events")), "no-events panel should not be visible");
  ok(isVisible($("#automation-empty")), "empty panel should be visible");

  yield teardown(target);
});

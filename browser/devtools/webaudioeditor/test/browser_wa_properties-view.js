






add_task(function*() {
  let { target, panel } = yield initWebAudioEditor(SIMPLE_CONTEXT_URL);
  let { panelWin } = panel;
  let { gFront, $, $$, EVENTS, PropertiesView } = panelWin;
  let gVars = PropertiesView._propsView;

  let started = once(gFront, "start-context");

  reload(target);

  let [actors] = yield Promise.all([
    get3(gFront, "create-node"),
    waitForGraphRendered(panelWin, 3, 2)
  ]);
  let nodeIds = actors.map(actor => actor.actorID);

  
  click(panelWin, findGraphNode(panelWin, nodeIds[2]));
  yield waitForInspectorRender(panelWin, EVENTS);

  ok(isVisible($("#properties-content")), "Parameters shown when they exist.");
  ok(!isVisible($("#properties-empty")),
    "Empty message hidden when AudioParams exist.");

  
  click(panelWin, findGraphNode(panelWin, nodeIds[0]));
  yield waitForInspectorRender(panelWin, EVENTS);

  ok(!isVisible($("#properties-content")),
    "Parameters hidden when they don't exist.");
  ok(isVisible($("#properties-empty")),
    "Empty message shown when no AudioParams exist.");

  yield teardown(target);
});

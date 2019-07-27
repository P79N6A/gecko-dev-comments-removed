






add_task(function*() {
  let { target, panel } = yield initWebAudioEditor(COMPLEX_CONTEXT_URL);
  let { panelWin } = panel;
  let { gFront, $, $$, EVENTS, PropertiesView } = panelWin;
  let gVars = PropertiesView._propsView;

  let started = once(gFront, "start-context");

  reload(target);

  let [actors] = yield Promise.all([
    getN(gFront, "create-node", 8),
    waitForGraphRendered(panelWin, 8, 8)
  ]);
  let nodeIds = actors.map(actor => actor.actorID);

  click(panelWin, findGraphNode(panelWin, nodeIds[3]));
  
  yield Promise.all([
    waitForInspectorRender(panelWin, EVENTS),
    once(panelWin, EVENTS.UI_INSPECTOR_TOGGLED),
  ]);

  let errorEvent = once(panelWin, EVENTS.UI_SET_PARAM_ERROR);

  try {
    yield modifyVariableView(panelWin, gVars, 0, "bufferSize", 2048);
  } catch(e) {
    
  }

  yield errorEvent;

  checkVariableView(gVars, 0, {bufferSize: 4096}, "check that unwritable variable is not updated");

  yield teardown(target);
});

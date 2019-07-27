







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

  ok(InspectorView.isVisible(), "InspectorView shown after toggling.");

  ok(isVisible($("#web-audio-editor-details-pane-empty")),
    "InspectorView empty message should still be visible.");
  ok(!isVisible($("#web-audio-editor-tabs")),
    "InspectorView tabs view should still be hidden.");

  
  $("#inspector-pane-toggle").click();
  yield once(panelWin, EVENTS.UI_INSPECTOR_TOGGLED);

  ok(!InspectorView.isVisible(), "InspectorView back to being hidden.");

  
  $("#inspector-pane-toggle").click();
  yield once(panelWin, EVENTS.UI_INSPECTOR_TOGGLED);

  ok(InspectorView.isVisible(), "InspectorView being shown.");
  ok(!isVisible($("#web-audio-editor-tabs")),
    "InspectorView tabs are still hidden.");

  yield clickGraphNode(panelWin, findGraphNode(panelWin, nodeIds[1]));

  ok(!isVisible($("#web-audio-editor-details-pane-empty")),
    "Empty message hides even when loading node while open.");
  ok(isVisible($("#web-audio-editor-tabs")),
    "Switches to tab view when loading node while open.");

  yield teardown(target);
});

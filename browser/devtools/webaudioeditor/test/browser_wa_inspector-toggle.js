







function spawnTest() {
  let [target, debuggee, panel] = yield initWebAudioEditor(SIMPLE_CONTEXT_URL);
  let { panelWin } = panel;
  let { gFront, $, $$, EVENTS, WebAudioInspectorView } = panelWin;
  let gVars = WebAudioInspectorView._propsView;

  let started = once(gFront, "start-context");

  reload(target);

  let [actors] = yield Promise.all([
    get3(gFront, "create-node"),
    waitForGraphRendered(panelWin, 3, 2)
  ]);
  let nodeIds = actors.map(actor => actor.actorID);

  ok(!WebAudioInspectorView.isVisible(), "InspectorView hidden on start.");

  
  $("#inspector-pane-toggle").click();
  yield once(panelWin, EVENTS.UI_INSPECTOR_TOGGLED);

  ok(WebAudioInspectorView.isVisible(), "InspectorView shown after toggling.");

  ok(isVisible($("#web-audio-editor-details-pane-empty")),
    "InspectorView empty message should still be visible.");
  ok(!isVisible($("#web-audio-editor-tabs")),
    "InspectorView tabs view should still be hidden.");
  is($("#web-audio-inspector-title").value, "AudioNode Inspector",
    "Inspector should still have default title.");

  
  $("#inspector-pane-toggle").click();
  yield once(panelWin, EVENTS.UI_INSPECTOR_TOGGLED);

  ok(!WebAudioInspectorView.isVisible(), "InspectorView back to being hidden.");

  
  $("#inspector-pane-toggle").click();
  yield once(panelWin, EVENTS.UI_INSPECTOR_TOGGLED);

  ok(WebAudioInspectorView.isVisible(), "InspectorView being shown.");
  ok(!isVisible($("#web-audio-editor-tabs")),
    "InspectorView tabs are still hidden.");

  click(panelWin, findGraphNode(panelWin, nodeIds[1]));
  yield once(panelWin, EVENTS.UI_INSPECTOR_NODE_SET);

  ok(!isVisible($("#web-audio-editor-details-pane-empty")),
    "Empty message hides even when loading node while open.");
  ok(isVisible($("#web-audio-editor-tabs")),
    "Switches to tab view when loading node while open.");
  is($("#web-audio-inspector-title").value, "OscillatorNode (" + nodeIds[1] + ")",
    "Inspector title updates when loading node while open.");

  yield teardown(panel);
  finish();
}

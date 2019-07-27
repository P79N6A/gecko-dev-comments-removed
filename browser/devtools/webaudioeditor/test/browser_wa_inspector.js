







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
  ok(isVisible($("#web-audio-editor-details-pane-empty")),
    "InspectorView empty message should show when no node's selected.");
  ok(!isVisible($("#web-audio-editor-tabs")),
    "InspectorView tabs view should be hidden when no node's selected.");
  is($("#web-audio-inspector-title").value, "AudioNode Inspector",
    "Inspector should have default title when empty.");

  click(panelWin, findGraphNode(panelWin, nodeIds[1]));
  
  yield Promise.all([
    once(panelWin, EVENTS.UI_INSPECTOR_NODE_SET),
    once(panelWin, EVENTS.UI_INSPECTOR_TOGGLED)
  ]);

  ok(InspectorView.isVisible(), "InspectorView shown once node selected.");
  ok(!isVisible($("#web-audio-editor-details-pane-empty")),
    "InspectorView empty message hidden when node selected.");
  ok(isVisible($("#web-audio-editor-tabs")),
    "InspectorView tabs view visible when node selected.");

  is($("#web-audio-inspector-title").value, "Oscillator",
    "Inspector should have the node title when a node is selected.");

  is($("#web-audio-editor-tabs").selectedIndex, 0,
    "default tab selected should be the parameters tab.");

  click(panelWin, findGraphNode(panelWin, nodeIds[2]));
  yield once(panelWin, EVENTS.UI_INSPECTOR_NODE_SET);

  is($("#web-audio-inspector-title").value, "Gain",
    "Inspector title updates when a new node is selected.");

  yield teardown(target);
});

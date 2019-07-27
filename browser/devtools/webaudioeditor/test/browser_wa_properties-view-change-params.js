







function spawnTest() {
  let [target, debuggee, panel] = yield initWebAudioEditor(CHANGE_PARAM_URL);
  let { panelWin } = panel;
  let { gFront, $, $$, EVENTS, WebAudioInspectorView } = panelWin;
  let gVars = WebAudioInspectorView._propsView;

  
  panelWin.PARAM_POLLING_FREQUENCY = 20;

  let started = once(gFront, "start-context");

  reload(target);

  let [actors] = yield Promise.all([
    getN(gFront, "create-node", 3),
    waitForGraphRendered(panelWin, 3, 0)
  ]);

  let oscId = actors[1].actorID;

  click(panelWin, findGraphNode(panelWin, oscId));
  yield once(panelWin, EVENTS.UI_INSPECTOR_NODE_SET);

  
  yield once(panelWin, EVENTS.CHANGE_PARAM);
  let [[_, args]] = yield getSpread(panelWin, EVENTS.CHANGE_PARAM);
  is(args.actorID, oscId, "EVENTS.CHANGE_PARAM has correct `actorID`");
  ok(args.oldValue < args.newValue, "EVENTS.CHANGE_PARAM has correct `newValue` and `oldValue`");
  is(args.param, "detune", "EVENTS.CHANGE_PARAM has correct `param`");

  let [[_, args]] = yield getSpread(panelWin, EVENTS.CHANGE_PARAM);
  checkVariableView(gVars, 0, { "detune": args.newValue }, "`detune` parameter updated.");
  let [[_, args]] = yield getSpread(panelWin, EVENTS.CHANGE_PARAM);
  checkVariableView(gVars, 0, { "detune": args.newValue }, "`detune` parameter updated.");

  yield teardown(panel);
  finish();
}

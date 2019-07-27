






function* spawnTest() {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, PerformanceController, $, DetailsView, JsCallTreeView } = panel.panelWin;
  Services.prefs.setBoolPref(JIT_PREF, true);


  yield startRecording(panel);
  let rendered = once(JsCallTreeView, EVENTS.JS_CALL_TREE_RENDERED);
  yield stopRecording(panel);

  yield DetailsView.selectView("js-calltree");
  ok(DetailsView.isViewSelected(JsCallTreeView), "The call tree is now selected.");
  yield rendered;

  let recording = PerformanceController.getCurrentRecording();
  is(recording.getConfiguration().withJITOptimizations, true, "recording model has withJITOptimizations as true");

  
  info("Disabling enable-jit-optimizations");
  Services.prefs.setBoolPref(JIT_PREF, false);
  is($("#jit-optimizations-view").hidden, false, "JIT Optimizations panel is displayed when feature enabled.");

  yield startRecording(panel);
  rendered = once(JsCallTreeView, EVENTS.JS_CALL_TREE_RENDERED);
  yield stopRecording(panel);

  yield DetailsView.selectView("js-calltree");
  ok(DetailsView.isViewSelected(JsCallTreeView), "The call tree is now selected.");
  yield rendered;

  recording = PerformanceController.getCurrentRecording();
  is(recording.getConfiguration().withJITOptimizations, false, "recording model has withJITOptimizations as false");
  is($("#jit-optimizations-view").hidden, true, "JIT Optimizations panel is hidden when feature disabled");

  yield teardown(panel);
  finish();
}

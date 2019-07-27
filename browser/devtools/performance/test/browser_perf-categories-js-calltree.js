






function* spawnTest() {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, $, DetailsView, JsCallTreeView } = panel.panelWin;

  
  Services.prefs.setBoolPref(PLATFORM_DATA_PREF, true);

  yield startRecording(panel);
  yield busyWait(100);

  let rendered = once(JsCallTreeView, EVENTS.JS_CALL_TREE_RENDERED);
  yield stopRecording(panel);
  yield DetailsView.selectView("js-calltree");
  yield rendered;

  is($(".call-tree-cells-container").hasAttribute("categories-hidden"), false,
    "The call tree cells container should show the categories now.");
  ok($(".call-tree-category[value=Gecko]"),
    "A category node with the label `Gecko` is displayed in the tree.");

  
  Services.prefs.setBoolPref(PLATFORM_DATA_PREF, false);

  is($(".call-tree-cells-container").getAttribute("categories-hidden"), "",
    "The call tree cells container should hide the categories now.");
  ok(!$(".call-tree-category[value=Gecko]"),
    "A category node with the label `Gecko` doesn't exist in the tree anymore.");

  yield teardown(panel);
  finish();
}

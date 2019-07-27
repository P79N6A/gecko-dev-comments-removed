



"use strict";



add_task(function*() {
  yield addTab(TEST_URL_ROOT + "doc_simple_animation.html");
  let {toolbox, inspector, panel} = yield openAnimationInspector();

  info("Select a non animated node");
  yield selectNode(".still", inspector);

  info("Switch to the rule-view panel");
  inspector.sidebar.select("ruleview");

  info("Select the animated node now");
  yield selectNode(".animated", inspector);

  ok(!panel.playerWidgets || !panel.playerWidgets.length,
    "The panel doesn't show the animation data while inactive");

  info("Switch to the animation panel");
  inspector.sidebar.select("animationinspector");
  yield panel.once(panel.UI_UPDATED_EVENT);

  is(panel.playerWidgets.length, 1,
    "The panel shows the animation data after selecting it");

  info("Switch again to the rule-view");
  inspector.sidebar.select("ruleview");

  info("Select the non animated node again");
  yield selectNode(".still", inspector);

  is(panel.playerWidgets.length, 1,
    "The panel still shows the previous animation data since it is inactive");

  info("Switch to the animation panel again");
  inspector.sidebar.select("animationinspector");
  yield panel.once(panel.UI_UPDATED_EVENT);

  ok(!panel.playerWidgets || !panel.playerWidgets.length,
    "The panel is now empty after refreshing");
});





"use strict";




const TESTCASE_URI = 'data:text/html;charset=utf-8,' +
                     '<div id="testdiv" style="font-size:10px;">Test div!</div>';

let test = asyncTest(function*() {
  yield addTab(TESTCASE_URI);

  info("Opening the computed view and selecting the test node");
  let {toolbox, inspector, view} = yield openComputedView();
  yield selectNode("#testdiv", inspector);

  let fontSize = getComputedViewPropertyValue(view, "font-size");
  is(fontSize, "10px", "The computed view shows the right font-size");

  info("Now switching to the rule view");
  yield openRuleView();

  info("Changing the node's style and waiting for the update");
  let onUpdated = inspector.once("computed-view-refreshed");
  getNode("#testdiv").style.cssText = "font-size: 20px; color: blue; text-align: center";
  yield onUpdated;

  fontSize = getComputedViewPropertyValue(view, "font-size");
  is(fontSize, "20px", "The computed view shows the updated font-size");
  let color = getComputedViewPropertyValue(view, "color");
  is(color, "#00F", "The computed view also shows the color now");
  let textAlign = getComputedViewPropertyValue(view, "text-align");
  is(textAlign, "center", "The computed view also shows the text-align now");
});

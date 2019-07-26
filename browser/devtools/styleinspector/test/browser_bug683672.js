



"use strict";



const {PropertyView} = devtools.require("devtools/styleinspector/computed-view");
const TEST_URI = TEST_URL_ROOT + "browser_bug683672.html";

let test = asyncTest(function*() {
  yield addTab(TEST_URI);
  let {toolbox, inspector, view} = yield openComputedView();

  yield selectNode("#test", inspector);
  yield testMatchedSelectors(view);
});

function* testMatchedSelectors(view) {
  info("checking selector counts, matched rules and titles");

  is(getNode("#test"), view.viewedElement.rawNode(),
      "style inspector node matches the selected node");

  let propertyView = new PropertyView(view, "color");
  propertyView.buildMain();
  propertyView.buildSelectorContainer();
  propertyView.matchedExpanded = true;

  yield propertyView.refreshMatchedSelectors();

  let numMatchedSelectors = propertyView.matchedSelectors.length;
  is(numMatchedSelectors, 6, "CssLogic returns the correct number of matched selectors for div");
  is(propertyView.hasMatchedSelectors, true, "hasMatchedSelectors returns true");
}

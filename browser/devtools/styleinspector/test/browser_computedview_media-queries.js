



"use strict";




const TEST_URI = TEST_URL_ROOT + "doc_media_queries.html";

let {PropertyView} = devtools.require("devtools/styleinspector/computed-view");
let {CssLogic} = devtools.require("devtools/styleinspector/css-logic");

let test = asyncTest(function*() {
  yield addTab(TEST_URI);
  let {toolbox, inspector, view} = yield openComputedView();

  info("Selecting the test element");
  yield selectNode("div", inspector);

  info("Checking property view");
  yield checkPropertyView(view);
});

function checkPropertyView(view) {
  let propertyView = new PropertyView(view, "width");
  propertyView.buildMain();
  propertyView.buildSelectorContainer();
  propertyView.matchedExpanded = true;

  return propertyView.refreshMatchedSelectors().then(() => {
    let numMatchedSelectors = propertyView.matchedSelectors.length;

    is(numMatchedSelectors, 2,
        "Property view has the correct number of matched selectors for div");

    is(propertyView.hasMatchedSelectors, true,
        "hasMatchedSelectors returns true");
  });
}

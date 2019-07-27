



"use strict";



add_task(function*() {
  yield addTab("data:text/html;charset=utf-8,default styles test");

  info("Creating the test document");
  content.document.body.innerHTML = '<style type="text/css"> ' +
    '.matches {color: #F00;}</style>' +
    '<span id="matches" class="matches">Some styled text</span>' +
    '</div>';
  content.document.title = "Style Inspector Search Filter Test";

  info("Opening the computed-view");
  let {toolbox, inspector, view} = yield openComputedView();

  info("Selecting the test node");
  yield selectNode("#matches", inspector);

  yield testToggleDefaultStyles(inspector, view);
  yield testAddTextInFilter(inspector, view);
});


function* testToggleDefaultStyles(inspector, computedView) {
  info("checking \"Browser styles\" checkbox");

  let doc = computedView.styleDocument;
  let checkbox = computedView.includeBrowserStylesCheckbox;
  let onRefreshed = inspector.once("computed-view-refreshed");
  checkbox.click();
  yield onRefreshed;
}

function* testAddTextInFilter(inspector, computedView) {
  info("setting filter text to \"color\"");

  let doc = computedView.styleDocument;
  let searchField = computedView.searchField;
  let onRefreshed = inspector.once("computed-view-refreshed");
  searchField.focus();

  let win = computedView.styleWindow;
  EventUtils.synthesizeKey("c", {}, win);
  EventUtils.synthesizeKey("o", {}, win);
  EventUtils.synthesizeKey("l", {}, win);
  EventUtils.synthesizeKey("o", {}, win);
  EventUtils.synthesizeKey("r", {}, win);

  yield onRefreshed;

  info("check that the correct properties are visible");

  let propertyViews = computedView.propertyViews;
  propertyViews.forEach(function(propView) {
    let name = propView.name;
    is(propView.visible, name.indexOf("color") > -1,
      "span " + name + " property visibility check");
  });
}

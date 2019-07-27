



"use strict";




add_task(function*() {
  yield addTab(TEST_URL_ROOT + "doc_simple_animation.html");
  let {inspector, panel, window} = yield openAnimationInspector();
  let doc = window.document;

  let toolbar = doc.querySelector("#toolbar");
  ok(toolbar, "The panel contains the toolbar element");
  ok(toolbar.querySelector("#toggle-all"), "The toolbar contains the toggle button");
  ok(isNodeVisible(toolbar), "The toolbar is visible");

  info("Select an animated node");
  yield selectNode(".animated", inspector);

  toolbar = doc.querySelector("#toolbar");
  ok(toolbar, "The panel still contains the toolbar element");
  ok(isNodeVisible(toolbar), "The toolbar is still visible");
});
